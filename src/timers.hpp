#pragma once
#include <map>
#include <unordered_map>
#include <functional>
#include "utils.hpp"
#include "types.hpp"
#include "event.hpp"

namespace Sb {
      class TimerDateId final {
      public:
            TimePointNs const tp;
            Event const* const id;
      };

      class TimerRunnableId final {
      public:
            Runnable const* ep;
            Event const* const id;
            Event eventObj;

            bool operator==(TimerRunnableId const& rhs) {
                  return ep == rhs.ep && id == rhs.id;
            }

            bool operator!=(TimerRunnableId const& rhs) {
                  return ep != rhs.ep || id != rhs.id;
            }
      };

      class Timers final {
      public:
            Timers() = delete;
            explicit Timers(std::function<void(Event const* what, NanoSecs const& when)> const armTimer);
            ~Timers();
            void cancelAllTimers(Runnable const* const what);
            NanoSecs setTimer(Event const& timer, NanoSecs const& timeout);
            NanoSecs cancelTimer(Event const& timer);
            void handleTimerExpired();
            void clear();
      private:
            void setTrigger();
            TimePointNs removeTimer(Runnable const* const what, Event const* const timer);
            TimerRunnableId getStartEvent() const;
      private:
            std::map<TimePointNs, TimerRunnableId> timesByDate;
            std::unordered_multimap<Runnable const*, TimerDateId> timersByOwner;
            std::mutex timeLock;
            const std::function<void(Event const* const what, NanoSecs const& when)> armTimer;
      };
}
