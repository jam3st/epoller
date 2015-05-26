#include "logger.hpp"
#include "tcpstream.hpp"

namespace Sb {
      void TcpStream::create(const int fd, std::shared_ptr<TcpStreamIf>& client, bool const replace) {
            auto ref = std::make_shared<TcpStream>(fd, client);
            ref->notifyWriteComplete = std::make_unique<Event>(ref, std::bind(&TcpStream::asyncWriteComplete, ref.get()));
            client->tcpStream = ref;
            Engine::add(ref, replace);
      }

      TcpStream::TcpStream(const int fd, std::shared_ptr<TcpStreamIf>& client) :
                              Socket(fd),
                              client(client),
                              writeQueue({}) {
            Socket::makeNonBlocking(fd);
      }

      TcpStream::~TcpStream() {
            client->disconnected();
      }

      void TcpStream::handleRead() {
            std::lock_guard<std::mutex> sync(readLock);
            for(;;) {
                  Bytes data(MAX_PACKET_SIZE);
                  auto const actuallyRead = read(data);
                  if(actuallyRead > 0) {
                        client->received(data);
                  } else {
                        break;
                  }
            }
      }

      bool TcpStream::handleConnect() {
            {
                  std::lock_guard<std::mutex> sync(writeLock);
                  if(notifiedConnect) {
                        return false;
                  } else {
                        notifiedConnect = true;
                        writeTriggered = true;
                        Engine::triggerWrites(this);
                  }
            }
            client->connected();
            return true;

      }
      void TcpStream::handleWrite() {
            if(!handleConnect()) {
                  writeHandler();
            }
      }

      void TcpStream::writeHandler() {
            std::lock_guard<std::mutex> sync(writeLock);
            writeTriggered = false;
            bool wasEmpty = writeQueue.len() == 0;
            blocked = false;
            for(;;) {
                  auto const q = writeQueue.removeAndIsEmpty();
                  auto const empty = q.second;
                  if(empty) {
                        break;
                  } else {
                        auto const data = q.first;
                        auto const actuallySent = write(data);
                        if((actuallySent - data.size()) == 0) {
                              continue;
                        } else if(actuallySent >= 0) {
                              writeQueue.addFirst(Bytes(data.begin() + actuallySent, data.end()));
                              continue;
                        } else if(actuallySent == -1) {
                              writeQueue.addFirst(data);
                              blocked = true;
                              break;
                        } else {
                              blocked = false;
                              break;
                        }
                  }
            }
            if(!wasEmpty && writeQueue.len() == 0 && notifyWriteComplete) {
                  Engine::runAsync(notifyWriteComplete.get());
            }
      }

      bool TcpStream::waitingOutEvent()  {
            std::lock_guard<std::mutex> sync(writeLock);
            return blocked;
      }
      void TcpStream::asyncWriteComplete() {
            client->writeComplete();
      }

      void TcpStream::handleError() {
            disconnect();
      }

      void TcpStream::disconnect() {
            std::lock_guard<std::mutex> sync(errorLock);
            if(!disconnected) {
                  disconnected = true;
                  Engine::remove(this);
            }
      }

      void TcpStream::queueWrite(const Bytes& data) {
            writeQueue.addLast(data);
            std::lock_guard<std::mutex> sync(writeLock);
            if(!writeTriggered && !blocked) {
                  writeTriggered = true;
                  Engine::triggerWrites(this);
            }
      }
}

