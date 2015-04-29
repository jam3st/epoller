#pragma once

#include <deque>
#include <memory>
#include <thread>
#include <map>
#include <unordered_map>
#include "semaphore.hpp"
#include "timeevent.hpp"
#include "timers.hpp"
#include "socket.hpp"
#include "syncvec.hpp"
#include "timeevent.hpp"
#include "resolver.hpp"

namespace Sb {
    class Engine final {
    public:
        static void start(int minWorkersPerCpu = 4);
        static void stop();
        static void init();
        static void addTimer(const std::shared_ptr<TimeEvent> &what);
        static void removeTimer(TimeEvent *const what);
        static void add(const std::shared_ptr<Socket> &what, const bool replace = false);
        static void remove(Socket *const what, bool const replace = false);
        static Resolver &resolver();
        static void setTimer(TimeEvent *const owner, Timer *const timerId, NanoSecs const &timeout);
        static NanoSecs cancelTimer(TimeEvent *const owner, Timer *const timerId);
        ~Engine();
        void sync();
        void startWorkers(int minWorkersPerCpu);
        void stopWorkers();

    private:
        static void setTrigger(const NanoSecs &when);
        static void blockSignals();

        class Worker {
        public:
            Worker(void (func(Worker *) noexcept)) : thread(func, this) {
            }

            ~Worker();

            std::thread thread;
            std::atomic_bool exited;
        };

    private:
        Engine();
        void doEpoll(Worker *me) noexcept;
        static void onTimerExpired();
        static void doWork(Worker *me) noexcept;
        static void signalHandler(int signalNum);
        static void initSignals();

        void doStop();
        void doSignalHandler();
        void doSetTimer(TimeEvent *const owner, Timer *const timerId, NanoSecs const &timeout);
        void doOnTimerExpired();
        NanoSecs doCancelTimer(TimeEvent *const owner, Timer *const timerId);
        void doSetTrigger(const NanoSecs &timeout) const;
        void doInit(int minWorkersPerCpu);
        void run(Socket &sock, const uint32_t events) const;
        void doEpoll();
        void worker(Worker &me);
        void doAdd(std::shared_ptr<Socket> const& what, const bool replace);
        void doRemove(Socket* const what, const bool replace);
        void doAddTimer(std::shared_ptr<TimeEvent> const &what);
        void doRemoveTimer(TimeEvent* const what);

    private:
        class EpollEvent final {
        public:
            explicit EpollEvent(const Socket *what, uint32_t events) : what(what), events(events) {
            }

            const TimeEvent *epollEvent() const {
                return what;
            };

            uint32_t epollEvents() const {
                return events;
            };
        private:
            const Socket *what;

            const uint32_t events;
        };

        static Engine *theEngine;
        Semaphore sem;
        SyncVec<EpollEvent> eventQueue;
        std::atomic_bool stopping;
        std::atomic_int activeCount;
        std::thread::id epollTid;
        std::thread::native_handle_type epollThreadHandle;
        int epollFd = -1;
        int timerFd = -1;
        std::vector<Worker *> slaves;
        std::unordered_map<const TimeEvent *, const std::shared_ptr<TimeEvent>> eventHash;
        std::mutex evHashLock;
        Resolver theResolver;
        Timers timers;

    private:
        const std::size_t NUM_ENGINE_EVENTS = 1;
        const std::size_t MAX_SHUTDOWN_ATTEMPTS = 5;
        const std::size_t EPOLL_EVENTS_PER_RUN = 4;
    };
}
