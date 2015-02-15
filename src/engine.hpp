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
#include "stats.hpp"
#include "resolver.hpp"

namespace Sb {
	class Engine final {
	public:
		static void start(int minWorkersPerCpu = 4);
		static void stop();
		static void init();
		static void add(const std::shared_ptr<Socket>& what, const bool replace = false);
		static void remove(const Socket* what, const bool replace = false);
		static Resolver& resolver();
		static void setTimer(const TimeEvent* owner, const size_t timerId, const NanoSecs& timeout);
		static NanoSecs cancelTimer(TimeEvent* owner, const size_t timerId);

		~Engine();
	private:
		static void blockSignals();
		class Worker {
			public:
				Worker(void (func(Worker*) noexcept))
					: thread(func, this) {
				}
				~Worker();
				std::thread thread;
				std::atomic_bool exited;
				Stats stats;
		};
	private:
		Engine();
		void doEpoll(Worker* me) noexcept;
		static void onTimerExpired();
		static void doWork(Worker* me) noexcept;
		static void signalHandler(int signalNum);
		static void initSignals();
		void doStop();
		void doSignalHandler();
		std::shared_ptr<TimeEvent> getEv(const TimeEvent* what);
		void doSetTimer(const TimeEvent* what, const size_t timerId, const NanoSecs& timeout);
		void doOnTimerExpired();
		NanoSecs doCancelTimer(TimeEvent* owner, const size_t timerId);
		void armTimerIn(const NanoSecs& timeout) const;
		void doInit(int minWorkersPerCpu);
		void run(Socket& sock, const uint32_t events) const;
		void doEpoll();
		void worker(Worker& me);
		void doAdd(const std::shared_ptr<Socket>& what, const bool replace);
		void doRemove(const Socket* what, const bool replace);
		void push(TimeEvent& what, uint32_t events);
		void interrupt(Worker& thread) const;
	private:
		class EpollEvent final {
			public:
				explicit EpollEvent(const Socket* what, uint32_t events) : what(what), events(events) { }
				const TimeEvent* epollEvent() const { return what; };
				uint32_t epollEvents() const { return events; };
			private:
				const Socket* what;
				const uint32_t events;
		};

		static Engine* theEngine;
		Semaphore sem;
		SyncVec<EpollEvent> eventQueue;
		std::atomic_bool stopped;
		std::thread::id epollTid;
		std::thread::native_handle_type epollHandle;
		int epollFd = -1;
		int timerFd	= -1;

		std::vector<Worker*> slaves;
		std::unordered_map<const TimeEvent *, std::shared_ptr<TimeEvent>> eventHash;
		std::mutex evHashLock;
		Resolver theResolver;
		Timers timers;
	private:
		const NanoSecs SHUTDOWN_WAIT_CHECK_NSECS = NanoSecs(10000000);
		const int MAX_SHUTDOWN_ATTEMPTS = 5;
		const int EPOLL_EVENTS_PER_RUN = 4;
	};
}
