#pragma once
#include <atomic>
#include <deque>
#include <memory>
#include "socket.hpp"

namespace Sb {
      class UdpSocket;

      class UdpSocketIf {
      public:
            friend class UdpSocket;
            [[deprecated("cOnnnect is useless")]]
            virtual void connected(const InetDest&) = 0;
            virtual void disconnected() = 0;
            virtual void received(InetDest const&, Bytes const&) = 0;
            virtual void notSent(const InetDest&, const Bytes&) = 0;
            virtual void writeComplete() = 0;
            virtual void disconnect() = 0;
      protected:
            std::weak_ptr<Socket> udpSocket;
      };

      class UdpSocket final : virtual public Socket {
      public:
            static void create(uint16_t const localPort, std::shared_ptr<UdpSocketIf>& client);
            static void create(InetDest const& dest, std::shared_ptr<UdpSocketIf>& client);
            void queueWrite(const InetDest& dest, const Bytes& data);
            void disconnect();
            virtual ~UdpSocket();
            virtual void handleRead() override;
            virtual void handleWrite() override;
            virtual void handleError() override;
            virtual bool waitingOutEvent() override;
            void doWrite(const InetDest& dest, const Bytes& data);
            UdpSocket(std::shared_ptr<UdpSocketIf>& client);
      private:
            void bindAndAdd(std::shared_ptr<Socket>& me, uint16_t const localPort, std::shared_ptr<UdpSocketIf>& client);
            void connectAndAdd(std::shared_ptr<Socket>& me, InetDest const& dest, std::shared_ptr<UdpSocketIf>& client);
      private:
            std::shared_ptr<UdpSocketIf> client;
            //                InetDest myDest;
            std::mutex writeLock;
            std::mutex readLock;
            //                bool waitingWriteEvent;
            std::deque<std::pair<const InetDest, const Bytes>> writeQueue;
      };
}
