#include <string>

#include "logger.hpp"
#include "tcplistener.hpp"

namespace Sb {
      void
      TcpListener::create(const uint16_t port, std::function<std::shared_ptr<TcpStreamIf>()> clientFactory) {
            auto tmp = std::make_shared<TcpListener>(port, clientFactory);
            logDebug("TcpListener::create fd " + std::to_string(tmp->getFd()));
            Engine::add(tmp);
      }

      TcpListener::TcpListener(const uint16_t port, std::function<std::shared_ptr<TcpStreamIf>()> clientFactory) : Socket(Socket::createTcpSocket()),
            clientFactory(clientFactory) {
            logDebug(std::string("TcpListener::TcpListener " + std::to_string(port)));
            reuseAddress();
            bind(port);
            listen();
      }

      TcpListener::~TcpListener() {
            logDebug("TcpListener::~TcpListener()");
      }

      void
      TcpListener::handleRead() {
            logDebug("TcpListener::handleRead");
            for(;;) {
                  int const connFd = accept();
                  if(connFd >= 0) {
                        createStream(connFd);
                  } else if(connFd == -1) {
                        break;
                  } else {
                        continue;
                  }
            }

      }

      void
      TcpListener::handleWrite() {
            logDebug("Epollable::handleWrite()");
      }

      void
      TcpListener::handleError() {
            logDebug("Epollable::handleWrite()");
      }

      void TcpListener::createStream(const int connFd) {
            auto client = clientFactory();
            TcpStream::create(connFd, client);
      }
}
