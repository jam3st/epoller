﻿#pragma once
#include <string>
#include "engine.hpp"
#include "socket.hpp"
#include "types.hpp"
#include "tcpstream.hpp"
#include "resolver.hpp"

namespace Sb {
	class TcpConn;
	namespace TcpConnection {
		void create(const struct InetDest& dest, std::shared_ptr<TcpStreamIf> client);
		void create(const std::string& dest, const uint16_t port, std::shared_ptr<TcpStreamIf> client);
	}

	class TcpConn final : public Socket, public ResolverIf {
		public:
			friend void TcpConnection::create(const std::string &dest, const uint16_t port, std::shared_ptr<TcpStreamIf> client);
			friend void TcpConnection::create(const InetDest &dest, std::shared_ptr<TcpStreamIf> client);
			TcpConn(std::shared_ptr<TcpStreamIf> client, uint16_t port);
			virtual ~TcpConn();
			virtual void handleRead() override;
			virtual void handleWrite() override;
			virtual void handleError() override;
			virtual void handleTimer(const size_t timerId) override;
			virtual void resolved(const IpAddr& addr) override;
			virtual void error() override;
			void createStream();
		private:
			std::shared_ptr<TcpStreamIf> client;
			uint16_t port;
			bool added = false;
	};
}
