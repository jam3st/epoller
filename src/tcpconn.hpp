#pragma once
#include <string>
#include "engine.hpp"
#include "socket.hpp"
#include "types.hpp"
#include "tcpstream.hpp"

namespace Sb {
	class TcpConn final : virtual public Socket {
		public:
			static void create(const std::string&address, const uint16_t port,
													 std::function<std::shared_ptr<TcpSockIface>()> clientFactory);
			TcpConn(const std::string&address, const uint16_t port,
					std::function<std::shared_ptr<TcpSockIface>()> clientFactory);
			virtual ~TcpConn();
			void handleRead();
			void handleWrite();
			void handleError();
			void handleTimer(const size_t timerId);
		private:
			std::function<std::shared_ptr<TcpSockIface>()> clientFactory;
	};
}
