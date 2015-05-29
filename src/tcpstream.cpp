#include "logger.hpp"
#include "tcpstream.hpp"

namespace Sb {
      void TcpStream::create(std::shared_ptr<TcpStreamIf>& client, int const fd) {
            auto ref = std::make_shared<TcpStream>(fd, client);
            client->tcpStream = ref;
            ref->notifyWriteComplete = std::make_unique<Event>(ref, std::bind(&TcpStream::asyncWriteComplete, ref.get()));
            ref->connectTimer = std::make_unique<Event>(ref, std::bind(&TcpStream::asyncConnectCheck, ref.get()));
            ref->connected = true;
            Engine::add(ref);
      }

      void TcpStream::create(std::shared_ptr<TcpStreamIf>& client, InetDest const& dest) {
            auto ref = std::make_shared<TcpStream>(Socket::createTcpSocket(), client);
            client->tcpStream = ref;
            ref->notifyWriteComplete = std::make_unique<Event>(ref, std::bind(&TcpStream::asyncWriteComplete, ref.get()));
            ref->connectTimer = std::make_unique<Event>(ref, std::bind(&TcpStream::asyncConnectCheck, ref.get()));
            Engine::add(ref);
            ref->connect(dest);
            Engine::setTimer(ref->connectTimer.get(), NanoSecs { 10'000'000'000 });
      }

      TcpStream::TcpStream(const int fd, std::shared_ptr<TcpStreamIf>& client) :
                              Socket(fd),
                              client(client),
                              writeQueue({}),
                              blocked(true),
                              connected(false) {
            Socket::makeNonBlocking(fd);
      }

      TcpStream::~TcpStream() {
            client->disconnected();
            std::lock_guard<std::mutex> sync(writeLock);
            client = nullptr;
            Engine::cancelTimer(connectTimer.get());
            notifyWriteComplete.reset();
            connectTimer.reset();
      }

      void TcpStream::handleRead() {
            std::lock_guard<std::mutex> sync(readLock);
            for(;;) {
                  Bytes data(MAX_PACKET_SIZE);
                  auto const actuallyRead = read(data);
                  if(actuallyRead > 0) {
                        counters.notifyIngress(actuallyRead);
                        client->received(data);
                  } else {
                        break;
                  }
            }
      }

      void TcpStream::handleWrite() {
            std::lock_guard<std::mutex> sync(writeLock);
            if(!connected) {
                  connected = true;
                  if(connectTimer) {
                        Engine::cancelTimer(connectTimer.get());
                  }
            } else {
                  if(!once) {
                        once = true;
                  }
            }

            writeTriggered = false;
            blocked = false;
            bool wasEmpty = (writeQueue.len() == 0);
            for(;;) {
                  auto const q = writeQueue.removeAndIsEmpty();
                  auto const empty = q.second;
                  if(empty) {
                        break;
                  } else {
                        auto const data = q.first;
                        auto const actuallySent = write(data);
                        if((actuallySent - data.size()) == 0) {
                              counters.notifyEgress(actuallySent);
                              continue;
                        } else if(actuallySent >= 0) {
                              writeQueue.addFirst(Bytes(data.begin() + actuallySent, data.end()));
                              counters.notifyEgress(actuallySent);
                              continue;
                        } else if(actuallySent == -1) {
                              writeQueue.addFirst(data);
                              blocked = true;
                              break;
                        } else {
                              break;
                        }
                  }
            }
            bool isEmpty = (writeQueue.len() == 0);
            if(!once || (!wasEmpty && isEmpty)) {
                  once = true;
                  if(notifyWriteComplete) {
                        Engine::runAsync(notifyWriteComplete.get());
                  }
            }
      }

      bool TcpStream::waitingOutEvent()  {
            std::lock_guard<std::mutex> sync(writeLock);
            return blocked || !once || !connected;
      }

      void TcpStream::asyncWriteComplete() {
            if(client) {
                  client->writeComplete();
            }
      }

      void TcpStream::asyncConnectCheck() {
            std::lock_guard<std::mutex> sync(writeLock);
            if(!connected) {
                  logError("Timeout in connection");
                  disconnect();
            }
      }


      void TcpStream::handleError() {
            disconnect();
      }

      void TcpStream::disconnect() {
            std::lock_guard<std::mutex> sync(errorLock);
            if(!disconnected) {
                  disconnected = true;
                  Engine::remove(self);
            }
      }

      void TcpStream::queueWrite(const Bytes& data) {
            std::lock_guard<std::mutex> sync(writeLock);
            writeQueue.addLast(data);
            if(connected && !blocked && !writeTriggered) {
                  writeTriggered = true;
                  Engine::triggerWrites(this);
            }
      }
}

