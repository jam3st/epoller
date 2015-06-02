#pragma once
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace Sb {
      class Semaphore final {
      public:
            void signal();
            void wait();
            explicit Semaphore(int count = 0);
            Semaphore(const Semaphore&) = delete;
            Semaphore(Semaphore&&) = delete;
            Semaphore&operator=(const Semaphore&) = delete;
            Semaphore&operator=(Semaphore&&) = delete;
      private:
            int count;
            std::mutex mutex;
            std::condition_variable cv;
      };
};
