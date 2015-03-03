#pragma once
#include <memory>
#include "socket.hpp"

namespace Sb {
	class UdpSocket;
	class UdpSocketIf {
		public:
			virtual void onError() = 0;
			virtual void onConnect(UdpSocket&, const InetDest&) = 0;
			virtual void onReadCompleted(UdpSocket&, const InetDest&, const Bytes&) = 0;
			virtual void onWriteCompleted(UdpSocket&) = 0;
			virtual void onTimerExpired(UdpSocket&, int) = 0;
	};

	class UdpSocket final : virtual public Socket {
		public:
			static void create(const uint16_t localPort, std::shared_ptr<UdpSocketIf> client);
			static void create(const InetDest& dest, std::shared_ptr<UdpSocketIf> client);
			void queueWrite(const Bytes& data, const InetDest& dest);
			void disconnect();
			virtual ~UdpSocket();
			void handleRead();
			void handleWrite();
			void handleError();
			void handleTimer(const size_t timerId);
			UdpSocket(std::shared_ptr<UdpSocketIf> client);
		private:
			void createStream(const int newFd);
			std::shared_ptr<UdpSocketIf> client;
			InetDest myDest;
	};
}
