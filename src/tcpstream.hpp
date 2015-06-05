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

            virtual void received(Bytes const&) = 0;
            virtual void writeComplete() = 0;
            virtual void disconnected() = 0;
      protected:
            std::weak_ptr<TcpStream> tcpStream;
      };

      class TcpStream : virtual public Socket {
      public:
            static void create(std::shared_ptr<TcpStreamIf> client, int const fd);
            static void create(std::shared_ptr<TcpStreamIf> client, InetDest const&dest);
            void queueWrite(const Bytes&data);
            void disconnect();
            TcpStream(std::shared_ptr<TcpStreamIf> client);
            TcpStream(std::shared_ptr<TcpStreamIf> client, int const fd);
            virtual ~TcpStream();
            bool didConnect() const;
            InetDest endPoint() const;
            bool writeQueueEmpty();
      protected:
            virtual void handleRead() override;
            virtual void handleWrite() override;
            virtual void handleError() override;
            virtual bool waitingOutEvent() override;
      private:
            virtual void asyncWriteComplete();
            virtual void asyncDisconnect();
            virtual void asyncEgress();
      private:
            std::shared_ptr<TcpStreamIf> client;
            std::mutex writeLock;
            std::mutex readLock;
            std::deque<Bytes> writeQueue;
            bool blocked = false;
            bool once = false;
            bool writeTriggered = false;
            bool connected = false;
            bool disconnecting = false;
            std::unique_ptr<Event> notifyWriteComplete;
            std::unique_ptr<Event> activity;
            std::unique_ptr<Event> egress;
            bitsPerSecond egressRate = 0; //8ULL * 1024ULL * 1024ULL;
            Counters counters;
            NanoSecs inactivityTimeout = NanoSecs{60 * ONE_SEC_IN_NS};
      };
}

