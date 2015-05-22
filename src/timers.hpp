#pragma once
#include <map>
#include <functional>
#include "utils.hpp"
#include "types.hpp"
#include "event.hpp"

namespace Sb {
      class Timers {
            public:
                  Timers(const std::function<void(NanoSecs const& when)> armTimer);
                  ~Timers();
                  std::pair<Runnable * const, Event * const> onTimerExpired();
                  void cancelAllTimers(Runnable * const what);
                  NanoSecs setTimer(Runnable * const what, Event * const timerId, NanoSecs const& timeout);
                  NanoSecs cancelTimer(Runnable * const what, Event * const timerId);
            private:
                  void setTrigger();
                  TimePointNs removeTimer(Runnable * const what, Event * const timerId);
            private:
                  struct TimerDateId final {
                        TimePointNs tp;
                        Event * const id;
                  };
                  struct TimerEpollId final {
                        Runnable * const ep;
                        Event * const id;
                  };
            private:
                  std::multimap<TimePointNs, TimerEpollId> timesByDate;
                  std::multimap<Runnable * const, TimerDateId> timersByOwner;
                  std::mutex timeLock;
                  const std::function<void(const NanoSecs& when)> armTimer;
      };
}
