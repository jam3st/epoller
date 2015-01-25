#pragma once
#include "engine.hpp"
#include "socket.hpp"
#include "types.hpp"

namespace Sb {
	class UdpServer final : public Socket  {
		public:
			UdpServer(const uint16_t port);
			virtual ~UdpServer();
			void handleRead();
			void handleWrite();
			void handleError();
			void handleTimer(int timerId);
		private:
	};
}
