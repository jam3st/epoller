#pragma once
#include <cstdint>
#include <chrono>
#include "constants.hpp"

namespace Sb {
      typedef std::chrono::nanoseconds NanoSecs;
      typedef std::chrono::seconds Seconds;
      typedef std::chrono::steady_clock SteadyClock;
      typedef std::chrono::time_point<SteadyClock, NanoSecs> TimePointNs;
      TimePointNs const zeroTimePoint{std::chrono::duration_cast<NanoSecs>(NanoSecs {0})};
      constexpr int64_t NanoSecsInSecs{ONE_SEC_IN_NS};
      namespace Clock {
            uint64_t now();
      }
}
