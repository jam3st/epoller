#include "timeevent.hpp"
#include "engine.hpp"

namespace Sb {
	TimeEvent::TimeEvent() {
	}

	TimeEvent::~TimeEvent() {
	}

	void TimeEvent::setTimer(Timer* const timerId, NanoSecs const& timeout) {
		assert(timeout.count() > 0, " Epoll::setTimer timeout must be > 0");
		logDebug("setTimer()");
		Engine::setTimer(this, timerId, timeout);
	}

	void TimeEvent::cancelTimer(Timer* const timerId) {
		logDebug("Epollable::cancelTimer()");
		Engine::cancelTimer(this, timerId);
	}

	std::shared_ptr<TimeEvent> TimeEvent::ref() {
		return shared_from_this();
	}

	Timer::Timer(std::function<void()> const& func) : func(func) {
	}

	void Timer::operator()() {
		func();
	}
}

