#pragma once
#include "timeevent.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace Sb {
      class Socket : virtual public TimeEvent {
            public:
                  explicit Socket(const int fd);
                  static InetDest destFromString(const std::string& where, const uint16_t port);
                  enum SockType {
                        UDP = 1, TCP = 2
                  };
            protected:
                  virtual int getFd() const final;
                  virtual int releaseFd() const final;
                  virtual bool fdReleased() const final;
      private:
                  friend class Engine;

                  virtual void handleError() = 0;
                  virtual void handleRead() = 0;
                  virtual void handleWrite() = 0;
            protected:
                  mutable int fd;
            protected:
                  virtual ~Socket();
                  static int createSocket(const SockType type);
                  static int createUdpSocket();
                  static int createTcpSocket();
                  static void makeNonBlocking(const int fd);
                  void onReadComplete();
                  void onWriteComplete();
                  void reuseAddress() const;
                  int read(Bytes& data) const;
                  int write(const Bytes& data) const;
                  void bind(const uint16_t port) const;
                  void connect(const InetDest& whereTo) const;
                  void listen() const;
                  int accept() const;
                  int receiveDatagram(InetDest& whereFrom, Bytes& data) const;
                  int sendDatagram(const InetDest& whereTo, const Bytes& data) const;
            private:
                  const int LISTEN_MAX_PENDING = 1;
      };
}
