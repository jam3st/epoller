#pragma once
#include <functional>
#include "syncqueue.hpp"
#include "engine.hpp"
#include "socket.hpp"
#include "types.hpp"

namespace Sb {
      class TcpStream;

      class TcpStreamIf : public std::enable_shared_from_this<TcpStreamIf> {
            public:
                  friend class TcpStream;
                  virtual void connected() = 0;
                  virtual void disconnect() = 0;
                  virtual void received(const Bytes&) = 0;
                  virtual void writeComplete() = 0;
                  virtual void disconnected() = 0;
            protected:
                  std::weak_ptr<TcpStream> tcpStream;
      };

      class TcpStream : virtual public Socket {
            public:
                  static void create(const int fd, std::shared_ptr<TcpStreamIf>& client, bool const replace = false);
                  void queueWrite(const Bytes& data);
                  void disconnect();
                  TcpStream(const int fd, std::shared_ptr<TcpStreamIf>& client);
                  virtual ~TcpStream();

            protected:
                  bool doWrite(Bytes const& data);
                  virtual void handleRead() override;
                  virtual void handleWrite() override;
                  virtual void handleError() override;
                  virtual bool waitingOutEvent() override;

            private:
                  virtual bool handleConnect();
                  virtual void asyncWriteComplete();
                  virtual void writeHandler();

            private:
                  std::shared_ptr<TcpStreamIf> client;
                  std::mutex writeLock;
                  std::mutex readLock;
                  std::mutex errorLock;
                  SyncQueue<Bytes> writeQueue;
                  bool disconnected = false;
                  bool notifiedConnect = false;
                  bool blocked = false;
                  bool writeTriggered = false;
                  std::unique_ptr<Event> notifyWriteComplete;
      };
}

