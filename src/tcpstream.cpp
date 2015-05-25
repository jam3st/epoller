#include "logger.hpp"
#include "tcpstream.hpp"

namespace Sb {
      void TcpStream::create(const int fd,
                             std::shared_ptr<TcpStreamIf>& client) {
            auto ref = std::make_shared<TcpStream>(fd, client);
            ref->notifyWriteComplete = std::make_unique<Event>(ref, std::bind(&TcpStream::asyncWriteComplete, ref.get()));
            client->tcpStream = ref;
            Engine::add(ref);
      }

      TcpStream::TcpStream(const int fd, std::shared_ptr<TcpStreamIf>& client) :
                              Socket(fd),
                              client(client),
                              writeQueue({}) {
            Socket::makeNonBlocking(fd);
      }

      TcpStream::~TcpStream() {
            logDebug("TcpStream::~TcpStream() " + std::to_string(getFd()));
            client->disconnected();
      }

      void TcpStream::handleRead() {
            std::lock_guard<std::mutex> sync(readLock);
            if(fatalReadError) {
                  return;
            }
            for(;;) {
                  Bytes data(MAX_PACKET_SIZE);
                  auto const actuallyRead = read(data);
                  if(actuallyRead > 0) {
                        client->received(data);
                  } else if(actuallyRead == 0 || actuallyRead == -1) {
                        break;
                  } else {
                        fatalReadError = true;
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
                        logDebug("TcpStream::handleConnect() " + std::to_string(fd));
                        notifiedConnect = true;
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
            if(fatalWriteError) {
                  return;
            }
            bool wasEmpty = writeQueue.len() == 0;
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
                              break;
                        } else {
                              fatalWriteError = true;
                        }
                  }
            }
            if(!wasEmpty && writeQueue.len() == 0 && notifyWriteComplete) {
                  Engine::runAsync(notifyWriteComplete.get());
            }

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
            writeHandler();
      }
}

