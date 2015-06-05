#pragma once
#include <map>
#include <functional>
#include "utils.hpp"
#include "types.hpp"
#include "event.hpp"

namespace Sb {
      class Timers final {
      private:
            class TimerDateId;
            class TimerRunnableId;
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
            TimerRunnableId getStartEvent() const;
      private:
            std::multimap<TimePointNs, TimerRunnableId> timesByDate;
            std::multimap<Runnable* const, TimerDateId> timersByOwner;
            std::mutex timeLock;
            const std::function<void(Event const* what, NanoSecs const& when)> armTimer;
      };

      class Timers::TimerDateId final {
      public:
            TimePointNs const tp;
            Event* const id;
      };
      class Timers::TimerRunnableId final {
      public:
            Runnable* ep;
            Event* id;
            bool operator==(TimerRunnableId const& rhs) {
                  return ep == rhs.ep && id == rhs.id;
            }
            bool operator!=(TimerRunnableId const& rhs) {
                  return ep != rhs.ep || id != rhs.id;
            }
      };

}
