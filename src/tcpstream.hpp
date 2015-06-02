#pragma once
#include <functional>
#include <deque>
#include "engine.hpp"
#include "socket.hpp"
#include "types.hpp"
#include "counters.hpp"

namespace Sb {
      class TcpStream;

      class TcpStreamIf : public std::enable_shared_from_this<TcpStreamIf> {
      public:
            friend class TcpStream;

            virtual void disconnect() = 0;
            virtual void received(Bytes const&) = 0;
            virtual void writeComplete() = 0;
            virtual void disconnected() = 0;
      protected:
            std::weak_ptr<TcpStream> tcpStream;
      };

      class TcpStream : virtual public Socket {
      public:
            static void create(std::shared_ptr<TcpStreamIf>&client, int const fd);
            static void create(std::shared_ptr<TcpStreamIf>&client, InetDest const&dest);
            void queueWrite(const Bytes&data);
            void disconnect();
            TcpStream(std::shared_ptr<TcpStreamIf>& client);
            TcpStream(std::shared_ptr<TcpStreamIf>&client, int const fd);
            virtual ~TcpStream();
      protected:
            bool doWrite(Bytes const&data);
            virtual void handleRead() override;
            virtual void handleWrite() override;
            virtual void handleError() override;
            virtual bool waitingOutEvent() override;
      private:
            virtual void asyncWriteComplete();
            virtual void asyncConnectCheck();
      private:
            std::shared_ptr<TcpStreamIf> client;
            std::mutex writeLock;
            std::mutex readLock;
            std::deque<Bytes> writeQueue;
            bool blocked = true;
            bool once = false;
            bool connected = false;
            bool writeTriggered = false;
            std::atomic_bool disconnecting;
            std::unique_ptr<Event> notifyWriteComplete;
            std::unique_ptr<Event> connectTimer;
            Counters counters;
      };
}

