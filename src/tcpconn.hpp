#pragma once
#include <string>
#include "engine.hpp"
#include "socket.hpp"
#include "types.hpp"
#include "tcpstream.hpp"

namespace Sb {
	class TcpConn final : virtual public Socket {
		public:
			static void create(const struct InetDest& dest, std::shared_ptr<TcpStreamIf> client);
			TcpConn(std::shared_ptr<TcpStreamIf> client);
			virtual ~TcpConn();
			virtual void handleRead() override;
			virtual void handleWrite() override;
			virtual void handleError() override;
			virtual void handleTimer(const size_t timerId);
		private:
			std::shared_ptr<TcpStreamIf> client;
			std::mutex writeLock;
			SyncVec<Bytes> writeQueue;
	};
}
