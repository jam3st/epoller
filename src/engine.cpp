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

      Engine::Engine() : stopping(false), activeCount(0), epollTid(std::this_thread::get_id()), epollThreadHandle(::pthread_self()),
                         epollFd(::epoll_create1(EPOLL_CLOEXEC)), timerFd(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)),
                         timers(std::bind(&Engine::setTimerTrigger, this, std::placeholders::_1, std::placeholders::_2)) {
            Logger::start();
            Logger::setMask(Logger::LogType::EVERYTHING);
            assert(epollFd >= 0, "Failed to create epollFd");
            assert(timerFd >= 0, "Failed to create timerFd");
            epoll_event event = {EPOLLIN | EPOLLONESHOT | EPOLLET, {.fd = timerFd}};
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
            if (Engine::theEngine != nullptr) {
                  Engine::theEngine->doStop();
            }
      }

      void signalHandler(int) {
            Engine::theEngine->doSignalHandler();
      }

      void Engine::doSignalHandler() {
            if (!stopping && epollTid == std::this_thread::get_id()) {
                  std::lock_guard<std::mutex> sync(evHashLock);
                  stopping = true;
            }
      }

      void Engine::startWorkers(int minWorkersPerCpu) {
            std::lock_guard<std::mutex> sync(evHashLock);
            int initialNumThreadsToSpawn = std::thread::hardware_concurrency() * minWorkersPerCpu + 1;
            for (int i = 0; i < initialNumThreadsToSpawn; ++i) {
                  slaves.push_back(new Worker(Engine::doWork));
            }
      }

      void Engine::stopWorkers() {
            bool waiting = true;
            for (int i = 1024 * std::thread::hardware_concurrency(); i > 0 && waiting; i--) {
                  waiting = false;
                  for (auto&slave : slaves) {
                        if (!slave->exited) {
                              sem.signal();
                              waiting = true;
                        }
                  }
                  if (waiting) {
                        std::this_thread::sleep_for(THREAD_TERMINATE_WAIT_TIME);
                  }
            }
            assert(!waiting, "Engine::stopWorkers failed to stop");
            for (auto&slave : slaves) {
                  delete slave;
            }
      }

      void Engine::doInit(int const minWorkersPerCpu) {
            assert(eventHash.size() > NUM_ENGINE_EVENTS || timerEvent, "Engine::doInit Need to Add() something before Go().");
            std::signal(SIGPIPE, signalHandler);
            for (int i = SIGHUP; i < _NSIG; ++i) {
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
            for (int i = SIGHUP; i < _NSIG; ++i) {
                  std::signal(i, SIG_DFL);
            }
            timerEvent.reset();
            eventHash.clear();
            timers.clear();
            eventQueue.clear();
      }

      void Engine::triggerWrites(Socket* const what) {
            if (Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::triggerWrites Please call Engine::Init() first");
            }
            Engine::theEngine->doTriggerWrites(what);
      }

      void Engine::doTriggerWrites(Socket* const what) {
            auto ev = getSocket(what->fd);
            uint32_t const evts = EPOLLOUT;
            if (ev) {
                  {
                        std::lock_guard<std::mutex> sync(eqLock);
                        eventQueue.push_back(Event(ev, std::bind(&Engine::run, this, what, evts)));
                  }
                  sem.signal();
            }
      }

      void Engine::runAsync(Event* const event) {
            if (Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::runAsync Please call Engine::Init() first");
            }
            Engine::theEngine->doRunAsync(event);
      }

      void Engine::doRunAsync(Event* const event) {
            {
                  std::lock_guard<std::mutex> sync(eqLock);
                  eventQueue.push_back(*event);
            }
            sem.signal();
      }

      NanoSecs Engine::setTimer(Event* const timer, NanoSecs const&timeout) {
            if (Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::setTimer Please call Engine::Init() first");
            }
            return Engine::theEngine->doSetTimer(timer, timeout);
      }

      NanoSecs Engine::doSetTimer(Event* const timer, NanoSecs const&timeout) {
            std::lock_guard<std::mutex> sync(timerLock);
            return timers.setTimer(timer->owner(), timer, timeout);
      }

      NanoSecs Engine::cancelTimer(Event* const timer) {
            if (Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::cancelTimer Please call Engine::Init() first");
            }
            return Engine::theEngine->doCancelTimer(timer);
      }

      NanoSecs Engine::doCancelTimer(Event* const timer) {
            std::lock_guard<std::mutex> sync(timerLock);
            return timers.cancelTimer(timer->owner(), timer);
      }

      void Engine::handleTimerExpired() {
            bool queued = false; {
                  std::lock_guard<std::mutex> sync(timerLock);
                  if (timerEvent) {
                        clearTimer();
                        {
                              std::lock_guard<std::mutex> sync(eqLock);
                              eventQueue.push_back(*timerEvent);
                              queued = true;
                        }
                        timerEvent.reset();
                        timers.handleTimerExpired();
                  }
            }
            if(queued) {
                  sem.signal();
            }
      }

      void Engine::clearTimer() const {
            for (; ;) {
                  uint64_t value;
                  auto numRead = read(timerFd, &value, sizeof(value));
                  if (numRead == -1) {
                        if (errno == EINTR) {
                              continue;
                        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                              break;
                        } else {
                              pErrorThrow(numRead, timerFd);
                        }
                  }
            }
      }

      void Engine::setTimerTrigger(Event const* const what, NanoSecs const& when) {
            clearTimer();
            if (when == NanoSecs{0}) {
                  timerEvent.reset();
            } else {
                  timerEvent.reset();
                  timerEvent = std::make_unique<Event>(*what);
                  epoll_event event = {EPOLLIN | EPOLLONESHOT | EPOLLET, {.fd = timerFd}};
                  pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_MOD, timerFd, &event), epollFd);
                  auto const wNs = when.count();
                  itimerspec oldTimer, newTimer = {.it_interval = {0, 0}, .it_value = {static_cast<decltype(newTimer.it_value.tv_sec)>(wNs / ONE_SEC_IN_NS),
                                                                                       static_cast<decltype(newTimer.it_value.tv_nsec)>(wNs % ONE_SEC_IN_NS)}};
                  pErrorThrow(::timerfd_settime(timerFd, 0, &newTimer, &oldTimer), timerFd);
            }
      }

      void Engine::doAdd(std::shared_ptr<Socket> what) {
            if (!stopping) {
                  auto const epollOut = what->waitingOutEvent();
                  std::lock_guard<std::mutex> sync(evHashLock);
                  what->self = what;
                  eventHash.insert(std::make_pair(what->fd, what));
                  epoll_event event = { (epollOut ? EPOLLOUT : 0) | EPOLLONESHOT | EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLET, {.fd = what->fd}};
                  pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_ADD, what->fd, &event), epollFd);
            }
      }

      void Engine::add(std::shared_ptr<Socket> what) {
            if (Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::add Please call Engine::Init() first");
            }
            theEngine->doAdd(what);
      }

      void Engine::doRemove(std::weak_ptr<Socket>& what) {
            if (!stopping) {
                  auto const ref = what.lock();
                  if (ref) {
                        {
                              std::lock_guard<std::mutex> sync(evHashLock);
                              auto it = eventHash.find(ref->fd);
                              assert(it != eventHash.end(), "Not found for removal");
                              eventHash.erase(it);
                              epoll_event event = {0, {0}};
                              pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_DEL, ref.get()->fd, &event), epollFd);
                        }
                        timers.cancelAllTimers(ref.get());
                  }
            }
      }

      void Engine::remove(std::weak_ptr<Socket>& what) {
            if (Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::remove Please call Engine::Init() first");
            }
            theEngine->doRemove(what);
      }

      void Engine::init() {
            if (Engine::theEngine == nullptr) {
                  Engine::theEngine = new Engine();
                  Engine::theEngine->resolver().init();
            }
      }

      Resolver&Engine::resolver() {
            if (Engine::theEngine == nullptr) {
                  throw std::runtime_error("Engine::resolver Please call Engine::Init() first");
            }
            return Engine::theEngine->theResolver;
      }

      void Engine::doStop() {
            if (!stopping && epollTid != std::this_thread::get_id()) {
                  ::pthread_kill(epollThreadHandle, SIGUSR1);
            }
      }

      void Engine::run(Socket* const sock, const uint32_t events) {
            if ((events & EPOLLOUT) != 0) {
                  sock->handleWrite();
            }
            if ((events & (EPOLLIN)) != 0) {
                  sock->handleRead();
            }

            if((events & EPOLLRDHUP) != 0 || (events & EPOLLERR) != 0) {
                  sock->handleError();
                  pErrorLog(sock->getLastError(), sock->fd);
            } else {
                  bool const needOut = sock->waitingOutEvent();
                  std::lock_guard<std::mutex> sync(evHashLock);
                  auto const it = eventHash.find(sock->fd);
                  if(it != eventHash.end()) {
                        epoll_event event = { (needOut ? EPOLLOUT : 0) |  EPOLLONESHOT | EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLET, { .fd = sock->fd } };
                        pErrorThrow(::epoll_ctl(epollFd, EPOLL_CTL_MOD, sock->fd, &event), epollFd);
                  }
            }
      }

      void Engine::worker(Worker&me) {
            try {
                  while (!stopping) {
                        sem.wait();
                        Event event({}, []() {});
                        {
                              std::lock_guard<std::mutex> sync(eqLock);
                              if (eventQueue.size() != 0) {
                                    event = eventQueue.front();
                                    eventQueue.pop_front();
                              }
                        }
                        activeCount++;
                        event();
                        activeCount--;
                        if (eventHash.size() == NUM_ENGINE_EVENTS && !timerEvent && activeCount == 0) {
                              doStop();
                              break;
                        }
                  }
            } catch (std::exception&e) {
                  logError(std::string("Engine::worker threw a ") + e.what());
                  Engine::theEngine->stop();
            } catch (...) {
                  logError("Unknown exception in Engine::worker");
                  Engine::theEngine->stop();
            }
            me.exited = true;
      }

      void Engine::doWork(Worker* me) noexcept {
            theEngine->worker(*me);
      }

      std::shared_ptr<Socket> Engine::getSocket(int const fd) {
            std::shared_ptr<Socket> ret;
            std::lock_guard<std::mutex> sync(evHashLock);
            auto it = eventHash.find(fd);
            if (it != eventHash.end()) {
                  ret = it->second;
            }
            return ret;
      }

      void Engine::doEpoll() {
            try {
                  while (!stopping) {
                        epoll_event epEvents[EPOLL_EVENTS_PER_RUN];
                        int num = epoll_wait(epollFd, epEvents, EPOLL_EVENTS_PER_RUN, -1);
                        if (stopping) {
                              break;
                        }
                        if (num >= 0) {
                              for (int i = 0; i < num; ++i) {
                                    if (epEvents[i].data.fd == timerFd) {
                                          handleTimerExpired();
                                    } else {
                                          auto const ev = getSocket(epEvents[i].data.fd);
                                          if(ev) {
                                                auto const events = epEvents[i].events; {
                                                      std::lock_guard<std::mutex> sync(eqLock);
                                                      eventQueue.push_back(Event(ev, std::bind(&Engine::run, this, ev.get(), events)));
                                                }
                                                sem.signal();
                                          }
                                    }
                              }
                        } else if (num == -1 && (errno == EINTR || errno == EAGAIN)) {
                              continue;
                        } else {
                              pErrorThrow(num, epollFd);
                        }
                  }
            } catch (std::exception&e) {
                  logError(std::string("Engine::epollThread threw a ") + e.what());
                  stop();
            } catch (...) {
                  logError("Unknown exception in Engine::epollThread");
                  stop();
            }
      }
}