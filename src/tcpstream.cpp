#include "logger.hpp"
#include "tcpstream.hpp"

namespace Sb {
      void TcpStream::create(std::shared_ptr<TcpStreamIf>&client, int const fd) {
            auto ref = std::make_shared<TcpStream>(client, fd);
            client->tcpStream = ref;
            ref->notifyWriteComplete = std::make_unique<Event>(ref, std::bind(&TcpStream::asyncWriteComplete, ref.get()));
            ref->connectTimer = std::make_unique<Event>(ref, std::bind(&TcpStream::asyncConnectCheck, ref.get()));
            ref->connected = true;
            ref->originalDestination();
            Engine::add(ref);
      }

      void TcpStream::create(std::shared_ptr<TcpStreamIf>&client, InetDest const&dest) {
            auto ref = std::make_shared<TcpStream>(client);
            client->tcpStream = ref;
            ref->notifyWriteComplete = std::make_unique<Event>(ref, std::bind(&TcpStream::asyncWriteComplete, ref.get()));
            ref->connectTimer = std::make_unique<Event>(ref, std::bind(&TcpStream::asyncConnectCheck, ref.get()));
            auto const err = ref->connect(dest);
            if(err >= 0) {
                  logError("FROM CONST");
                  ref->connected = true;
            } else if(err == -1) {
                  ref->connected = false;
                  Engine::setTimer(ref->connectTimer.get(), NanoSecs{10 * ONE_SEC_IN_NS});
            } else {
                  pErrorLog(err, ref->fd);
                  client->disconnect();
                  return;
            }
            Engine::add(ref);
      }

      TcpStream::TcpStream(std::shared_ptr<TcpStreamIf>& client) : Socket(TCP), client(client), writeQueue({}), blocked(true),
                                                                                 connected(false), disconnecting(false) {
      }


      TcpStream::TcpStream(std::shared_ptr<TcpStreamIf>& client, const int fd) : Socket(TCP, fd), client(client), writeQueue({}), blocked(true),
                                                                                 connected(false), disconnecting(false) {
      }

      TcpStream::~TcpStream() {
            std::lock_guard<std::mutex> syncWrite(writeLock);
            std::lock_guard<std::mutex> syncRead(readLock);
            client->disconnected();
            client = nullptr;
            Engine::cancelTimer(connectTimer.get());
            notifyWriteComplete.reset();
            connectTimer.reset();
      }

      void TcpStream::handleRead() {
            std::lock_guard<std::mutex> sync(readLock);
            for (; ;) {
                  Bytes data(MAX_PACKET_SIZE);
                  auto const actuallyRead = read(data);
                  if (actuallyRead > 0) {
                        counters.notifyIngress(actuallyRead);
                        client->received(data);
                  } else {
                        break;
                  }
            }
      }

      void TcpStream::handleWrite() {
            std::lock_guard<std::mutex> sync(writeLock);
            writeTriggered = false;
            blocked = false;
            if(connected) {
                  bool wasEmpty = (writeQueue.size() == 0);
                  for (; ;) {
                        if (writeQueue.size() == 0) {
                              break;
                        } else {
                              auto const data = writeQueue.front();
                              writeQueue.pop_front();
                              auto const actuallySent = write(data);
                              if ((actuallySent - data.size()) == 0) {
                                    counters.notifyEgress(actuallySent);
                                    continue;
                              } else if (actuallySent >= 0) {
                                    writeQueue.push_front(Bytes(data.begin() + actuallySent, data.end()));
                                    counters.notifyEgress(actuallySent);
                                    continue;
                              } else if (actuallySent == -1) {
                                    writeQueue.push_front(data);
                                    blocked = true;
                                    break;
                              } else {
                                    break;
                              }
                        }
                  }
                  bool isEmpty = (writeQueue.size() == 0);
                  if (!once || (!wasEmpty && isEmpty)) {
                        once = true;
                        if (notifyWriteComplete) {
                              Engine::runAsync(notifyWriteComplete.get());
                        }
                  }
            } else {
                  connected = true;
                  if (notifyWriteComplete) {
                        Engine::runAsync(notifyWriteComplete.get());
                  }
                  if (connectTimer) {
                        Engine::cancelTimer(connectTimer.get());
                  }
            }
      }

      bool TcpStream::waitingOutEvent() {
            std::lock_guard<std::mutex> sync(writeLock);
            return (blocked || !once || !connected) && !disconnecting;
      }

      void TcpStream::asyncWriteComplete() {
            if (client) {
                  client->writeComplete();
            }
      }

      void TcpStream::asyncConnectCheck() {
            std::lock_guard<std::mutex> sync(writeLock);
            logError("asyncConnectCheck");
            if (!connected) {
                  logError("Timeout in connection");
                  disconnect();
            }
      }

      void TcpStream::handleError() {
            disconnect();
      }

      void TcpStream::disconnect() {
            if (!disconnecting) {
                  disconnecting = true;
                  if (counters.getEgress() == 0 || counters.getIngress() == 0) {

                        getLastError();
                  }
                  Engine::remove(self);
            }
      }

      void TcpStream::queueWrite(const Bytes&data) {
            std::lock_guard<std::mutex> sync(writeLock);
            writeQueue.push_back(data);
            if (connected && !blocked && !writeTriggered) {
                  writeTriggered = true;
                  Engine::triggerWrites(this);
            }
      }
}

