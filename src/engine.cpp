#include <sys/epoll.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <algorithm>
#include "engine.hpp"
#include <sys/timerfd.h>

namespace Sb {
      Engine* Engine::theEngine = nullptr;

      Engine::Worker::~Worker() {
            thread.detach();
      }

      Engine::Engine() : eventQueue(EpollEvent(nullptr, 0)),
            stopping(false),
            activeCount(0),
            epollTid(std::this_thread::get_id()),
            epollThreadHandle(::pthread_self()),
            epollFd(::epoll_create(23423)),
            timerFd(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)),
            timers(Engine::setTrigger) {
            blockSignals();
            Logger::start();
            Logger::setMask(Logger::LogType::EVERYTHING);
            assert(epollFd >= 0, "Failed to create epollFd");
            assert(timerFd >= 0, "Failed to create timerFd");
            epoll_event event = { 0, { 0 }};
            event.events = EPOLLIN | EPOLLET;
            event.data.ptr = nullptr;
            pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_ADD, timerFd, &event), epollFd);
      }

      Engine::~Engine() {
            blockSignals();
            ::signal(SIGUSR2, SIG_DFL);
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

      void Engine::signalHandler(int) {
            Engine::theEngine->doSignalHandler();
      }

      void Engine::blockSignals() {
            sigset_t sigMask;
            ::sigfillset(&sigMask);
            pErrorThrow(::sigprocmask(SIG_SETMASK, &sigMask, nullptr));
      }

      void Engine::doSignalHandler() {
            if(epollTid == std::this_thread::get_id()) {
                  stopping = true;
            }
      }

      void Engine::initSignals() {
            struct sigaction sigAction;
            sigAction.sa_handler = Engine::signalHandler;
            sigAction.sa_flags = SA_RESTART;
            sigAction.sa_restorer = nullptr;
            ::sigemptyset(&sigAction.sa_mask);
            pErrorThrow(::sigaction(SIGQUIT, &sigAction, nullptr));
            pErrorThrow(::sigaction(SIGINT, &sigAction, nullptr));
            pErrorThrow(::sigaction(SIGTERM, &sigAction, nullptr));
            pErrorThrow(::sigaction(SIGUSR2, &sigAction, nullptr));
            sigset_t sigMask;
            ::sigfillset(&sigMask);
            ::sigdelset(&sigMask, SIGQUIT);
            ::sigdelset(&sigMask, SIGINT);
            ::sigdelset(&sigMask, SIGTERM);
            ::sigdelset(&sigMask, SIGUSR2);
            pErrorThrow(::sigprocmask(SIG_SETMASK, &sigMask, nullptr));
      }

      void Engine::startWorkers(int minWorkersPerCpu) {
            std::lock_guard<std::mutex> sync(evHashLock);
            int initialNumThreadsToSpawn = std::thread::hardware_concurrency() * minWorkersPerCpu + 1;
            logDebug("Starting with " + std::to_string(initialNumThreadsToSpawn) + " threads");

            for(int i = 0; i < initialNumThreadsToSpawn; ++i) {
                  slaves.push_back(new Worker(Engine::doWork));
            }
      }

      void Engine::stopWorkers() {
            bool waiting = true;

            for(int i = MAX_SHUTDOWN_ATTEMPTS; i > 0 && waiting; i--) {
                  waiting = false;

                  for(auto& slave : slaves) {
                        if(!slave->exited) {
                              eventQueue.add(EpollEvent(nullptr, 0));
                              sem.signal();
                              waiting = true;
                              std::this_thread::yield();
                        }
                  }
            }

            for(auto& slave : slaves) {
                  delete slave;
            }
      }

      void Engine::doInit(int minWorkersPerCpu) {
            assert(eventHash.size() > NUM_ENGINE_EVENTS, "Need to Add() something before Go().");
            initSignals();
            startWorkers(minWorkersPerCpu);
            doEpoll();
            stopWorkers();
            blockSignals();
            theResolver.destroy();
      }

      void Engine::setTimer(TimeEvent* const owner, Timer* const timerId, NanoSecs const& timeout) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Please call Engine::Init() first");
            }

            Engine::theEngine->doSetTimer(owner, timerId, timeout);
      }

      void Engine::doSetTimer(TimeEvent* const owner, Timer* const timerId, NanoSecs const& timeout) {
            timers.setTimer(owner, timerId, timeout);
      }

      NanoSecs Engine::cancelTimer(TimeEvent* const owner, Timer* const timerId) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Please call Engine::Init() first");
            }

            return Engine::theEngine->doCancelTimer(owner, timerId);
      }

      NanoSecs Engine::doCancelTimer(TimeEvent* const owner, Timer* const timerId) {
            auto ret = timers.cancelTimer(owner, timerId);
            return ret;
      }

      void Engine::onTimerExpired() {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Forgot to cancel the timer");
            }

            Engine::theEngine->doOnTimerExpired();
      }

      void Engine::doOnTimerExpired() {
            uint64_t value;
            pErrorThrow(::read(timerFd, &value, sizeof(value)), timerFd);
            auto ev = timers.onTimerExpired();
            std::shared_ptr<TimeEvent> ref;
            {
                  std::lock_guard<std::mutex> sync(evHashLock);
                  auto it = eventHash.find(ev.first);

                  if(it != eventHash.end()) {
                        ref = it->second;
                  }
            }

            if(ref) {
                  auto togo = timers.cancelTimer(ev.first, ev.second);
                  logDebug("Timer togo " + std::to_string(togo.count()));
                  (*ev.second)();
            }

            logDebug("Engine::doOnTimerExpired");
      }

      void Engine::setTrigger(NanoSecs const& when) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Please call Engine::Init() first");
            }

            Engine::theEngine->doSetTrigger(when);
      }

      void Engine::doSetTrigger(const NanoSecs& timeout) const {
            logDebug("Engine::doSetTrigger " + std::to_string(timeout.count()));
            itimerspec new_timer;
            new_timer.it_interval.tv_sec = 0;
            new_timer.it_interval.tv_nsec = 0;
            new_timer.it_value.tv_sec = timeout.count() / NanoSecsInSecs;
            new_timer.it_value.tv_nsec = timeout.count() % NanoSecsInSecs;
            itimerspec old_timer;
            pErrorThrow(::timerfd_settime(timerFd, 0, &new_timer, &old_timer), timerFd);
      }

      void Engine::doAdd(const std::shared_ptr<Socket>& what, const bool replace) {
            std::lock_guard<std::mutex> sync(evHashLock);

            if(!stopping) {
                  logDebug("Engine::add " + std::to_string(what->getFd()));
                  eventHash.insert(std::make_pair(what.get(), what));
                  epoll_event event = { 0, { 0 }};
                  event.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP;
                  event.data.ptr = what.get();

                  if(replace) {
                        epoll_event nullEvent = { 0, { 0 }};
                        pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_DEL, what->getFd(), &nullEvent), epollFd);
                  }

                  pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_ADD, what->getFd(), &event), epollFd);
            } else {
                  throw std::runtime_error("Cannot add when engine is stopping.");
            }
      }

      void Engine::doAddTimer(std::shared_ptr<TimeEvent> const& what) {
            bool found = false;
            {
                  std::lock_guard<std::mutex> sync(evHashLock);
                  found = eventHash.find(what.get()) != eventHash.end();
            }
            assert(!found, "Aleady added " + intToHexString(what.get()));
            std::lock_guard<std::mutex> sync(evHashLock);

            if(!stopping) {
                  eventHash.insert(std::make_pair(what.get(), what));
            } else {
                  throw std::runtime_error("Engine is stopped.");
            }
      }

      void Engine::doRemoveTimer(TimeEvent* const what) {
            std::lock_guard<std::mutex> sync(evHashLock);
            auto it = eventHash.find(what);
            assert(it != eventHash.end(), "Not found for removal");
            auto ptr = it->second;
            timers.cancelAllTimers(what);
            eventHash.erase(it);
      }

      void Engine::addTimer(const std::shared_ptr<TimeEvent>& what) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Please call Engine::Init() first");
            }

            theEngine->doAddTimer(what);
      }

      void Engine::removeTimer(TimeEvent* const what) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Please call Engine::Init() first");
            }

            theEngine->doRemoveTimer(what);
      }

      void Engine::add(const std::shared_ptr<Socket>& what, const bool replace) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Please call Engine::Init() first");
            }

            theEngine->doAdd(what, replace);
      }

      void Engine::doRemove(Socket* const what, const bool replace) {
            std::lock_guard<std::mutex> sync(evHashLock);
            epoll_event event = { 0, { 0 }};
            auto it = eventHash.find(what);
            assert(it != eventHash.end(), "Not found for removal");

            if(!replace) {
                  pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_DEL, what->getFd(), &event), epollFd);
            }

            timers.cancelAllTimers(what);
            eventHash.erase(it);
      }

      void Engine::remove(Socket* const what, bool const replace) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Please call Engine::Init() first");
            }

            theEngine->doRemove(what, replace);
      }

      void Engine::init() {
            if(Engine::theEngine == nullptr) {
                  Engine::theEngine = new Engine();
                  Engine::theEngine->resolver().init();
            }
      }

      Resolver& Engine::resolver() {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Please call Engine::Init() first");
            }

            return Engine::theEngine->theResolver;
      }

      void Engine::doStop() {
            logDebug("Engine::doStop " + std::to_string(stopping));

            if(epollTid != std::this_thread::get_id()) {
                  logDebug("Engine::doStop using SIGUSR2");
                  ::pthread_kill(epollThreadHandle, SIGUSR2);
            }
      }

      void Engine::run(Socket& sock, const uint32_t events) const {
            logDebug("Engine::Run " + pollEventsToString(events) + " " + std::to_string(sock.getFd()));

            if((events & EPOLLOUT) != 0) {
                  sock.handleWrite();
            }

            if((events & (EPOLLIN)) != 0) {
                  sock.handleRead();
            }

            if((events & EPOLLRDHUP) != 0 || (events & EPOLLHUP) != 0 || (events & EPOLLERR) != 0) {
                  sock.handleError();
            }
      }

      void Engine::sync() {
            std::lock_guard<std::mutex> sync(evHashLock);
      }

      void Engine::worker(Worker& me) {
            try {
                  sync();

                  for(; ;) {
                        sem.wait();
                        auto eq = eventQueue.removeAndIsEmpty();

                        if(std::get<1>(eq)) {
                              logDebug("Run without event");
                              continue;
                        }

                        auto event = std::get<0>(eq);

                        if(event.epollEvent() == nullptr) {
                              if(event.epollEvents() == 0) {
                                    break;
                              } else {
                                    activeCount++;
                                    Engine::onTimerExpired();
                                    activeCount--;
                              }
                        } else {
                              std::weak_ptr<TimeEvent> wp;
                              {
                                    std::lock_guard<std::mutex> sync(evHashLock);
                                    auto it = eventHash.find(event.epollEvent());

                                    if(it != eventHash.end()) {
                                          wp = it->second;
                                    }
                              }
                              auto ref = wp.lock();

                              if(ref) {
                                    activeCount++;
                                    run(*dynamic_cast<Socket*>(ref.get()), event.epollEvents());
                                    activeCount--;
                              }
                        }

                        std::lock_guard<std::mutex> sync(evHashLock);

                        if(eventHash.size() == NUM_ENGINE_EVENTS && activeCount == 0) {
                              doStop();
                        }
                  }
            } catch(std::exception& e) {
                  logError(std::string("Engine::worker threw a ") + e.what());
                  __builtin_trap();
                  Engine::theEngine->stop();
            } catch(...) {
                  logError("Unknown exception in Engine::worker");
                  Engine::theEngine->stop();
            }

            me.exited = true;
      }

      void Engine::doWork(Worker* me) noexcept {
            theEngine->worker(*me);
      }

      void Engine::doEpoll() {
            try {
                  logDebug("Starting Engine::Epoll");

                  while(!stopping) {
                        epoll_event events[EPOLL_EVENTS_PER_RUN];
                        int num = epoll_wait(epollFd, events, EPOLL_EVENTS_PER_RUN, -1);

                        if(stopping) {
                              break;
                        }

                        if(num < 0) {
                              if(errno == EINTR || errno == EAGAIN) {
                                    continue;
                              } else {
                                    pErrorThrow(num, epollFd);
                              }
                        } else {
                              for(int i = 0; i < num; ++i) {
                                    auto ev = static_cast<Socket*>(events[i].data.ptr);

                                    if(ev == nullptr) {
                                          logDebug("Timer event on timeFd");
                                    } else {
                                          logDebug("Added event for fd " + std::to_string(ev->getFd()));
                                    }

                                    eventQueue.add(EpollEvent(ev, events[i].events));
                                    sem.signal();
                              }
                        }
                  }
            } catch(std::exception& e) {
                  logError(std::string("Engine::epollThread threw a ") + e.what());
                  stop();
            } catch(...) {
                  logError("Unknown exception in Engine::epollThread");
                  stop();
            }

            logDebug("Stopping Epolling");
      }
}