#include "counters.hpp"

namespace Sb{
      template<typename T, typename U>
      inline auto elapsed(T const& start, U const& end) -> decltype(end - start) const {
            return end - start;
      }

      Counters::Counters() : start(SteadyClock::now()),
                       lastEgress(SteadyClock::now()),
                       lastIngress(SteadyClock::now()) {
      };

      Counters::~Counters() {
            std::lock_guard<std::mutex> sync(lock);
            auto end = SteadyClock::now();
            auto elapsedNs = elapsed(start, end).count();
            if(egress <= 675 || ingress <= 675 || elapsedNs > 30'000'000'000 ) {
                  dumpStats();
            }


      }

      void Counters::dumpStats() const {
            auto end = SteadyClock::now();
            auto elapsedNs = elapsed(start, end).count();
            if(elapsedNs <= 0) {
                  elapsedNs = 1;
            }
//            double elapsed = static_cast<double>(elapsedNs);
//            double ingressMb = static_cast<double>(ingress) / 1024.0 / 1024.0;
//            double egressMb = static_cast<double>(egress) / 1024.0 / 1024.0;
//            double ingessRate = ingressMb / elapsed / 1.0e9;
//            double egressRate = egressMb / elapsed / 1.0e9;
             logDebug("Elapsed: " + std::to_string(elapsedNs) + " IN " + std::to_string(ingress) +  " OUT " +  std::to_string(egress));
      };

      void Counters::notifyIngress(ssize_t count) {
            if(count > 0) {
                  std::lock_guard<std::mutex> sync(lock);
                  lastIngress = SteadyClock::now();
                  ingress += count;
            }
      }
      void Counters::notifyEgress(ssize_t count) {
            if(count > 0) {
                  std::lock_guard<std::mutex> sync(lock);
                  lastEgress = SteadyClock::now();
                  egress += count;
            }
      }

}
