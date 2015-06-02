#include "semaphore.hpp"

namespace Sb {
      Semaphore::Semaphore(int count) : count{count} {
      }

      void Semaphore::signal() {
            std::lock_guard<std::mutex> lock{mutex};
            ++count;
            cv.notify_one();
      }

      void Semaphore::wait() {
            std::unique_lock<std::mutex> lock{mutex};
            cv.wait(lock, [&] {
                  return count > 0;
            });
            --count;
      }
};
