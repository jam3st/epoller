#pragma once
#include "epoll.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace Sb {
	class Socket : virtual public Epoll {
		public:
			explicit Socket();
			enum SockType {
				UDP = 1,
				TCP = 2
			};
		protected:
			virtual ~Socket();
			static int createSocket(const SockType type);
			static int createUdpSocket();
			static int createTcpSocket();
			static void makeSocketNonBlocking(const int fd);
			void onReadComplete();
			void onWriteComplete();
			void reuseAddress() const;
			int read(Bytes& data) const;
			int write(const Bytes& data) const;
			void bind(const uint16_t port) const;
			void connect(const InetDest& whereTo) const;
			void listen() const;
			int	accept() const;
			int receiveDatagram(Bytes& data, InetDest& whereFrom) const;
			int sendDatagram(const Bytes& data, const InetDest& whereTo) const;
			InetDest destFromString(const std::string& where, const uint16_t port) const;
	};
}
