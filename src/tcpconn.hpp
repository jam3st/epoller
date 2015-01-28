#pragma once
#include <string>
#include "engine.hpp"
#include "socket.hpp"
#include "types.hpp"
#include "tcpstream.hpp"

namespace Sb {
	class TcpConn final : virtual public Socket {
		public:
			static void create(const struct InetDest& dest, std::function<std::shared_ptr<TcpStreamIf>()> clientFactory);
			TcpConn(std::function<std::shared_ptr<TcpStreamIf>()> clientFactory);
			virtual ~TcpConn();
			void handleRead();
			void handleWrite();
			void handleError();
			void handleTimer(const size_t timerId);
		private:
			std::function<std::shared_ptr<TcpStreamIf>()> clientFactory;
			std::mutex writeLock;
			SyncVec<Bytes> writeQueue;
	};
}
