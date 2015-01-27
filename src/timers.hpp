#pragma once
#include <map>
#include "utils.hpp"
#include "types.hpp"
#include "timeevent.hpp"

namespace Sb {
	class Timers
	{
		public:
			Timers();
			~Timers();

			std::pair<const TimeEvent*, const size_t> onTimerExpired();
			void cancelAllTimers(const TimeEvent* what);
			NanoSecs setTimer(const TimeEvent* what, const size_t timerId, const NanoSecs& timeout);
			NanoSecs cancelTimer(const TimeEvent* what, const size_t timerId);
			NanoSecs getTrigger() const;
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
			std::multimap<const TimePointNs, TimerEpollId> timesByDate;
			std::multimap<const TimeEvent*, TimerDateId> timersByOwner;
			std::mutex timeLock;
			std::atomic<NanoSecs>trigger;
	};
}
