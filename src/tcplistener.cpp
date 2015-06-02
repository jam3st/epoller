#include <string>
#include "logger.hpp"
#include "tcplistener.hpp"

namespace Sb {
      void
      TcpListener::create(const uint16_t port, std::function<std::shared_ptr<TcpStreamIf>()> clientFactory) {
            Engine::add(std::make_shared<TcpListener>(port, clientFactory));
      }

      TcpListener::TcpListener(const uint16_t port, std::function<std::shared_ptr<TcpStreamIf>()> clientFactory) : Socket(TCP),
                                                                                                                   clientFactory(clientFactory) {
            reuseAddress();
            makeTransparent();
            bind(port);
            listen();
      }

      TcpListener::~TcpListener() {
      }

      void TcpListener::handleRead() {
            for (; ;) {
                  int connFd = -1;
                  {
                        std::lock_guard<std::mutex> sync(lock);
                        connFd = accept();
                  }
                  if (connFd >= 0) {
                        createStream(connFd);
                  } else {
                        break;
                  }
            }
      }

      void TcpListener::createStream(const int connFd) {
            auto client = clientFactory();
            TcpStream::create(client, connFd);
      }
}
