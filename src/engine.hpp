#pragma once
#include <deque>
#include <memory>
#include <thread>
#include <map>
#include <unordered_map>
#include "semaphore.hpp"
#include "event.hpp"
#include "timers.hpp"
#include "socket.hpp"
#include "syncqueue.hpp"
#include "event.hpp"
#include "resolver.hpp"

namespace Sb {
      class Engine final {
            public:
                  static void start(int minWorkersPerCpu = 4);
                  static void stop();
                  static void init();
                  static void add(const std::shared_ptr<Socket>& what, bool const replace = false);
                  static void remove(Socket* const what);
                  static void triggerWrites(Socket* const what);
                  static void runAsync(Event* const event);
                  static Resolver& resolver();
                  static NanoSecs setTimer(Event* const timer, NanoSecs const& timeout);
                  static NanoSecs cancelTimer(Event* const timer);
                  ~Engine();
                  void sync();
                  void startWorkers(int const minWorkersPerCpu);
                  void stopWorkers();

            private:
                  static void setTrigger(NanoSecs const& when);
                  static void blockSignals();

                  class Worker {
                        public:
                              Worker() = delete;
                              Worker(void (func(Worker*) noexcept)) : thread(func, this) {
                              }
                              ~Worker();
                              std::thread thread;
                              std::atomic_bool exited;
                  };

            private:
                  Engine();
                  void doEpoll(Worker* me) noexcept;
                  static void doWork(Worker* me) noexcept;
                  static void signalHandler(int);
                  static void initSignals();

                  void doStop();
                  void doSignalHandler();
                  void handleTimerExpired();
                  NanoSecs doSetTimer(Event* const timer, NanoSecs const& timeout);
                  NanoSecs doCancelTimer(Event* const timer);
                  void doSetTrigger(NanoSecs const& timeout);
                  void doInit(int const minWorkersPerCpu);
                  std::shared_ptr<Runnable> getSocket(Socket const* const ev);
                  void run(Socket* const sock, const uint32_t events) const;
                  void doEpoll();
                  void worker(Worker& me);
                  void doAdd(std::shared_ptr<Socket> const& what, bool const replace);
                  void doRemove(Socket* const what);
                  void doTriggerWrites(Socket* const what);
                  void doRunAsync(Event* const event);

            private:
                  static Engine* theEngine;
                  Semaphore sem;
                  SyncQueue<Event> eventQueue;
                  bool stopping;
                  bool timerPending;
                  std::atomic_int activeCount;
                  std::thread::id epollTid;
                  std::thread::native_handle_type epollThreadHandle;
                  int epollFd = -1;
                  int timerFd = -1;
                  std::vector<Worker*> slaves;
                  std::unordered_map<Socket const*, const std::shared_ptr<Socket>> eventHash;
                  std::mutex evHashLock;
                  Resolver theResolver;
                  Timers timers;

            private:
                  std::size_t const NUM_ENGINE_EVENTS = 0;
                  std::size_t const EPOLL_EVENTS_PER_RUN = 128;
                  NanoSecs const THREAD_TERMINATE_WAIT_TIME = NanoSecs{500'000};
      };
}
