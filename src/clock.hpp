#pragma once
#include <cstdint>
#include <chrono>
#include <functional>
#include "constants.hpp"

namespace Sb {
      typedef std::chrono::nanoseconds NanoSecs;
      typedef std::chrono::seconds Seconds;
      typedef std::chrono::steady_clock SteadyClock;
      typedef std::chrono::time_point<SteadyClock, NanoSecs> TimePointNs;
      TimePointNs const zeroTimePoint{std::chrono::duration_cast<NanoSecs>(NanoSecs {0})};
      constexpr int64_t NanoSecsInSecs{ONE_SEC_IN_NS};
      namespace Clock {
            int64_t now();
            template<typename T, typename U>
            inline auto elapsed(T const& start, U const& end) -> decltype(end - start) const {
                  return end - start;
            }
      }
      struct HashTimePointNs {
            size_t operator()(TimePointNs const& tp) const {
                  return std::hash<int64_t>()(Clock::elapsed(zeroTimePoint, tp).count());
            }
      };
}
