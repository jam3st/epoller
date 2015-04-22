#pragma once
#include <vector>
#include <mutex>
#include <utility>
#include "utils.hpp"
#include "resouces.hpp"

namespace Sb {
	template<typename T>
	class SyncVec {
		public:
			SyncVec(T sentinel)
				: sentinel(sentinel) {
				if(Resouces::pageSize() > sizeof (SyncVec<T>)) {
					auto res = (Resouces::pageSize() - sizeof (SyncVec<T>)) / size;
					if(res != 0) {
						vec.reserve (res);
					}
				}
			}

			~SyncVec() {
			}

			void add(T src) {
				std::lock_guard<std::mutex> sync(lock);
				vec.push_back(src);
			}

			std::pair<T, bool>
			removeAndIsEmpty() {
				std::lock_guard<std::mutex> sync(lock);
				if(isEmptyUnlocked()) {
					return std::make_pair(sentinel, true);
				} else {
					return std::make_pair(vec[start++], false);
				}
			}

			int len() {
				std::lock_guard<std::mutex> sync(lock);
				return vec.size () - start;
			}
		private:
			bool isEmptyUnlocked() {
				if(start == vec.size()) {
					if(start != 0) {
						vec.clear();
						start = 0;
					}
					return true;
				}
				return false;
			}

		private:
			std::mutex lock;
			std::vector<T> vec;
			size_t start = 0;
			static const size_t size;
			const T sentinel;
	};

	template<typename T>
	const size_t SyncVec<T>::size = sizeof(T);
}
