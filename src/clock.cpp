#include <chrono>
#include "clock.hpp"

namespace Sb {
      namespace Clock {
            static int64_t driftCorrectionInNs = std::chrono::duration_cast <NanoSecs> (std::chrono::system_clock::now().time_since_epoch() -
                                                 std::chrono::steady_clock::now().time_since_epoch()).count();
            int64_t now() {
                  return std::chrono::duration_cast <NanoSecs> (SteadyClock::now().time_since_epoch()).count() + driftCorrectionInNs;
            }
      }
}
