#pragma once
#include <memory>
#include "socket.hpp"

namespace Sb {
	class UdpClient final : virtual public Socket {
		public:
			virtual ~UdpClient();
			void handleRead();
			void handleWrite();
			void handleError();
			void handleTimer(int timerId);
			UdpClient(const std::string& address, const uint16_t port, const Bytes& initialData);
		private:
			void createStream(const int newFd);
			InetDest dest;
	};
}
