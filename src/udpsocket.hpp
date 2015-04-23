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
		protected:
			std::weak_ptr<UdpSocket> udpSocket;
	};

	class UdpSocket final : virtual public Socket {
		public:
			static void create(const uint16_t localPort, std::shared_ptr<UdpSocketIf> client);
			static void create(const InetDest& dest, std::shared_ptr<UdpSocketIf> client);
			void queueWrite(const InetDest& dest, const Bytes& data);
			void disconnect();
			virtual ~UdpSocket();
			virtual void handleRead() override;
			virtual void handleWrite() override;
			virtual void handleError() override;
			virtual void handleTimer(const size_t timerId) override;
			void doWrite(const InetDest& dest, const Bytes& data);
			UdpSocket(std::shared_ptr<UdpSocketIf> client);
		private:
			std::shared_ptr<UdpSocketIf> client;
//			InetDest myDest;
			std::mutex writeLock;
			std::mutex readLock;
//			bool waitingWriteEvent;
			SyncVec<std::pair<const InetDest, const Bytes>> writeQueue;
	};
}
