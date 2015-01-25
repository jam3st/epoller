#pragma once
#include <memory>
#include "socket.hpp"
#include "tcpstream.hpp"

namespace Sb {
	class TcpListener final : virtual public Socket {
		public:
			static void create(const uint16_t port,
													 std::function<std::shared_ptr<TcpSockIface>()> clientFactory);
			virtual ~TcpListener();
			void handleRead();
			void handleWrite();
			void handleError();
			void handleTimer(int timerId);
			TcpListener(const uint16_t port, std::function<std::shared_ptr<TcpSockIface>()> clientFactory);
		private:
			void createStream(const int newFd);
			std::function<std::shared_ptr<TcpSockIface>()> clientFactory;
	};
}
