#pragma once
#include "event.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace Sb {
      class Socket : virtual public Runnable {
            public:
                  explicit Socket(const int fd);
                  static InetDest destFromString(const std::string& where, const uint16_t port);
                  enum SockType {
                        UDP = 1, TCP = 2
                  };
      private:
                  friend class Engine;

                  virtual void handleError();
                  virtual void handleRead();
                  virtual void handleWrite();
                  virtual bool waitingOutEvent();

            protected:
                  int const fd;
                  std::weak_ptr<Socket> self;
            protected:
                  virtual ~Socket();
                  static int createSocket(const SockType type);
                  static int createUdpSocket();
                  static int createTcpSocket();
                  static void makeNonBlocking(const int fd);
                  void onReadComplete();
                  void onWriteComplete();
                  void reuseAddress() const;
                  ssize_t read(Bytes& data) const;
                  ssize_t write(const Bytes& data) const;
                  void bind(const uint16_t port) const;
                  void connect(const InetDest& whereTo) const;
                  void listen() const;
                  int accept() const;
                  int receiveDatagram(InetDest& whereFrom, Bytes& data) const;
                  int sendDatagram(const InetDest& whereTo, const Bytes& data) const;
            private:
                  ssize_t convertFromStdError(ssize_t const error) const;
            private:
                  const int LISTEN_MAX_PENDING = 64;
      };
}
