#include "timeevent.hpp"
#include "engine.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include <sys/epoll.h>
#include <unistd.h>

namespace Sb {
	TimeEvent::TimeEvent() {
	}

	TimeEvent::~TimeEvent() {
	}

	void TimeEvent::setTimer(const int timerId, const NanoSecs& timeout) {
		assert(timeout.count() > 0, " Epoll::setTimer timeout must be > 0");
		logDebug("setTimer() " + intToString(timerId));
		Engine::setTimer(this, timerId, timeout);
	}

	void TimeEvent::cancelTimer(const int timerId) {
		logDebug("Epollable::cancelTimer() " + intToString(timerId));
		Engine::cancelTimer(this, timerId);
	}
	std::shared_ptr<TimeEvent> TimeEvent::ref() {
		return shared_from_this();
	}
}

