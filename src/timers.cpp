#include <algorithm>
#include "timers.hpp"
#include "engine.hpp"

namespace Sb {
      Timers::Timers(const std::function<void(const NanoSecs& when)> armTimer) : armTimer(armTimer) {
      }

      Timers::~Timers() {
      }

      std::pair<Runnable * const, Event * const> Timers::onTimerExpired() {
            std::lock_guard<std::mutex> lock(timeLock);
            auto ev = timesByDate.begin()->second.ep;
            auto timerId = timesByDate.begin()->second.id;
            removeTimer(ev, timerId);
            setTrigger();
            return std::make_pair(ev, timerId);
      }

      TimePointNs Timers::removeTimer(Runnable * const what, Event * const timerId) {
            auto iiByOwner = timersByOwner.equal_range(what);
            TimePointNs prevTp = SteadyClock::now();

            if(iiByOwner.first != iiByOwner.second) {
                  for_each(iiByOwner.first, iiByOwner.second, [&, timerId ](const auto & bo) {
                        auto iiByDate = timesByDate.equal_range(bo.second.tp);

                        if(iiByDate.first != iiByDate.second) {
                              for(auto it = iiByDate.first; it != iiByDate.second; ++it) {
                                    if(timerId == it->second.id) {
                                          timesByDate.erase(it);
                                          break;
                                    }
                              }
                        }
                  });
                  auto it = std::find_if(iiByOwner.first, iiByOwner.second, [ &timerId ](const auto & tp) {
                        return timerId == tp.second.id;
                  });

                  if(it != iiByOwner.second) {
                        prevTp = it->second.tp;
                        timersByOwner.erase(it);
                  }
            }
            return prevTp;
      }

      void Timers::setTrigger() {
            auto trigger = NanoSecs { 0 };

            if(timesByDate.size() > 0) {
                  trigger = std::max(NanoSecs { 1 }, NanoSecs { timesByDate.begin()->first - SteadyClock::now() });
            }

            armTimer(trigger);
      }

      NanoSecs Timers::cancelTimer(Runnable * const what, Event * const timerId) {
            std::lock_guard<std::mutex> sync(timeLock);
            auto oldStart = timesByDate.begin();
            auto timerCount = removeTimer(what, timerId) - SteadyClock::now();

            if(oldStart != timesByDate.begin()) {
                  setTrigger();
            }

            return timerCount;
      }

      NanoSecs Timers::setTimer(Runnable * const what, Event * const timerId, const NanoSecs& timeout) {
            auto now = SteadyClock::now();
            TimePointNs when = now + timeout;
            std::lock_guard<std::mutex> lock(timeLock);
            struct {
                  TimePointNs st;
                  Runnable *  ep;
                  Event * id;
            } oldStart  {
                  zeroTimePoint, nullptr, nullptr
            }, newStart {
                  zeroTimePoint, nullptr, nullptr
            };

            if(timesByDate.begin() != timesByDate.end()) {
                  oldStart = { timesByDate.begin()->first, timesByDate.begin()->second.ep, timesByDate.begin()->second.id };
            }

            auto timerCount = removeTimer(what, timerId) - SteadyClock::now();
            timersByOwner.insert(std::make_pair(what, TimerDateId { when, timerId }));
            timesByDate.insert(std::make_pair(when, TimerEpollId { what, timerId }));
            newStart = { timesByDate.begin()->first, timesByDate.begin()->second.ep, timesByDate.begin()->second.id };

            if(oldStart.id != newStart.id || oldStart.ep != newStart.ep || oldStart.st != newStart.st) {
                  setTrigger();
            }
            return timerCount;
      };

      void Timers::cancelAllTimers(Runnable * const what) {
            std::lock_guard<std::mutex> sync(timeLock);
            auto oldStart = timesByDate.begin();
            auto iiByOwner = timersByOwner.equal_range(what);
            for_each(iiByOwner.first, iiByOwner.second, [ & ](const auto & bo) {
                  auto iiByDate = timesByDate.equal_range(bo.second.tp);
                  timesByDate.erase(iiByDate.first, iiByDate.second);
            });
            timersByOwner.erase(iiByOwner.first, iiByOwner.second);

            if(timesByDate.begin() == timesByDate.end() || oldStart != timesByDate.begin()) {
                  setTrigger();
            }
      }
}
