#include "tcpconn.hpp"

namespace Sb {
      namespace TcpConnection {
            void create(const struct InetDest &dest, std::shared_ptr<TcpStreamIf> client) {
                  auto ref = std::make_shared<TcpConn>(client, dest.port);
                  ref->doConnect(ref, dest);
            }

            void create(const std::string &dest, const uint16_t port, std::shared_ptr<TcpStreamIf> client) {
                  InetDest inetDest = Socket::destFromString(dest, port);

                  if(inetDest.valid) {
                        create(inetDest, client);
                  } else {
                        auto ref = std::make_shared<TcpConn>(client, port);
                        ref->self = ref;
                        std::shared_ptr<ResolverIf> res = ref;
                        Engine::resolver().resolve(res, dest);
                  }
            }
      }

      TcpConn::TcpConn(std::shared_ptr<TcpStreamIf> client, uint16_t port) : Socket(Socket::createTcpSocket()), client(client), port(port),
                                                                             self(nullptr), added(false) {
      }

      TcpConn::~TcpConn() {
            Engine::resolver().cancel(this);
            auto ref = client;
            if(ref != nullptr) {
                  ref->disconnected();
            }
      }

      void
      TcpConn::handleRead() {
      }

      void TcpConn::handleWrite() {
      }

      void TcpConn::doConnect(std::shared_ptr<TcpConn> &ref, InetDest const &dest) {
            std::lock_guard<std::mutex> sync(lock);
            added = true;
            if(client) {
                  auto const thisFd = releaseFd();
                  TcpStream::create(thisFd, client, dest);
                  added = false;
                  client = nullptr;
            }

            // After add because self == ref when called from resolver
            self = nullptr;
      }

      void
      TcpConn::handleError() {
            logDebug("TcpConn::handleError " + std::to_string(fd));
            std::lock_guard<std::mutex> sync(lock);
            if(added) {
                  added = false;
                  Engine::remove(this);
            }

            if(client) {
                  client->disconnected();
                  client = nullptr;
            }
      }

      bool TcpConn::waitingOutEvent() {
            return true;
      }

      void TcpConn::resolved(IpAddr const& addr) {
            InetDest dest { addr, true, port };
            doConnect(self, dest);
      }

      void TcpConn::notResolved() {
            logDebug("TcpConn resolver error not connecting");
            handleError();
      }
}
