#pragma once
#include <functional>
#include "syncvec.hpp"
#include "engine.hpp"
#include "socket.hpp"
#include "types.hpp"

namespace Sb {
	class TcpStream;
	class TcpStreamIf {
		public:
			virtual void onConnect(TcpStream&, const struct InetDest&) = 0;
			virtual void onReadCompleted(TcpStream&, const Bytes&) = 0;
			virtual void onWriteCompleted(TcpStream&) = 0;
			virtual void onDisconnect(TcpStream&) = 0;
			virtual void onTimerExpired(TcpStream&, int) = 0;
	};

	class TcpStream : virtual public Socket {
		public:
			static void create(const int fd, const struct InetDest remote, std::shared_ptr<TcpStreamIf> client);
			void queueWrite(const Bytes& data);
			void disconnect();
			TcpStream(const int fd, const struct InetDest remote,
					  std::shared_ptr<TcpStreamIf> client);
			virtual ~TcpStream();

		protected:
			void doWrite(const Bytes& data);
			virtual void handleRead();
			virtual void handleWrite();
			virtual void handleError();
			virtual void handleTimer(const size_t timerId);

		private:
			const struct InetDest remote;
			std::shared_ptr<TcpStreamIf> client;
			std::mutex writeLock;
			SyncVec<Bytes> writeQueue;
	};
}

