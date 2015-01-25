#pragma once
#include <deque>
#include <memory>
#include "semaphore.hpp"
#include <thread>
#include <map>
#include <unordered_map>
#include "syncvec.hpp"
#include "epoll.hpp"
#include "stats.hpp"

namespace Sb {
	class Engine final {
	public:
		static void start(int minWorkersPerCpu = 1);
		static void init();
		static void add(const std::shared_ptr<Epoll>& what);
		static void remove(const Epoll* what);
		static void setTimer(Epoll* owner, const int timerId, const NanoSecs timeout);
		static void cancelTimer(Epoll* owner, const int timerId);

		~Engine();
	private:
		static void blockSignals();
		class Worker {
			public:
				Worker(void (func(Worker*) noexcept))
					: thread(func, this) {
				}
				~Worker();
				Stats stats;
				int id;
				bool exited = false;
				std::thread thread;
		};
	private:
		Engine();
		void doEpoll(Worker* me) noexcept;
		static void onTimerExpired();
		static void doWork(Worker* me) noexcept;
		static void signalHandler(int signalNum);
		static void initSignals();
		std::shared_ptr<Epoll> getEpoll(const Epoll* what);
		void doSetTimer(const Epoll* what, const int timerId, const NanoSecs& timeout);
		void doOnTimerExpired();
		void doCancelTimer(const Epoll* what, const int timerId);
		void doCancelAllTimers(const Epoll* what);
		void armTimerIn(const NanoSecs& timeout) const;
		void disArmTimer() const;
		void stop();
		void doInit(int minWorkersPerCpu);
		void doEpoll();
		void worker(Worker& me);
		void doAdd(const std::shared_ptr<Epoll>& what);
		void doRemove(const Epoll* what);
		void push(Epoll& what, uint32_t events);
		void interrupt(Worker& thread) const;
	private:
		class EpollEvent final {
			public:
				explicit EpollEvent(const Epoll* what, uint32_t events) : what(what), events(events) { }
				const Epoll* epollEvent() const { return what; };
				uint32_t epollEvents() const { return events; };
			private:
				const Epoll* what;
				const uint32_t events;
		};
		struct TimerDateId final {
				TimePointNs tp;
				int id;
		};
		struct TimerEpollId final {
				const Epoll* ep;
				int id;
		};

		static Engine* theEngine;
		Semaphore sem;
		SyncVec<EpollEvent> eventQueue;
		std::atomic_bool stopped;
		std::thread::id epollTid;
		int epollFd = -1;
		int timerFd	= -1;

		std::vector<Worker*> slaves;
		std::unordered_map<const Epoll *, std::shared_ptr<Epoll>> eventHash;
		std::multimap<const TimePointNs, TimerEpollId> timesByDate;
		std::multimap<const Epoll*, TimerDateId> timersByOwner;
		std::mutex evHashLock;
		std::mutex timeLock;
	private:
		const NanoSecs SHUTDOWN_WAIT_CHECK_NSECS = NanoSecs(100000000);
		const int MAX_SHUTDOWN_ATTEMPTS = 5;
		const int EPOLL_EVENTS_PER_RUN = 4;
	};
}
