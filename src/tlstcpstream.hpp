#pragma once
#include <queue>
#include <mutex>
#include <botan/tls_client.h>
#include <botan/auto_rng.h>
#include "engine.hpp"
#include "socket.hpp"
#include "tlscredentials.hpp"

namespace Sb {
	class TlsTcpStream : virtual public Socket {
		public:
			TlsTcpStream(const int fd, const Bytes& initialData = {});
			virtual void queueWrite(const Bytes& data);
			virtual ~TlsTcpStream();
		protected:
			void handleRead();
			void handleWrite();
			void handleError();
			void handleTimeout(int timerId);
		private:
			std::queue<Bytes> writeQueue;
			std::mutex writeQueueLock;
			Botan::AutoSeeded_RNG rng;
			Botan::TLS::Policy policy;
	};
}
