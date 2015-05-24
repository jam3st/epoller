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

      Engine::Engine() : eventQueue(Event({}, []() {})),
            stopping(false),
            timerPending(false),
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
            ::signal(SIGUSR1, SIG_DFL);
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
            pErrorThrow(::sigaction(SIGUSR1, &sigAction, nullptr));
            sigset_t sigMask;
            ::sigfillset(&sigMask);
            ::sigdelset(&sigMask, SIGQUIT);
            ::sigdelset(&sigMask, SIGINT);
            ::sigdelset(&sigMask, SIGTERM);
            ::sigdelset(&sigMask, SIGUSR1);
            pErrorThrow(::sigprocmask(SIG_SETMASK, &sigMask, nullptr));
      }

      void Engine::startWorkers(int minWorkersPerCpu) {
            std::lock_guard<std::mutex> sync(evHashLock);
            int initialNumThreadsToSpawn = std::thread::hardware_concurrency() * minWorkersPerCpu + 1;
            logDebug("Engine::startWorkers Starting with " + std::to_string(initialNumThreadsToSpawn) + " threads");

            for(int i = 0; i < initialNumThreadsToSpawn; ++i) {
                  slaves.push_back(new Worker(Engine::doWork));
            }
      }

      void Engine::stopWorkers() {
            bool waiting = true;
            eventQueue.clear();
            for(int i = MAX_SHUTDOWN_ATTEMPTS; i > 0 && waiting; i--) {
                  waiting = false;
                  for(auto& slave : slaves) {
                        if(!slave->exited) {
                              sem.signal();
                              waiting = true;
                              std::this_thread::yield();
                        }
                  }
            }
            assert(!waiting, "Engine::stopWorkers failed to stop");
            for(auto& slave : slaves) {
                  delete slave;
            }
      }

      void Engine::doInit(int minWorkersPerCpu) {
            assert(eventHash.size() > NUM_ENGINE_EVENTS || timerPending, "Engine::doInit Need to Add() something before Go().");
            initSignals();
            startWorkers(minWorkersPerCpu);
            doEpoll();
            blockSignals();
            stopWorkers();
            theResolver.destroy();
            eventHash.clear();
      }

      void Engine::runAsync(Event* const event) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::runAsync Please call Engine::Init() first");
            }

            Engine::theEngine->doRunAsync(event);
      }

      void Engine::doRunAsync(Event* const event) {
            eventQueue.addLast(*event);
            sem.signal();
      }

      NanoSecs Engine::setTimer(Event* const timer, NanoSecs const& timeout) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::setTimer Please call Engine::Init() first");
            }

            return Engine::theEngine->doSetTimer(timer, timeout);
      }

      NanoSecs Engine::doSetTimer(Event* const timer, NanoSecs const& timeout) {
            return timers.setTimer(timer->owner(), timer, timeout);
      }

      NanoSecs Engine::cancelTimer(Event* const timer) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::cancelTimer Please call Engine::Init() first");
            }

            return Engine::theEngine->doCancelTimer(timer);
      }

      NanoSecs Engine::doCancelTimer(Event * const timer) {
            auto ret = timers.cancelTimer(timer->owner(), timer);
            return ret;
      }

      void Engine::handleTimerExpired() {
            assert(timerPending, "Engine::onTimerExpired called without timer set");
            timerPending = false;
            for(;;) {
                  uint64_t value;
                  auto numRead = ::read(timerFd, &value, sizeof(value));
                  if(numRead == -1) {
                        if(errno == EINTR) {
                              continue;
                        } else if(errno == EAGAIN || errno == EWOULDBLOCK) {
                              break;
                        } else {
                              pErrorThrow(numRead, timerFd);
                        }
                  }
            }
            auto ev = timers.handleTimerExpired();
            if(ev != nullptr) {
                  eventQueue.addLast(*ev);
                  sem.signal();
            }
            logDebug("Engine::onTimerExpired pending " + std::to_string(timerPending));
      }

      void Engine::setTrigger(NanoSecs const& when) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Please call Engine::Init() first");
            }

            Engine::theEngine->doSetTrigger(when);
      }

      void Engine::doSetTrigger(const NanoSecs& timeout) {
            timerPending = true;
            itimerspec new_timer;
            new_timer.it_interval.tv_sec = 0;
            new_timer.it_interval.tv_nsec = 0;
            new_timer.it_value.tv_sec = timeout.count() / NanoSecsInSecs;
            new_timer.it_value.tv_nsec = timeout.count() % NanoSecsInSecs;
            itimerspec old_timer;
            pErrorThrow(::timerfd_settime(timerFd, 0, &new_timer, &old_timer), timerFd);
      }

      void Engine::doAdd(const std::shared_ptr<Socket>& what) {
            std::lock_guard<std::mutex> sync(evHashLock);

            if(!stopping) {
                  eventHash.insert(std::make_pair(what.get(), what));
                  epoll_event event = { 0, { 0 }};
                  event.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP | EPOLLHUP;
                  event.data.ptr = what.get();
                  pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_ADD, what->getFd(), &event), epollFd);
            } else {
                  throw std::runtime_error("Cannot add when engine is stopping.");
            }
      }

      void Engine::add(const std::shared_ptr<Socket>& what) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Please call Engine::Init() first");
            }
            theEngine->doAdd(what);
      }

      void Engine::doRemove(Socket* const what) {
            std::lock_guard<std::mutex> sync(evHashLock);
            auto it = eventHash.find(what);
            assert(it != eventHash.end(), "Not found for removal");
            epoll_event event = { 0, { 0 }};
            pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_DEL, what->getFd(), &event), epollFd);
            timers.cancelAllTimers(what);
            auto dummy = Event(it->second, []() {});
            eventQueue.removeAll(&dummy);
            eventHash.erase(it);
      }

      void Engine::remove(Socket* const what) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Please call Engine::Init() first");
            }

            theEngine->doRemove(what);
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
                  ::pthread_kill(epollThreadHandle, SIGUSR1);
            }
      }

      void Engine::run(Socket* const sock, const uint32_t events) const {
            logDebug("Engine::Run " + pollEventsToString(events) + " " + std::to_string(sock->fd));
            if(sock->fd < 0) {
                  return;
            }
            if((events & EPOLLOUT) != 0) {
                  sock->handleWrite();
            }

            if((events & (EPOLLIN)) != 0) {
                  sock->handleRead();
            }

            if((events & EPOLLRDHUP) != 0 || (events & EPOLLHUP) != 0 || (events & EPOLLERR) != 0) {
                  sock->handleError();
            }
      }

      void Engine::sync() {
            std::lock_guard<std::mutex> sync(evHashLock);
      }

      void Engine::worker(Worker& me) {
            try {
                  sync();

                  for(;;) {
                        sem.wait();
                        evHashLock.lock();
                        auto const q = eventQueue.removeAndIsEmpty();
                        auto const empty = q.second;
                        auto const event = q.first;
                        evHashLock.unlock();

                        if(!empty) {
                              activeCount++;
                              event();
                              activeCount--;
                        }
                        {
                              std::lock_guard<std::mutex> sync(evHashLock);
                              if(eventHash.size() == NUM_ENGINE_EVENTS && !timerPending && activeCount == 0) {
                                    doStop();
                                    break;
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
      }

      void Engine::doWork(Worker* me) noexcept {
            theEngine->worker(*me);
      }

      std::shared_ptr<Runnable> Engine::getSocket(Socket const* const ev) {
            std::shared_ptr<Runnable> ret;
            std::lock_guard<std::mutex> sync(evHashLock);
            auto it = eventHash.find(ev);

            if(it != eventHash.end()) {
                  ret = it->second;
            }
            return ret;
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
                        if(num >= 0) {
                              for(int i = 0; i < num; ++i) {
                                    if(events[i].data.ptr == nullptr) {
                                          handleTimerExpired();
                                    } else {
                                          auto sock = static_cast<Socket*>(events[i].data.ptr);
                                          auto ev = getSocket(sock);
                                          auto const evts = events[i].events;
                                          if(ev) {
                                                eventQueue.addLast(Event(ev, std::bind(&Engine::run, this, sock, evts)));
                                                sem.signal();
                                          }
                                    }
                              }
                        } else if(num == -1 && (errno == EINTR || errno == EAGAIN)) {
                              continue;
                        } else {
                              pErrorThrow(num, epollFd);
                        }
                  }
            } catch(std::exception& e) {
                  logError(std::string("Engine::epollThread threw a ") + e.what());
                  stop();
            } catch(...) {
                  logError("Unknown exception in Engine::epollThread");
                  stop();
            }
      }
}
