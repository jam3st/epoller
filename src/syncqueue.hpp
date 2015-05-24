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

                  void clear() {
                        std::lock_guard<std::mutex> sync(lock);
                        queue.clear();
                  }

                  void addLast(T src) {
                        std::lock_guard<std::mutex> sync(lock);
                        queue.push_back(src);
                  }

                  void addFirst(T src) {
                        std::lock_guard<std::mutex> sync(lock);
                        queue.push_front(src);
                  }

                  void removeAll(T* src) {
                        std::lock_guard<std::mutex> sync(lock);
                        auto it = queue.begin();
                        while(it != queue.end()) {
                              if(*it == *src) {
                                    queue.erase(it);
                                    it = queue.begin();
                              } else {
                                    ++it;
                              }
                        }
                  }

                  std::pair<T, bool> removeAndIsEmpty() {
                        std::lock_guard<std::mutex> sync(lock);

                        if(queue.empty()) {
                              if(!shrunk) {
                                    shrunk = true;
                                    queue.shrink_to_fit();
                              }
                              return std::make_pair(sentinel, true);
                        } else {
                              shrunk = false;
                              auto ret = std::make_pair(queue.front(), false);
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
                  bool shrunk = true;
      };

      template<typename T> const size_t SyncQueue<T>::size = sizeof(T);
}
