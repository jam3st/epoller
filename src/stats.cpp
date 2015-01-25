#include "stats.hpp"


namespace Sb {
	template<typename T, typename U>
	inline auto elapsed(const T& end, const U& start) -> decltype(end - start) {
		return end - start;
	}

	Stats::Stats() :
		creation(HighResClock::now()),
		mark(HighResClock::now()),
		idleNs(0),
		busyNs(0),
		counter(0),
		active(true) {
	}

	void Stats::MarkIdleStart() {
		assert(active, "Already active.");
		active = false;
		auto now = HighResClock::now();
		idleNs += elapsed(now, mark);
		mark = now;
	}

	void Stats::MarkIdleEnd() {
		assert(!active, "Not activated yet.");
		active = true;
		auto now = HighResClock::now();
		busyNs += elapsed(now, mark);
		mark = now;
	}

	NanoSecs Stats::GetBusyTimeNs() const {
		auto busyTime = active ? elapsed(HighResClock::now(), mark) : NanoSecs(0);

		if(busyTime < NanoSecs(0)) {
			logWarn("Please check clocks. Possible correction as busytime is "
					 + intToString(busyTime.count())
					 +"ns");
		}
		return busyTime;
	}
}
