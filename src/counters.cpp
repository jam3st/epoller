#include "counters.hpp"

namespace Sb {
      Counters::Counters() : start(SteadyClock::now()), lastEgress(SteadyClock::now()), lastIngress(SteadyClock::now()) {
      }

      Counters::~Counters() {
            std::lock_guard<std::mutex> sync(lock);
      }

      void Counters::dumpStats() const {
            auto elapsedNs = Clock::elapsed(start, SteadyClock::now()).count();
            if (elapsedNs <= 0) {
                  elapsedNs = 1;
            }
            logDebug("Elapsed: " + std::to_string(elapsedNs) + " IN " + std::to_string(ingress) + " OUT " + std::to_string(egress));
      };

      void Counters::notifyIngress(ssize_t const count) {
            if (count > 0) {
                  std::lock_guard<std::mutex> sync(lock);
                  lastIngress = SteadyClock::now();
                  ingress += count;
            }
      }

      void Counters::notifyEgress(ssize_t const count) {
            if (count > 0) {
                  std::lock_guard<std::mutex> sync(lock);
                  lastEgress = SteadyClock::now();
                  egress += count;
            }
      }
}
