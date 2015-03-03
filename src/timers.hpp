#pragma once
#include <map>
#include <functional>
#include "utils.hpp"
#include "types.hpp"
#include "timeevent.hpp"

namespace Sb {
	class Timers
	{
		public:
			Timers(const std::function<void (const NanoSecs& when)> armTimer);
			~Timers();

			std::pair<const TimeEvent*, const size_t> onTimerExpired();
			void cancelAllTimers(const TimeEvent* what);
			NanoSecs setTimer(const TimeEvent* what, const size_t timerId, const NanoSecs& timeout);
			NanoSecs cancelTimer(const TimeEvent* what, const size_t timerId);
		private:
			void setTrigger();
			TimePointNs removeTimer(const TimeEvent* what, const size_t timerId);
		private:
			struct TimerDateId final {
					TimePointNs tp;
					size_t id;
			};
			struct TimerEpollId final {
					const TimeEvent* ep;
					size_t id;
			};
		private:
			std::multimap<TimePointNs, const TimerEpollId> timesByDate;
			std::multimap<const TimeEvent*, const TimerDateId> timersByOwner;
			std::mutex timeLock;
			const std::function<void (const NanoSecs& when)> armTimer;
	};
}
