#pragma once
#include <deque>
#include <memory>
#include <thread>
#include <map>
#include "semaphore.hpp"
#include "event.hpp"
#include "timers.hpp"
#include "socket.hpp"
#include "resolver.hpp"

namespace Sb {
      class Engine final {
      public:
            static void start(int minWorkersPerCpu = 4);
            static void stop();
            static void init();
            static void add(std::shared_ptr<Socket>& what);
            static void remove(std::weak_ptr<Socket>& what);
            static void triggerWrites(Socket* const what);
            static void runAsync(Event const& event);
            static Resolver&resolver();
            static NanoSecs setTimer(Event const& timer, NanoSecs const&timeout);
            static NanoSecs cancelTimer(Event const& timer);
            ~Engine();
            void startWorkers(int const minWorkersPerCpu);
            void stopWorkers();
      private:
            class Worker;

            Engine();
            friend void signalHandler(int);
            void doEpoll(Worker* me) noexcept;
            static void doWork(Worker* me) noexcept;
            void doStop();
            void doSignalHandler();
            void handleTimerExpired();
            NanoSecs doSetTimer(Event const& timer, NanoSecs const& timeout);
            NanoSecs doCancelTimer(Event const& timer);
            void setTimerTrigger(Event const* const what, NanoSecs const& when);
            void doInit(int const minWorkersPerCpu);
            void newEvent(uint64_t const evId, uint32_t const events);
            void run(Socket* const sock, const uint32_t events);
            void doEpoll();
            void worker(Worker&me);
            void doAdd(std::shared_ptr<Socket>& what);
            void doRemove(std::weak_ptr<Socket>& what);
            void doTriggerWrites(Socket* const what);
            void doRunAsync(Event const& event);
            void clearTimer() const;

      private:
            static Engine* theEngine;
            std::mutex timerLock;
            Semaphore sem;
            std::mutex eqLock;
            std::deque<Event> eventQueue;
            std::unique_ptr<Event> timerEvent;
            bool stopping;
            std::atomic_int activeCount;
            std::thread::id epollTid;
            std::thread::native_handle_type epollThreadHandle;
            int epollFd = -1;
            int timerFd = -1;
            std::vector<Worker*> slaves;
            std::mutex evHashLock;
            std::map<uint64_t, std::shared_ptr<Socket>> eventHash;
            Resolver theResolver;
            Timers timers;
      private:
            uint64_t const timerEvId = 0;
            uint64_t evCounter = timerEvId;
            std::size_t const NUM_ENGINE_EVENTS = 0;
            std::size_t const EPOLL_EVENTS_PER_RUN = 128;
            NanoSecs const THREAD_TERMINATE_WAIT_TIME = NanoSecs{ONE_MS_IN_NS};
      };

      class Engine::Worker {
      public:
            Worker() = delete;

            Worker(void (func(Worker*) noexcept)) : thread(func, this) {
            }

            ~Worker();
            std::thread thread;
            bool exited;
      };
}
