#pragma once
#include <memory>
#include "socket.hpp"
#include "tcpstream.hpp"

namespace Sb {
      class TcpListener final : virtual public Socket {
      public:
            static void create(uint16_t const port, std::function<std::shared_ptr<TcpStreamIf>()> const& clientFactory);
            virtual ~TcpListener();
            TcpListener(uint16_t const port, std::function<std::shared_ptr<TcpStreamIf>()> const& clientFactory);
      private:
            virtual void handleRead() override;
      private:
            void createStream(const int newFd);
            std::function<std::shared_ptr<TcpStreamIf>()> clientFactory;
            std::mutex lock;
      };
}
