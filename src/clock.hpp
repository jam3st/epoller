#pragma once
#include <cstdint>
#include <chrono>

namespace Sb {
	typedef std::chrono::nanoseconds NanoSecs;
	typedef std::chrono::seconds Seconds;
	typedef std::chrono::steady_clock SteadyClock;
	typedef std::chrono::time_point<SteadyClock, NanoSecs> TimePointNs;
	const int64_t NanoSecsInSecs = { 1'000'000'000 };

	namespace Clock {
		uint64_t now();
	}
}
