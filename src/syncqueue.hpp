#pragma once
#include <deque>
#include <mutex>
#include <utility>
#include "utils.hpp"

namespace Sb {
      template<typename T> class SyncQueue {
            public:
                  SyncQueue(T sentinel) : sentinel(sentinel) {
                  }

                  ~SyncQueue() {
                  }

                  void addLast(T src) {
                        std::lock_guard<std::mutex> sync(lock);
                        queue.push_back(src);
                  }

                  void addFirst(T src) {
                        std::lock_guard<std::mutex> sync(lock);
                        queue.push_front(src);
                  }

            std::tuple<T, bool> removeAndIsEmpty() {
                        std::lock_guard<std::mutex> sync(lock);

                        if(queue.empty()) {
                              return std::make_tuple(sentinel, true);
                        } else {
                              auto ret = std::make_tuple(queue.front(), false);
                              queue.pop_front();
                              return ret;
                        }
                  }

                  size_t len()  {
                        std::lock_guard<std::mutex> sync(lock);
                        return queue.size();
                  }

            private:
                  std::mutex lock;
                  std::deque<T> queue;
                  size_t start = 0;
                  static const size_t size;
                  const T sentinel;
      };

      template<typename T> const size_t SyncQueue<T>::size = sizeof(T);
}
