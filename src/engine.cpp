#include <sys/epoll.h>
#include <csignal>
#include <unistd.h>
#include <pthread.h>
#include <algorithm>
#include <sys/timerfd.h>
#include "engine.hpp"

namespace Sb {
      Engine* Engine::theEngine = nullptr;

      Engine::Worker::~Worker() {
            thread.detach();
      }

      Engine::Engine() : eventQueue(Event({}, []() {})),
            timerPending(false),
            stopping(false),
            activeCount(0),
            epollTid(std::this_thread::get_id()),
            epollThreadHandle(::pthread_self()),
            epollFd(::epoll_create1(EPOLL_CLOEXEC)),
            timerFd(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)),
            timers(Engine::setTrigger) {
            Logger::start();
            Logger::setMask(Logger::LogType::EVERYTHING);
            assert(epollFd >= 0, "Failed to create epollFd");
            assert(timerFd >= 0, "Failed to create timerFd");
            epoll_event event = { 0, { 0 }};
            event.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
            event.data.ptr = nullptr;
            pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_ADD, timerFd, &event), epollFd);
      }

      Engine::~Engine() {
            ::signal(SIGUSR1, SIG_DFL);
            ::close(timerFd);
            ::close(epollFd);
            logDebug("Engine::~Engine()");
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

      void signalHandler(int) {
            Engine::theEngine->doSignalHandler();
      }

      void Engine::doSignalHandler() {
            if(!stopping && epollTid == std::this_thread::get_id()) {
                  std::lock_guard<std::mutex> sync(evHashLock);
                  stopping = true;
            }
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
            for(int i = 1024 * std::thread::hardware_concurrency(); i > 0 && waiting; i--) {
                  waiting = false;
                  for(auto& slave : slaves) {
                        if(!slave->exited) {
                              sem.signal();
                              waiting = true;
                        }
                  }
                  if(waiting) {
                        std::this_thread::sleep_for(THREAD_TERMINATE_WAIT_TIME);
                  }
            }
            assert(!waiting, "Engine::stopWorkers failed to stop");
            for(auto& slave : slaves) {
                  delete slave;
            }
      }

      void Engine::doInit(int const minWorkersPerCpu) {
            assert(eventHash.size() > NUM_ENGINE_EVENTS || timerPending, "Engine::doInit Need to Add() something before Go().");
            for(int i = SIGHUP; i < _NSIG; ++i) {
                  std::signal(i, SIG_IGN);
            }
            std::signal(SIGQUIT, signalHandler);
            std::signal(SIGINT, signalHandler);
            std::signal(SIGTERM, signalHandler);
            std::signal(SIGUSR1, signalHandler);
            startWorkers(minWorkersPerCpu);
            doEpoll();
            theResolver.destroy();
            stopWorkers();
            for(int i = SIGHUP; i < _NSIG; ++i) {
                  std::signal(i, SIG_DFL);
            }
            eventQueue.clear();
            eventHash.clear();
      }

      void Engine::triggerWrites(Socket* const what) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::triggerWrites Please call Engine::Init() first");
            }
            Engine::theEngine->doTriggerWrites(what);
      }

      void Engine::doTriggerWrites(Socket* const what) {
            auto ev = getSocket(what);
            auto const evts = EPOLLOUT;
            if(ev) {
                  eventQueue.addLast(Event(ev, std::bind(&Engine::run, this, what, evts)));
                  sem.signal();
            }
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
            std::lock_guard<std::mutex> sync(evHashLock);
            return timers.setTimer(timer->owner(), timer, timeout);
      }

      NanoSecs Engine::cancelTimer(Event* const timer) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::cancelTimer Please call Engine::Init() first");
            }

            return Engine::theEngine->doCancelTimer(timer);
      }

      NanoSecs Engine::doCancelTimer(Event * const timer) {
            std::lock_guard<std::mutex> sync(evHashLock);
            return timers.cancelTimer(timer->owner(), timer);
      }

      void Engine::handleTimerExpired() {
            std::lock_guard<std::mutex> sync(evHashLock);
            if(timerPending) {
                  timerPending = false;
                  clearTimer();

                  auto ev = timers.handleTimerExpired();
                  if(ev != nullptr) {
                        eventQueue.addLast(*ev);
                        sem.signal();
                  }
            }
      }

      void Engine::clearTimer() const {
            for(;;) {
                  uint64_t value;
                  auto numRead = read(timerFd, &value, sizeof(value));
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
      }

      void Engine::setTrigger(NanoSecs const& when) {
            if(Engine::theEngine == nullptr) {
                  return;
            }

            Engine::theEngine->doSetTrigger(when);
      }

      void Engine::doSetTrigger(NanoSecs const& timeout) {
            if(timeout == NanoSecs{0}) {
                  timerPending = false;
                  clearTimer();
            } else {
                  timerPending = true;
                  epoll_event event = { 0, { 0 }};
                  event.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
                  event.data.ptr = nullptr;
                  pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_MOD, timerFd, &event), epollFd);
                  itimerspec new_timer;
                  new_timer.it_interval.tv_sec = 0;
                  new_timer.it_interval.tv_nsec = 0;
                  new_timer.it_value.tv_sec = timeout.count() / NanoSecsInSecs;
                  new_timer.it_value.tv_nsec = timeout.count() % NanoSecsInSecs;
                  itimerspec old_timer;
                  pErrorThrow(::timerfd_settime(timerFd, 0, &new_timer, &old_timer), timerFd);
            }
      }

      void Engine::doAdd(const std::shared_ptr<Socket>& what) {
            std::lock_guard<std::mutex> sync(evHashLock);
            if(!stopping) {
                  what->self = what;
                  eventHash.insert(std::make_pair(what.get(), what));
                  epoll_event event = { 0, { 0 }};
                  event.events = what.get()->waitingOutEvent() ? EPOLLOUT : 0;
                  event.events |= EPOLLONESHOT | EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLET;
                  event.data.ptr = what.get();
                  pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_ADD, what->fd, &event), epollFd);
            } else {
                  throw std::runtime_error("Cannot add when engine is stopping.");
            }
      }

      void Engine::add(const std::shared_ptr<Socket>& what) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::add Please call Engine::Init() first");
            }
            theEngine->doAdd(what);
      }

      void Engine::doRemove(std::weak_ptr<Socket> const& what) {
            if(!stopping) {
                  decltype(eventHash)::const_iterator it;
                  {
                        std::lock_guard <std::mutex> sync(evHashLock);
                        auto ref = what.lock();
                        if(ref) {
                              it = eventHash.find(ref.get());
                              assert(it != eventHash.end(), "Not found for removal");
                              eventHash.erase(it);
                              auto dummy = Event(it->second, []() { });
                              eventQueue.removeAll(&dummy);
                              timers.cancelAllTimers(ref.get());
                        }
                  }
            }
      }

      void Engine::remove(std::weak_ptr<Socket> const& what) {
            if(Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::remove Please call Engine::Init() first");
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
                  throw std::runtime_error("Engine::resolver Please call Engine::Init() first");
            }

            return Engine::theEngine->theResolver;
      }

      void Engine::doStop() {
            if(!stopping && epollTid != std::this_thread::get_id()) {
                  ::pthread_kill(epollThreadHandle, SIGUSR1);
            }
      }

      void Engine::run(Socket* const sock, const uint32_t events) const {
            if((events & EPOLLOUT) != 0) {
                  sock->handleWrite();
            }

            if((events & (EPOLLIN)) != 0) {
                  sock->handleRead();
            }

            if((events & EPOLLRDHUP) != 0 || (events & EPOLLERR) != 0) {
                  sock->handleError();
            } else {
                  if(sock->fd < 0) {
                        return;
                  }

                  epoll_event event = {0, {0}};
                  event.events = sock->waitingOutEvent() ? EPOLLOUT : 0;
                  event.events |= EPOLLONESHOT | EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLET;
                  event.data.ptr = sock;
                  ::epoll_ctl(epollFd, EPOLL_CTL_MOD, sock->fd, &event);
            }
      }

      void Engine::sync() {
            std::lock_guard<std::mutex> sync(evHashLock);
      }

      void Engine::worker(Worker& me) {
            try {
                  sync();
                  do {
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
                  } while(!stopping);
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
                  while(!stopping) {
                        epoll_event epEvents[EPOLL_EVENTS_PER_RUN];
                        int num = epoll_wait(epollFd, epEvents, EPOLL_EVENTS_PER_RUN, -1);
                        if(stopping) {
                              break;
                        }
                        if(num >= 0) {
                              for(int i = 0; i < num; ++i) {
                                    if(epEvents[i].data.ptr == nullptr) {
                                          handleTimerExpired();
                                    } else {
                                          auto sock = static_cast<Socket*>(epEvents[i].data.ptr);
                                          auto ev = getSocket(sock);
                                          auto const events = epEvents[i].events;
                                          if(ev) {
                                                if(sock->highPriority()) {
                                                      eventQueue.addFirst(Event(ev, std::bind(&Engine::run, this, sock, events)));
                                                } else {
                                                      eventQueue.addLast(Event(ev, std::bind(&Engine::run, this, sock, events)));
                                                }
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
