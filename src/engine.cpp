#include <sys/epoll.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <algorithm>
#include "engine.hpp"
#include "semaphore.hpp"
#include "utils.hpp"
#include "socket.hpp"
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
		  epollHandle(::pthread_self()),
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

	void Engine::stop() {
		if(Engine::theEngine != nullptr) {
			Engine::theEngine->doStop();
		}
	}

	void Engine::signalHandler(int signalNum) {
		if(signalNum != SIGCONT) {
			Engine::theEngine->doSignalHandler();
		}
	}

	void Engine::doSignalHandler() {
		if(epollTid == std::this_thread::get_id()) {
			Engine::stop();
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

		logDebug("Starting with " + std::to_string(initialNumThreadsToSpawn) + " threads");
		for(int i = 0; i < initialNumThreadsToSpawn; ++i) {
			logDebug("Started thread " + std::to_string(i));
			slaves.push_back(new Worker(Engine::doWork));
		}
		doEpoll();
		bool waiting = true;
		for(int i = MAX_SHUTDOWN_ATTEMPTS; i > 0 && waiting; i--) {
			for(auto& slave: slaves) {
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

	void Engine::setTimer(const TimeEvent* owner, const size_t timerId, const NanoSecs& timeout) {
		if(Engine::theEngine == nullptr) {
			throw std::runtime_error("Please call Engine::Init() first");
		}
		Engine::theEngine->doSetTimer(owner, timerId, timeout);
	}

	void Engine::doSetTimer(const TimeEvent* owner, const size_t timerId, const NanoSecs& timeout) {
		timers.setTimer(owner, timerId, timeout);
		armTimerIn(timers.getTrigger());
	}

	NanoSecs Engine::cancelTimer(TimeEvent* owner, const size_t timerId) {
		if(Engine::theEngine == nullptr) {
			throw std::runtime_error("Please call Engine::Init() first");
		}
		return Engine::theEngine->doCancelTimer(owner, timerId);
	}

	NanoSecs Engine::doCancelTimer(TimeEvent* owner, const size_t timerId) {
		auto ret = timers.cancelTimer(owner, timerId);
		armTimerIn(timers.getTrigger());
		return ret;
	}

	void Engine::onTimerExpired( ) {
		if(Engine::theEngine == nullptr) {
			throw std::runtime_error("Forgot to cancel the timer");
		}
		Engine::theEngine->doOnTimerExpired();
	}

	void Engine::doOnTimerExpired() {
		uint64_t value;
		pErrorThrow(::read(timerFd, &value, sizeof(value)));
		auto ev = timers.onTimerExpired();
		auto what = getEv(ev.first);
		if(what != nullptr) {
			what->handleTimer(ev.second);
		}
		armTimerIn(timers.getTrigger());
		logDebug("Engine::doOnTimerExpired");
	}

	void Engine::armTimerIn(const NanoSecs& timeout) const	{
		logDebug("Engine::armTimerIn " + std::to_string(timeout.count()));
		itimerspec new_timer;
		new_timer.it_interval.tv_sec = 0;
		new_timer.it_interval.tv_nsec = 0;
		new_timer.it_value.tv_sec = timeout.count() / NanoSecsInSecs;
		new_timer.it_value.tv_nsec = timeout.count() % NanoSecsInSecs;
		itimerspec old_timer;
		pErrorThrow(::timerfd_settime(timerFd, 0, &new_timer, &old_timer));
	}

	std::shared_ptr<TimeEvent> Engine::getEv(const TimeEvent* what)
	{
		std::lock_guard<std::mutex> sync(evHashLock);
		auto it = eventHash.find(what);
		if(it == eventHash.end()) {
			logDebug("Cannot find what in evHashes");
			return nullptr;
		}
		return it->second;
	}

	void Engine::doAdd(const std::shared_ptr<Socket>& what, const bool replace) {
		std::lock_guard<std::mutex> sync(evHashLock);
		if(!stopped) {
			eventHash.insert(std::make_pair(what.get(), what));
			epoll_event event = { 0, { 0 } };
			event.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP;
			event.data.ptr = what.get();
			pErrorThrow (::epoll_ctl (epollFd, replace ? EPOLL_CTL_MOD : EPOLL_CTL_ADD, what->getFd(), &event));
		} else {
			throw std::runtime_error("Engine is stopped.");
		}
	}

	void Engine::add(const std::shared_ptr<Socket>& what, const bool replace) {
		if(Engine::theEngine == nullptr) {
			throw std::runtime_error("Please call Engine::Init() first");
		}
		theEngine->doAdd(what, replace);
	}

	void Engine::doRemove(const Socket* what, const bool replace) {
		std::lock_guard<std::mutex> sync(evHashLock);
		epoll_event event = { 0, { 0 } };
		auto it = eventHash.find(what);
		auto ptr = it->second;
		if(!replace) {
			pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_DEL, what->getFd(), &event));
		}
		timers.cancelAllTimers(what);
		eventHash.erase(it);
		if(eventHash.size() == 0) {
			Engine::theEngine->stop();
		}
	}

	void Engine::remove(const Socket* what, const bool replace)
	{
		if(Engine::theEngine == nullptr) {
			throw std::runtime_error("Please call Engine::Init() first");
		}
		theEngine->doRemove(what, replace);
	}

	void Engine::init() {
		if(Engine::theEngine == nullptr) {
			Engine::theEngine = new Engine();
		}
	}

	void Engine::doStop() {
		stopped = true;
		if(epollTid != std::this_thread::get_id()) {
			::pthread_kill(epollHandle, SIGINT);
		}
	}

	void Engine::run(Socket& sock, const uint32_t events) const {
		logDebug("Engine::Run " + pollEventsToString(events));
		if((events & EPOLLIN) != 0) {
			sock.handleRead();
		}
		if((events & EPOLLRDHUP) != 0  || (events & EPOLLHUP) != 0 || (events & EPOLLERR) != 0) {
			sock.handleError();
		} else if((events & EPOLLOUT) != 0) {
			sock.handleWrite();
		}
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
					std::shared_ptr<TimeEvent> ptr = getEv(event.epollEvent());
					if(ptr != nullptr) {
						run(*dynamic_cast<Socket*>(ptr.get()), event.epollEvents());
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
		logDebug("Worker exited " + std::to_string(me.id));
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
				logDebug("doEpoll " + std::to_string(num));
				if(stopped) {
					break;
				}
				if(num < 0) {
					if(errno == EINTR || errno == EAGAIN) {
						logDebug("Epoll errno " + std::to_string(errno));
						continue;
					} else {
						pErrorThrow(num);
					}
				} else {
					for(int i = 0; i < num; ++i) {
						logDebug("Added for " + std::to_string(static_cast<Socket*>(events[i].data.ptr)->getFd()));
						eventQueue.add(EpollEvent(static_cast<Socket*>(events[i].data.ptr), events[i].events));
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
