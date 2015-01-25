#pragma once
#include <cstdint>
#include <memory>
#include "utils.hpp"

namespace Sb {
	class Epoll : public std::enable_shared_from_this<Epoll> {
		public:
			explicit Epoll(const int fd);
			virtual ~Epoll();
			virtual void setTimer(const int timerId, const NanoSecs& timeout) final;
			virtual void cancelTimer(const int timerId) final;
		protected:
			virtual void handleError() = 0;
			virtual void handleRead() = 0;
			virtual void handleWrite() = 0;
			virtual void handleTimer(int timerId) = 0;
			virtual std::shared_ptr<Epoll> ref() final;
			virtual int getFd() const final;
			virtual int dupFd() const final;
		private:
			friend class Engine;
			virtual void run(const uint32_t events) final;
			virtual void timeout(const int timerId) final;
		protected:
			const int fd;
	};
}
