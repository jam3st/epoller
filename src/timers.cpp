#include <algorithm>
#include "timers.hpp"
#include "engine.hpp"

namespace Sb {
      Timers::Timers(std::function<void(Event const* what, NanoSecs const& when)> const armTimer) : armTimer(armTimer) {
      }

      Timers::~Timers() {
      }

      void Timers::handleTimerExpired() {
            std::lock_guard<std::mutex> sync(timeLock);
            assert(timesByDate.size() > 0, "Expired timer not found");
            auto ev = timesByDate.begin()->second.ep;
            auto event = timesByDate.begin()->second.id;
            removeTimer(ev, event);
            setTrigger();
      }

      TimePointNs Timers::removeTimer(Runnable const* const what, Event const* const timer) {
            auto iiByOwner = timersByOwner.equal_range(what);
            TimePointNs prevTp = SteadyClock::now();
            if(iiByOwner.first != iiByOwner.second) {
                  for_each(iiByOwner.first, iiByOwner.second, [&, timer](auto const& bo) {
                        auto iiByDate = timesByDate.equal_range(bo.second.tp);
                        if(iiByDate.first != iiByDate.second) {
                              for(auto it = iiByDate.first; it != iiByDate.second; ++it) {
                                    if(timer == it->second.id) {
                                          timesByDate.erase(it);
                                          break;
                                    }
                              }
                        }
                  });
                  auto it = std::find_if(iiByOwner.first, iiByOwner.second, [&timer](auto const& tp) {
                        return timer == tp.second.id;
                  });
                  if(it != iiByOwner.second) {
                        prevTp = it->second.tp;
                        timersByOwner.erase(it);
                  }
            }
            assert(timesByDate.size() == timersByOwner.size(), "Timers::removeTimer Timer mismatch");
            return prevTp;
      }

      void Timers::setTrigger() {
            if(timesByDate.size() > 0) {
                  auto const trigger = std::max(NanoSecs{1}, NanoSecs {timesByDate.begin()->first - SteadyClock::now()});
                  armTimer(&timesByDate.begin()->second.eventObj, trigger);
            } else {
                  armTimer(nullptr, NanoSecs{0});
            }
      }

      TimerRunnableId Timers::getStartEvent() const {
            if(timesByDate.size() > 0) {
                  return TimerRunnableId{timesByDate.begin()->second.ep, timesByDate.begin()->second.id};
            } else {
                  return TimerRunnableId {nullptr, nullptr};
            }
      }

      NanoSecs Timers::cancelTimer(Event const& timer) {
            std::lock_guard<std::mutex> sync(timeLock);
            auto owner = timer.obj.lock();
            if(owner) {
                  TimerRunnableId oldStartEvent = getStartEvent();
                  auto const timerCount = removeTimer(owner.get(), &timer) - SteadyClock::now();
                  if(oldStartEvent != getStartEvent()) {
                        setTrigger();
                  }
                  return timerCount;
            } else {
                  return NanoSecs{0};
            }
      }

      NanoSecs Timers::setTimer(Event const& timer, const NanoSecs& timeout) {
            std::lock_guard<std::mutex> sync(timeLock);
            auto owner = timer.obj.lock();
            if(owner) {
                  auto now = SteadyClock::now();
                  TimePointNs when = now + timeout;
                  TimerRunnableId oldStartEvent = getStartEvent();
                  auto const timerCount = removeTimer(owner.get(), &timer) - SteadyClock::now();
                  timersByOwner.emplace(owner.get(), TimerDateId {when, &timer});
                  timesByDate.emplace(when, TimerRunnableId {owner.get(), &timer, Event{owner, timer}});
                  assert(timesByDate.size() == timersByOwner.size(), "Timers::setTimer Timer mismatch");
                  if(oldStartEvent != getStartEvent()) {
                        setTrigger();
                  }
                  return timerCount;
            } else {
                  return NanoSecs{0};
            }
      };

      void Timers::cancelAllTimers(Runnable const* const what) {
            std::lock_guard<std::mutex> sync(timeLock);
            TimerRunnableId oldStartEvent = getStartEvent();
            auto iiByOwner = timersByOwner.equal_range(what);
            for_each(iiByOwner.first, iiByOwner.second, [&](auto const& bo) {
                  auto iiByDate = timesByDate.equal_range(bo.second.tp);
                  timesByDate.erase(iiByDate.first, iiByDate.second);
            });
            timersByOwner.erase(iiByOwner.first, iiByOwner.second);
            assert(timesByDate.size() == timersByOwner.size(), "Timers::cancelAllTimers Timer mismatch");
            if(oldStartEvent != getStartEvent()) {
                  setTrigger();
            }
      }

      void Timers::clear() {
            timesByDate.clear();
            timersByOwner.clear();
      }
}
