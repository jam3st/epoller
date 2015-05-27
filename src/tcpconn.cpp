#include "tcpconn.hpp"

namespace Sb {
      namespace TcpConnection {
             void create(const std::string &dest, const uint16_t port, std::shared_ptr<TcpStreamIf> client) {
                  InetDest inetDest = Socket::destFromString(dest, port);

                  if(inetDest.valid) {
                        TcpStream::create(client, inetDest);
                  } else {
                        auto ref = std::make_shared<TcpConn>(client, port);
                        ref->self = ref;
                        std::shared_ptr<ResolverIf> res = ref;
                        Engine::resolver().resolve(res, dest);
                  }
            }
      }

      TcpConn::TcpConn(std::shared_ptr<TcpStreamIf> client, uint16_t port) : client(client), port(port) {
      }

      TcpConn::~TcpConn() {
            Engine::resolver().cancel(this);
            auto ref = client;
            if(ref != nullptr) {
                  ref->disconnected();
            }
      }

      void TcpConn::doConnect(std::shared_ptr<TcpConn> &ref, InetDest const &dest) {
            std::lock_guard<std::mutex> sync(lock);
            if(client) {
                  TcpStream::create(client, dest);
                  client = nullptr;
            }
            self = nullptr;
      }


      void TcpConn::resolved(IpAddr const& addr) {
            InetDest dest { addr, true, port };
            doConnect(self, dest);
      }

      void TcpConn::notResolved() {
            logDebug("TcpConn::notResolved error not connecting");
            std::lock_guard<std::mutex> sync(lock);
            if(client) {
                  client->disconnected();
                  client = nullptr;
            }
      }
}
