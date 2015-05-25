#include "tcpconn.hpp"

namespace Sb {
      namespace TcpConnection {
            void create(const struct InetDest& dest, std::shared_ptr<TcpStreamIf> client) {
                  auto ref = std::make_shared<TcpConn>(client, dest.port);
                  logDebug("TcpConn::create() " + dest.toString() + " " + std::to_string(ref->getFd()));
                  ref->doConnect(ref, dest);
            }

            void create(const std::string& dest, const uint16_t port, std::shared_ptr<TcpStreamIf> client) {
                  InetDest inetDest = Socket::destFromString(dest, port);

                  if(inetDest.valid) {
                        create(inetDest, client);
                  } else {
                        auto ref = std::make_shared<TcpConn>(client, port);
                        logDebug("TcpConn::create() " + dest + " " + std::to_string(ref->getFd()));
                        ref->self = ref;
                        std::shared_ptr<ResolverIf> res = ref;
                        Engine::resolver().resolve(res, dest);
                  }
            }
      }

      TcpConn::TcpConn(std::shared_ptr<TcpStreamIf> client, uint16_t port) : Socket(Socket::createTcpSocket()), client(client), port(port), self(nullptr),
                                                                             added(false) {
            logDebug("TcpConn::TcpConn() " + std::to_string(fd));
      }

      TcpConn::~TcpConn() {
            logDebug("TcpConn::~TcpConn() " + std::to_string(fd));
            Engine::resolver().cancel(this);
            auto ref = client;
            if(ref != nullptr) {
                  ref->disconnected();
            }
      }

      void
      TcpConn::handleRead() {
            assert(false, "TcpConn::handleRead() is not a valid operation");
      }

      void TcpConn::createStream() {
            logDebug("TcpConn::createStream() " + std::to_string(fd));
            std::lock_guard<std::mutex> sync(lock);
            if(client) {
                  // event still has a reference
                  Engine::remove(this);
                  auto const thisFd = releaseFd();
                  TcpStream::create(thisFd, client);
                  added = false;
                  client = nullptr;
            }
      }

      void
      TcpConn::handleWrite() {
            createStream();
      }

      void TcpConn::doConnect(std::shared_ptr<TcpConn>& ref, InetDest const& dest) {
            std::lock_guard<std::mutex> sync(lock);
            added = true;
            connect(dest);
            Engine::add(ref);
            self = nullptr; // After add because self == ref when called from resolver
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

      void
      TcpConn::resolved(IpAddr const& addr) {
            InetDest dest { addr, true, port };
            logDebug("resolved added " + dest.toString());
            doConnect(self, dest);
      }

      void TcpConn::error() {
            logDebug("TcpConn resolver error not connecting");
            handleError();
      }
}
