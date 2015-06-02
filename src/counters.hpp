#pragma once
#include "utils.hpp"
#include "clock.hpp"

namespace Sb {
      class Counters final {
      public:
            Counters();
            ~Counters();
            void notifyIngress(ssize_t count);
            void notifyEgress(ssize_t count);
            void dumpStats() const;
            ssize_t getIngress() const {
                  return ingress;
            }

            ssize_t getEgress() const {
                  return egress;
            }
      private:
            std::mutex lock;
            TimePointNs start;
            TimePointNs lastEgress;
            TimePointNs lastIngress;
            ssize_t ingress = 0;
            ssize_t egress = 0;
      };
}
