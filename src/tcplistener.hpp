#pragma once
#include <memory>
#include "socket.hpp"
#include "tcpstream.hpp"

namespace Sb {
      class TcpListener final : virtual public Socket {
            public:
                  static void create(const uint16_t port, std::function<std::shared_ptr<TcpStreamIf>()> clientFactory);
                  virtual ~TcpListener();
                  void handleRead();
                  void handleWrite();
                  void handleError();
                  TcpListener(const uint16_t port, std::function<std::shared_ptr<TcpStreamIf>()> clientFactory);
            private:
                  void createStream(const int newFd);
                  std::function<std::shared_ptr<TcpStreamIf>()> clientFactory;
                  std::mutex lock;
      };
}
