#pragma once
#include <functional>
#include "syncvec.hpp"
#include "engine.hpp"
#include "socket.hpp"
#include "types.hpp"

namespace Sb {
	class TcpStream;
	class TcpStreamIf : public std::enable_shared_from_this<TcpStreamIf> {
		public:
			virtual void connected(std::shared_ptr<TcpStream> stream, const struct InetDest&) = 0;
			virtual void received(const Bytes&) = 0;
			virtual void writeComplete() = 0;
			virtual void disconnected() = 0;
			virtual void timeout(const size_t timerId) = 0;

			std::shared_ptr<TcpStreamIf> ref() {
				 return shared_from_this();
			}
			std::shared_ptr<TcpStream>& getStream() {
				return stream;
			}

			std::shared_ptr<TcpStream> stream;
	};

	class TcpStream : virtual public Socket {
		public:
			static void create(const int fd, const struct InetDest remote, std::shared_ptr<TcpStreamIf> client, const bool replace = false);
			void queueWrite(const Bytes& data);
			void disconnect();
			TcpStream(const int fd, const struct InetDest remote, std::shared_ptr<TcpStreamIf>& client);
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

