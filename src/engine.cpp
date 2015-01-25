﻿#include <sys/epoll.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <algorithm>
#include "engine.hpp"
#include "semaphore.hpp"
#include "utils.hpp"
#include "logger.hpp"
#include <sys/timerfd.h>

namespace Sb {
	Engine* Engine::theEngine = nullptr;

	Engine::Worker::~Worker() {
		logDebug("Worker destroyed");
		thread.detach();
	}
	Engine::Engine()
		: stopped(false),
		  epollTid(std::this_thread::get_id()),
		  epollFd(::epoll_create(23423)),
		  timerFd(::timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK)) {
		initSignals();
		Logger::start();
		Logger::setMask(Logger::LogType::EVERYTHING);
		Logger::setMask(Logger::LogType::NOTHING);
		Logger::setMask(Logger::LogType::WARNING);
Logger::setMask(Logger::LogType::EVERYTHING);
		assert(epollFd >=0, "Failed to create epollFd");
		assert(timerFd >=0, "Failed to create timerFd");
		epoll_event event = { 0, { 0 } };
		event.events = EPOLLIN | EPOLLET;
		event.data.ptr = nullptr;
		pErrorThrow (::epoll_ctl (epollFd, EPOLL_CTL_ADD, timerFd, &event));
	}

	Engine::~Engine() {
		logDebug("Engine::~Engine");
		::signal(SIGINT, SIG_DFL);
		::close(timerFd);
		::close(epollFd);
		eventHash.clear();
		logDebug("Engine destroyed. To restart call Init() then Go().");
		Logger::stop();
	}

	void Engine::start(int minWorkersPerCpu) {
		std::unique_ptr<Engine> tmp(Engine::theEngine);
		theEngine->doInit(minWorkersPerCpu);
		Engine::theEngine = nullptr;
	}

	void Engine::signalHandler(int signalNum) {
		if(signalNum != SIGCONT && Engine::theEngine->epollTid == std::this_thread::get_id()) {
			Engine::theEngine->stop();
		}
	}

	void Engine::initSignals()
	{
		struct sigaction sigAction;
		sigAction.sa_handler = Engine::signalHandler;
		sigAction.sa_flags = SA_RESTART;
		sigAction.sa_restorer = nullptr;
		::sigemptyset(&sigAction.sa_mask);
		::sigaction(SIGINT, &sigAction, nullptr);
		sigset_t sigMask;
		::sigemptyset(&sigMask);
		::sigaddset(&sigMask, SIGINT);
	}

	void Engine::doInit(int minWorkersPerCpu) {
		assert(theEngine != nullptr, "Need to Add() something before Go().");

		int initialNumThreadsToSpawn = std::thread::hardware_concurrency() * minWorkersPerCpu + 1;

		logDebug("Starting with " + intToString(initialNumThreadsToSpawn) + " threads");
		for(int i = 0; i < initialNumThreadsToSpawn; i++) {
			logDebug("Started thread " + intToString(i));
			slaves.push_back(new Worker(Engine::doWork));

		}
		doEpoll();
		bool waiting = true;
		for(int i = MAX_SHUTDOWN_ATTEMPTS; i > 0 && waiting; i--) {
			for(auto& slave: slaves) {
				sem.signal();
				sem.signal();
				interrupt(*slave);
			}
			waiting = false;
			for(auto& slave: slaves) {
				if(!slave->exited) {
					waiting = true;
				}
			}
			std::this_thread::sleep_for(NanoSecs(SHUTDOWN_WAIT_CHECK_NSECS));
		}

		for(auto& slave: slaves) {
			delete slave;
		}
		logDebug("All done, exiting.");
	}

	void Engine::setTimer(Epoll* owner, const int timerId, const NanoSecs timeout) {
		if(Engine::theEngine == nullptr) {
			throw std::runtime_error("Please call Engine::Init() first");
		}
		theEngine->doSetTimer(owner, timerId, timeout);
	}

	void Engine::cancelTimer(Epoll* owner, const int timerId) {
		if(Engine::theEngine == nullptr) {
			throw std::runtime_error("Please call Engine::Init() first");
		}
		theEngine->doCancelTimer(owner, timerId);
	}

	void Engine::onTimerExpired( ) {
		if(Engine::theEngine == nullptr) {
			throw std::runtime_error("Forgot to cancel the timer");
		}
		Engine::theEngine->doOnTimerExpired();
		logDebug("Engine::onTimerExpired");
	}

	void Engine::armTimerIn(const NanoSecs& timeout) const	{
		logDebug("Engine::armTimerIn " + intToString(timeout.count()));
		assert(timeout.count() > 0, "Invalid timeout, must be greater than zero");
		itimerspec new_timer;
		new_timer.it_interval.tv_sec = 0;
		new_timer.it_interval.tv_nsec = 0;
		new_timer.it_value.tv_sec = timeout.count() / NanoSecsInSecs;
		new_timer.it_value.tv_nsec = timeout.count() % NanoSecsInSecs;
		itimerspec old_timer;
		pErrorThrow(::timerfd_settime(timerFd, 0, &new_timer, &old_timer));
	}

	void Engine::disArmTimer() const {
		itimerspec new_timer;
		new_timer.it_interval.tv_sec = 0;
		new_timer.it_interval.tv_nsec = 0;
		new_timer.it_value.tv_sec = 0;
		new_timer.it_value.tv_nsec = 0;
		itimerspec old_timer;
		pErrorThrow(::timerfd_settime(timerFd, 0, &new_timer, &old_timer));
	}

	void Engine::doOnTimerExpired() {
		uint64_t value;
		::read(timerFd, &value, 8);
		logDebug("Engine::doOnTimerExpired() BO: " +
				 intToString(timersByOwner.size()) + " BD: " +
				 intToString(timesByDate.size()));
		const Epoll* ep = nullptr;
		int id = -1;
		{
			std::lock_guard<std::mutex> lock(timeLock);
			ep = timesByDate.begin()->second.ep;
			id = timesByDate.begin()->second.id;
			auto diff = HighResClock::now() - timesByDate.begin()->first;
			logDebug("Engine::doOnTimerExpired diff " + intToString(diff.count()));
		}
		doCancelTimer(ep, id);
		auto what = getEpoll(ep);
		what->handleTimer(id);
		logDebug("Engine::doOnTimerExpired done");
	}

	std::shared_ptr<Epoll> Engine::getEpoll(const Epoll* what)
	{
		std::lock_guard<std::mutex> sync(evHashLock);
		auto it = eventHash.find(what);
		if(it == eventHash.end()) {
			logDebug("Cannot find what in evHashes");
			return nullptr;
		}
		return it->second;
	}

	void Engine::doSetTimer(const Epoll* what, const int timerId, const NanoSecs& timeout) {
		logDebug("Engine::doSetTimer() BO: " +
				 intToString(timersByOwner.size()) + " BD: " +
				 intToString(timesByDate.size()));
		logDebug("Timer::doSetTimer() for " + intToString(timerId) + " duration "
				 + intToString(timeout.count()) + "ns");

		auto now = HighResClock::now();
		TimePointNs when = now + timeout;
		std::lock_guard<std::mutex> lock(timeLock);
		// Remove old timer
		auto oldStart = timesByDate.begin();
		auto iiByOwner = timersByOwner.equal_range(what);
		if(iiByOwner.first != iiByOwner.second) {
			for_each(iiByOwner.first, iiByOwner.second, [&, timerId](const auto& bo) {
				auto iiByDate = timesByDate.equal_range(bo.second.tp);
				if(iiByDate.first != iiByDate.second) {
					for(auto it = iiByDate.first; it != iiByDate.second; ++it) {
						if(timerId == it->second.id) {
							timesByDate.erase(it);
						}
					}
				}
			});
			for(auto it = iiByOwner.first; it != iiByOwner.second; ++it) {
				if(timerId == it->second.id) {
					timersByOwner.erase(it);
				}
			}
		}
		// remove old timer
		logDebug("Engine::doSetTimer() remove  BO: " +
				 intToString(timersByOwner.size()) + " BD: " +
				 intToString(timesByDate.size()));
		timersByOwner.insert(std::make_pair(what,  TimerDateId { when, timerId }));
		timesByDate.insert(std::make_pair(when, TimerEpollId { what, timerId }));
		if(oldStart != timesByDate.begin()) {
			armTimerIn(timeout);
		}
		logDebug("Engine::doSetTimer() BO: " +
				 intToString(timersByOwner.size()) + " BD: " +
				 intToString(timesByDate.size()));
	}

	void Engine::doCancelTimer(const Epoll* what, const int timerId) {
		logDebug("Engine::doCancelTimer() for " + intToString(timerId));
		logDebug("Engine::doCancelTimer() BO: " +
				 intToString(timersByOwner.size()) + " BD: " +
				 intToString(timesByDate.size()));
		std::lock_guard<std::mutex> sync(timeLock);
		auto oldStart = timesByDate.begin();
		auto iiByOwner = timersByOwner.equal_range(what);
		if(iiByOwner.first != iiByOwner.second) {
			for_each(iiByOwner.first, iiByOwner.second, [&, timerId](const auto& bo) {
				auto iiByDate = timesByDate.equal_range(bo.second.tp);
				if(iiByDate.first != iiByDate.second) {
					for(auto it = iiByDate.first; it != iiByDate.second; ++it) {
						if(timerId == it->second.id) {
							timesByDate.erase(it);
						}
					}
				}
			});
			for(auto it = iiByOwner.first; it != iiByOwner.second; ++it) {
				if(timerId == it->second.id) {
					timersByOwner.erase(it);
				}
			}
		}
		logDebug("Engine::doCancelTimer() BO: " +
				 intToString(timersByOwner.size()) + " BD: " +
				 intToString(timesByDate.size()));
		if(oldStart != timesByDate.begin()) {
			if(timesByDate.size() > 0) {
				armTimerIn(std::max(NanoSecs {1 }, NanoSecs { timesByDate.begin()->first -  HighResClock::now() }));
			} else {
				disArmTimer();
			}
		}
	}

	void Engine::doCancelAllTimers(const Epoll* what) {
		logDebug("Engine::doCancelAllTimers() BO: " +
				 intToString(timersByOwner.size()) + " BD: " +
				 intToString(timesByDate.size()));
		std::lock_guard<std::mutex> sync(timeLock);
		auto oldStart = timesByDate.begin();
		auto iiByOwner = timersByOwner.equal_range(what);
		for_each(iiByOwner.first, iiByOwner.second, [&](const auto& bo) {
			 auto iiByDate = timesByDate.equal_range(bo.second.tp);
			 timesByDate.erase(iiByDate.first, iiByDate.second);
		});
		timersByOwner.erase(iiByOwner.first, iiByOwner.second);
		if(oldStart != timesByDate.begin()) {
			if(timesByDate.size() > 0) {
				armTimerIn(std::max(NanoSecs {1 }, NanoSecs { timesByDate.begin()->first -  HighResClock::now() }));
			} else {
				disArmTimer();
			}
		}
		logDebug("Engine::doCancelAllTimers() BO: " +
				 intToString(timersByOwner.size()) + " BD: "+
				 intToString(timesByDate.size()));
	}

	void Engine::doAdd(const std::shared_ptr<Epoll>& what) {
		std::lock_guard<std::mutex> sync(evHashLock);
		eventHash.insert(std::make_pair(what.get(), what));
		epoll_event event = { 0, { 0 } };
		event.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP;
		event.data.ptr = what.get();
		pErrorThrow (::epoll_ctl (epollFd, EPOLL_CTL_ADD, what->getFd(), &event));
	}

	void Engine::add(const std::shared_ptr<Epoll>& what) {
		if(Engine::theEngine == nullptr) {
			throw std::runtime_error("Please call Engine::Init() first");
		}
		theEngine->doAdd(what);
	}

	void Engine::doRemove(const Epoll* what) {
		std::lock_guard<std::mutex> sync(evHashLock);
		epoll_event event = { 0, { 0 } };
		auto it = eventHash.find(what);
		auto ptr = it->second;
		pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_DEL, ptr->getFd(), &event));
		doCancelAllTimers(what);
		eventHash.erase(it);
	}

	void Engine::remove(const Epoll* what)
	{
		if(Engine::theEngine == nullptr) {
			throw std::runtime_error("Please call Engine::Init() first");
		}
		theEngine->doRemove(what);
	}

	void Engine::init() {
		if(Engine::theEngine == nullptr) {
			Engine::theEngine = new Engine();
		}
	}

	void Engine::stop() {
		stopped = true;
	}

	void Engine::worker(Worker& me) {
		logDebug("Worker started");
		try {
			while(!stopped) {
				me.stats.MarkIdleStart();
				sem.wait();
				me.stats.MarkIdleEnd();
				if(stopped) {
					break;
				}
				bool noEvents = true;
				auto& event  = eventQueue.removeAndIsEmpty (noEvents);
				if(noEvents) {
					logDebug("Run without event");
					continue;
				}
				logDebug("Running with events " + pollEventsToString(event.epollEvents()));

				if(event.epollEvent() == nullptr) {
					Engine::onTimerExpired();
				} else  {
					std::shared_ptr<Epoll> ptr = getEpoll(event.epollEvent());
					if(ptr != nullptr) {
						ptr->run(event.epollEvents());
					}
				}
			}
		} catch(std::exception& e) {
			logError(std::string("Engine::worker threw a ") + e.what());
			Engine::theEngine->stop();
		} catch(...) {
			logError("Unknown exception in Engine::worker");
			Engine::theEngine->stop();
		}
		me.exited = true;
		logDebug("Worker exited " + intToString(me.id));
	}

	void Engine::doWork(Worker* me) noexcept {
		theEngine->worker(*me);
	}

	void Engine::doEpoll() {
		try {
			logDebug("Starting Engine::Epoll");
			while(!stopped) {
				epoll_event events[EPOLL_EVENTS_PER_RUN];
				int num = epoll_wait(epollFd, events, EPOLL_EVENTS_PER_RUN, -1);
				logDebug("Epoll " + intToString(num));
				if(stopped) {
					break;
				}
				if(num < 0) {
					if(errno == EINTR || errno == EAGAIN) {
						logDebug("Epoll errno " + intToString(errno));
						continue;
					} else {
						pErrorThrow(num);
					}
				} else {
					for(int i = 0; i < num; i++) {
						logDebug("Added");
						eventQueue.add(EpollEvent(static_cast<Epoll *>(events[i].data.ptr), events[i].events));
						sem.signal();
					}
				}
			}
		} catch(std::exception& e) {
			logError(std::string("Engine::epollThread threw a ") + e.what());
			Engine::theEngine->stop();
		} catch(...) {
			logError("Unknown exception in Engine::epollThread");
			Engine::theEngine->stop();
		}
		logDebug("Stopping Epolling");
	}

	void Engine::interrupt(Worker &thread) const {
		if(!thread.exited && thread.thread.native_handle() != 0) {
			::pthread_kill(thread.thread.native_handle(), SIGINT);
		}
	}
}
