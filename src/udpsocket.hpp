#pragma once
#include <atomic>
#include <memory>
#include "socket.hpp"
#include "syncvec.hpp"

namespace Sb {
	class UdpSocket;
	class UdpSocketIf {
		public:
			friend class UdpSocket;
			virtual void connected(const InetDest&) = 0;
			virtual void received(const InetDest&, const Bytes&) = 0;
			virtual void notSent(const InetDest&, const Bytes&) = 0;
			virtual void writeComplete() = 0;
			virtual void disconnected() = 0;
			virtual void timeout(const size_t timerId) = 0;
			virtual const std::shared_ptr<UdpSocket> socket() const {
				return udpSocket;
			}
		private:
			std::shared_ptr<UdpSocket> udpSocket;
	};

	class UdpSocket final : virtual public Socket {
		public:
			static void create(const uint16_t localPort, std::shared_ptr<UdpSocketIf> client);
			static void create(const InetDest& dest, std::shared_ptr<UdpSocketIf> client);
			void queueWrite(const InetDest& dest, const Bytes& data);
			void disconnect();
			virtual ~UdpSocket();
			void handleRead();
			void handleWrite();
			void handleError();
			void handleTimer(const size_t timerId);
			void doWrite(const InetDest& dest, const Bytes& data);
			UdpSocket(std::shared_ptr<UdpSocketIf> client);
		private:
			void createStream(const int newFd);
			std::shared_ptr<UdpSocketIf> client;
			InetDest myDest;
			std::mutex writeLock;
			SyncVec<std::pair<const InetDest, const Bytes>> writeQueue;
	};
}
