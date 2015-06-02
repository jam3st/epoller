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

      TimePointNs Timers::removeTimer(Runnable* const what, Event* const timer) {
            auto iiByOwner = timersByOwner.equal_range(what);
            TimePointNs prevTp = SteadyClock::now();
            if (iiByOwner.first != iiByOwner.second) {
                  for_each(iiByOwner.first, iiByOwner.second, [&, timer](const auto&bo) {
                        auto iiByDate = timesByDate.equal_range(bo.second.tp);
                        if (iiByDate.first != iiByDate.second) {
                              for (auto it = iiByDate.first; it != iiByDate.second; ++it) {
                                    if (timer == it->second.id) {
                                          timesByDate.erase(it);
                                          break;
                                    }
                              }
                        }
                  });
                  auto it = std::find_if(iiByOwner.first, iiByOwner.second, [&timer](const auto&tp) {
                        return timer == tp.second.id;
                  });
                  if (it != iiByOwner.second) {
                        prevTp = it->second.tp;
                        timersByOwner.erase(it);
                  }
            }
            return prevTp;
      }

      void Timers::setTrigger() {
            if (timesByDate.size() > 0) {
                  auto const trigger = std::max(NanoSecs{1}, NanoSecs {timesByDate.begin()->first - SteadyClock::now()});
                  armTimer(timesByDate.begin()->second.id, trigger);
            } else {
                  armTimer(nullptr, NanoSecs{0});
            }
      }

      NanoSecs Timers::cancelTimer(Runnable* const what, Event* const timer) {
            std::lock_guard<std::mutex> sync(timeLock);
            auto oldStart = timesByDate.begin();
            auto timerCount = removeTimer(what, timer) - SteadyClock::now();
            if (oldStart != timesByDate.begin()) {
                  setTrigger();
            }
            return timerCount;
      }

      NanoSecs Timers::setTimer(Runnable* const what, Event* const timer, const NanoSecs&timeout) {
            auto now = SteadyClock::now();
            TimePointNs when = now + timeout;
            std::lock_guard<std::mutex> lock(timeLock);
            struct {
                  TimePointNs st;
                  Runnable* ep;
                  Event* id;
            } oldStart{zeroTimePoint, nullptr, nullptr}, newStart{zeroTimePoint, nullptr, nullptr};
            if (timesByDate.begin() != timesByDate.end()) {
                  oldStart = {timesByDate.begin()->first, timesByDate.begin()->second.ep, timesByDate.begin()->second.id};
            }
            auto timerCount = removeTimer(what, timer) - SteadyClock::now();
            timersByOwner.insert(std::make_pair(what, TimerDateId {when, timer}));
            timesByDate.insert(std::make_pair(when, TimerEpollId {what, timer}));
            newStart = {timesByDate.begin()->first, timesByDate.begin()->second.ep, timesByDate.begin()->second.id};
            if (oldStart.id != newStart.id || oldStart.ep != newStart.ep || oldStart.st != newStart.st) {
                  setTrigger();
            }
            return timerCount;
      };

      void Timers::cancelAllTimers(Runnable* const what) {
            std::lock_guard<std::mutex> sync(timeLock);
            auto oldStart = timesByDate.begin();
            auto iiByOwner = timersByOwner.equal_range(what);
            for_each(iiByOwner.first, iiByOwner.second, [&](const auto&bo) {
                  auto iiByDate = timesByDate.equal_range(bo.second.tp);
                  timesByDate.erase(iiByDate.first, iiByDate.second);
            });
            timersByOwner.erase(iiByOwner.first, iiByOwner.second);
            if (timesByDate.begin() == timesByDate.end() || oldStart != timesByDate.begin()) {
                  setTrigger();
            }
      }

      void Timers::clear() {
            std::lock_guard<std::mutex> sync(timeLock);
            timesByDate.clear();
            timersByOwner.clear();
      }
}
