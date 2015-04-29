#pragma once
#include <cstdint>
#include <chrono>

namespace Sb {
	typedef std::chrono::nanoseconds NanoSecs;
	typedef std::chrono::seconds Seconds;
	typedef std::chrono::steady_clock SteadyClock;
	typedef std::chrono::time_point<SteadyClock, NanoSecs> TimePointNs;

	 TimePointNs const zeroTimePoint { std::chrono::duration_cast<NanoSecs>(NanoSecs{ 0 }) };
	constexpr int64_t NanoSecsInSecs { 1'000'000'000 };
	namespace Clock {
		uint64_t now();
	}

//	class Something {
//		int64_t nowNs = Clock::now();
//		std::time_t secs;
//	};
}
