#pragma once
#include <map>
#include <functional>
#include "utils.hpp"
#include "types.hpp"
#include "timeevent.hpp"

namespace Sb {
	class Timers {
	public:
		Timers(const std::function<void(NanoSecs const& when)> armTimer);
		~Timers();
		std::pair<TimeEvent* const, Timer* const> onTimerExpired();
		void cancelAllTimers(TimeEvent* const what);
		NanoSecs setTimer(TimeEvent* const what, Timer* const timerId, NanoSecs const& timeout);
		NanoSecs cancelTimer(TimeEvent* const what, Timer* const timerId);
	private:
		void setTrigger();
		TimePointNs removeTimer(TimeEvent* const what, Timer* const timerId);
	private:
		struct TimerDateId final {
			TimePointNs tp;
			Timer* const id;
		};
		struct TimerEpollId final {
			TimeEvent* const ep;
			Timer* const id;
		};
	private:
		std::multimap<TimePointNs, TimerEpollId> timesByDate;
		std::multimap<TimeEvent* const, TimerDateId> timersByOwner;
		std::mutex timeLock;
		const std::function<void(const NanoSecs& when)> armTimer;
	};
}
