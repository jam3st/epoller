#pragma once
#include <atomic>
#include "event.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace Sb {
      class Socket : virtual public Runnable {
      public:
            static InetDest destFromString(const std::string&where, const uint16_t port);
      protected:
            enum SockType {
                  UDP = 1, TCP = 2
            };

            explicit Socket(SockType const type);
            explicit Socket(SockType const type, int const fd);
            virtual ~Socket();
            void makeNonBlocking() const;
            void makeTransparent() const;
            void onReadComplete();
            void onWriteComplete();
            void reuseAddress() const;
            ssize_t read(Bytes&data) const;
            ssize_t write(const Bytes&data) const;
            void bind(uint16_t const port) const;
            int connect(InetDest const& whereTo) const;
            void listen() const;
            int accept() const;
            InetDest originalDestination() const;
            int receiveDatagram(InetDest& whereFrom, Bytes& data) const;
            int sendDatagram(InetDest const& whereTo, Bytes const& data) const;
            int getLastError() const;
      private:
            static int createSocket(SockType const type);
            friend class Engine;

            virtual void handleError();
            virtual void handleRead();
            virtual void handleWrite();
            virtual bool waitingOutEvent();
            ssize_t convertFromStdError(ssize_t const error) const;
      protected:
            std::weak_ptr<Socket> self;
            SockType const type;
            int const fd;
      private:
            const int LISTEN_MAX_PENDING = 64;
      };
}
