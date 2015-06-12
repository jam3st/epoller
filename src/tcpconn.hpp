#pragma once
#include <string>
#include "engine.hpp"
#include "socket.hpp"
#include "types.hpp"
#include "tcpstream.hpp"
#include "resolver.hpp"

namespace Sb {
      class TcpConn;
      namespace TcpConnection {
            void create(InetDest const& dest, std::shared_ptr<TcpStreamIf> const& client);
            void create(std::string const& dest, uint16_t const port, std::shared_ptr<TcpStreamIf> const& client);
      }
      class TcpConn final : public ResolverIf {
      public:
            friend void TcpConnection::create(std::string const& dest, uint16_t const port, std::shared_ptr<TcpStreamIf> const& client);
            friend void TcpConnection::create(InetDest const& dest, std::shared_ptr<TcpStreamIf> const& client);
            TcpConn(std::shared_ptr<TcpStreamIf> const& client, uint16_t const port);
            virtual ~TcpConn();
            virtual void resolved(IpAddr const& addr) override;
            virtual void notResolved() override;
      private:
            virtual void doConnect(std::shared_ptr<TcpConn> const& ref, InetDest const& dest);
      private:
            std::shared_ptr<TcpStreamIf> client;
            uint16_t port;
            std::shared_ptr<TcpConn> self;
            std::mutex lock;
      };
}
