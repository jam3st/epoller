#pragma once
#include "engine.hpp"
#include "socket.hpp"
#include "types.hpp"

namespace Sb {
	class UdpServer;
	class UdpDispatchIf {
		public:
			virtual void onRecvFrom(UdpServer&, const Bytes&) = 0;
			virtual void onWriteComplted(UdpServer&) = 0;
			virtual void onError(UdpServer&) = 0;
			virtual void onTimerExpired(UdpServer&, int) = 0;
	};

	class UdpServer final : public Socket  {
		public:
			static void create(const uint16_t port, std::shared_ptr<UdpDispatchIf> dispatcher);
			UdpServer(const uint16_t port, std::shared_ptr<UdpDispatchIf> dispatcher);
			virtual ~UdpServer();
			void handleRead();
			void handleWrite();
			void handleError();
			void handleTimer(const size_t timerId);
		private:
			std::shared_ptr<UdpDispatchIf> dispatcher;
	};
}
