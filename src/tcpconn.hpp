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
            void create(const struct InetDest& dest, std::shared_ptr<TcpStreamIf> client);
            void create(const std::string& dest, const uint16_t port, std::shared_ptr<TcpStreamIf> client);
      }

      class TcpConn final
            : public Socket,
              public ResolverIf {
            public:
                  friend void TcpConnection::create(const std::string& dest, const uint16_t port, std::shared_ptr<TcpStreamIf> client);
                  friend void TcpConnection::create(const InetDest& dest, std::shared_ptr<TcpStreamIf> client);
                  TcpConn(std::shared_ptr<TcpStreamIf> client, uint16_t port);
                  virtual ~TcpConn();
                  virtual void handleRead() override;
                  virtual void handleWrite() override;
                  virtual void handleError() override;
                  virtual void resolved(IpAddr const& addr) override;
                  virtual void error() override;
                  void createStream();

            private:
                  void doConnect(const InetDest &dest) const;

            private:
                  std::shared_ptr<TcpStreamIf> client;
                  uint16_t port;
                  std::shared_ptr<Socket> self;
                  bool added;
                  std::mutex lock;
      };
}
