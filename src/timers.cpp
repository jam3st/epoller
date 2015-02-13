#include "timers.hpp"
#include <algorithm>

namespace Sb {
	Timers::Timers()
		: trigger(NanoSecs{ 0 }) {
	}

	Timers::~Timers(){
	}

	std::pair<const TimeEvent*, const size_t> Timers::onTimerExpired() {
		logDebug("doOnTimerExpired() BO: " +
				 std::to_string(timersByOwner.size()) + " BD: " +
				 std::to_string(timesByDate.size()));
		std::lock_guard<std::mutex> lock(timeLock);
		auto ev = timesByDate.begin()->second.ep;
		auto timerId = timesByDate.begin()->second.id;
		removeTimer(ev, timerId);
		setTrigger();
		return std::make_pair(ev, timerId);
	}

	TimePointNs Timers::removeTimer(const TimeEvent* what, const size_t timerId) {
		auto iiByOwner = timersByOwner.equal_range(what);
		TimePointNs prevTp = SteadyClock::now();
		if(iiByOwner.first != iiByOwner.second) {
			for_each(iiByOwner.first, iiByOwner.second, [&, timerId](const auto& bo) {
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
			auto it = std::find_if(iiByOwner.first, iiByOwner.second, [&timerId](const auto& tp) {
				return timerId == tp.second.id;
			} );

			if(it != iiByOwner.second) {
				prevTp = it->second.tp;
				timersByOwner.erase(it);
			}
		}
		return prevTp;
	}

	NanoSecs Timers::getTrigger() const {
		return trigger;
	}

	void Timers::setTrigger() {
		if(timesByDate.size() > 0) {
			trigger = std::max(NanoSecs{ 1 }, NanoSecs{ timesByDate.begin()->first -  SteadyClock::now() });
		} else {
			trigger = NanoSecs{ 0 };
		}
	}

	NanoSecs Timers::cancelTimer(const TimeEvent* what, const size_t timerId) {
		logDebug("Engine::doCancelTimer() for " + std::to_string(timerId));
		logDebug("Engine::doCancelTimer() BO: " +
				 std::to_string(timersByOwner.size()) + " BD: " +
				 std::to_string(timesByDate.size()));
		std::lock_guard<std::mutex> sync(timeLock);
		auto oldStart = timesByDate.begin();
		auto timerCount = removeTimer(what, timerId) - SteadyClock::now();
		logDebug("Engine::doCancelTimer() BO: " +
				 std::to_string(timersByOwner.size()) + " BD: " +
				 std::to_string(timesByDate.size()));
		if(oldStart != timesByDate.begin()) {
			setTrigger();
		}
		return timerCount;
	}

	NanoSecs Timers::setTimer(const TimeEvent* what, const size_t timerId, const NanoSecs& timeout) {
		logDebug("Engine::doSetTimer() BO: " +
				 std::to_string(timersByOwner.size()) + " BD: " +
				 std::to_string(timesByDate.size()));
		logDebug("Timer::doSetTimer() for " + std::to_string(timerId) + " duration "
				 + std::to_string(timeout.count()) + "ns");

		auto now = SteadyClock::now();
		TimePointNs when = now + timeout;
		std::lock_guard<std::mutex> lock(timeLock);
		auto oldStart = timesByDate.begin();
		auto timerCount = removeTimer(what, timerId) - SteadyClock::now();
		logDebug("Togo is " + std::to_string(timerCount.count()));
		logDebug("Engine::doSetTimer() remove  BO: " +
				 std::to_string(timersByOwner.size()) + " BD: " +
				 std::to_string(timesByDate.size()));
		timersByOwner.insert(std::make_pair(what,  TimerDateId { when, timerId }));
		timesByDate.insert(std::make_pair(when, TimerEpollId { what, timerId }));
		if(oldStart != timesByDate.begin()) {
			setTrigger();
		}
		logDebug("Engine::doSetTimer() BO: " +
				 std::to_string(timersByOwner.size()) + " BD: " +
				 std::to_string(timesByDate.size()));
		return timerCount;
	}

	void Timers::cancelAllTimers(const TimeEvent* what) {
		logDebug("Engine::doCancelAllTimers() BO: " +
				 std::to_string(timersByOwner.size()) + " BD: " +
				 std::to_string(timesByDate.size()));
		std::lock_guard<std::mutex> sync(timeLock);
		auto oldStart = timesByDate.begin();
		auto iiByOwner = timersByOwner.equal_range(what);
		for_each(iiByOwner.first, iiByOwner.second, [&](const auto& bo) {
			 auto iiByDate = timesByDate.equal_range(bo.second.tp);
			 timesByDate.erase(iiByDate.first, iiByDate.second);
		});
		timersByOwner.erase(iiByOwner.first, iiByOwner.second);
		if(oldStart != timesByDate.begin()) {
			setTrigger();
		}
		logDebug("Engine::doCancelAllTimers() BO: " +
				 std::to_string(timersByOwner.size()) + " BD: "+
				 std::to_string(timesByDate.size()));
	}
}
