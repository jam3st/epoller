#pragma once
#include <vector>
#include <mutex>
#include "utils.hpp"

namespace Sb {
	template<typename T>
	class SyncVec {
		public:
			SyncVec() {
				if(APPROX_PAGE_SIZE > sizeof (SyncVec<T>)) {
					auto res = (APPROX_PAGE_SIZE - sizeof (SyncVec<T>)) / size;
					if(res != 0) {
						vec.reserve (res);
					}
				}
			}

			~SyncVec() {
			}

			bool isEmpty() {
				std::lock_guard<std::mutex> sync(lock);
				return start == vec.size ();
			}

			void add(const T& src) {
				std::lock_guard<std::mutex> sync(lock);
				vec.push_back (src);
			}

			const T& addAndRemove(const T& src) {
				std::lock_guard<std::mutex> sync(lock);
				if(isEmptyUnlocked()) {
					return src;
				}
				vec.push_back (src);
				return vec[start++];
			}

			const T& remove() {
				std::lock_guard<std::mutex> sync(lock);
				return vec[start++];
			}

			const T& removeAndIsEmpty(bool& isEmpty) {
				std::lock_guard<std::mutex> sync(lock);
				isEmpty = isEmptyUnlocked();
				if(isEmpty) {
					return *((T *) nullptr);
				}
				return vec[start++];
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
	};

	template<typename T>
	const size_t SyncVec<T>::size = sizeof(T);
}
