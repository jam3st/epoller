#pragma once
#include <map>
#include <functional>
#include "utils.hpp"
#include "types.hpp"
#include "event.hpp"

namespace Sb {
      class Timers final {
      public:
            Timers() = delete;
            explicit Timers(std::function<void(Event const* what, NanoSecs const& when)> const armTimer);
            ~Timers();
            void cancelAllTimers(Runnable* const what);
            NanoSecs setTimer(Runnable* const what, Event* const timer, NanoSecs const&timeout);
            NanoSecs cancelTimer(Runnable* const what, Event* const timer);
            void handleTimerExpired();
            void clear();
      private:
            void setTrigger();
            TimePointNs removeTimer(Runnable* const what, Event* const timer);
      private:
            struct TimerDateId final {
                  TimePointNs tp;
                  Event* const id;
            };
            struct TimerEpollId final {
                  Runnable* const ep;
                  Event* const id;
            };
      private:
            std::multimap<TimePointNs, TimerEpollId> timesByDate;
            std::multimap<Runnable* const, TimerDateId> timersByOwner;
            std::mutex timeLock;
            const std::function<void(Event const* what, NanoSecs const& when)> armTimer;
      };
}
