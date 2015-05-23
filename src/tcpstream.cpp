#include "logger.hpp"
#include "tcpstream.hpp"

namespace Sb {
      void TcpStream::create(const int fd,
                             std::shared_ptr<TcpStreamIf>& client, const bool replace) {
            auto ref = std::make_shared<TcpStream>(fd, client);
            client->tcpStream = ref;
            Engine::add(ref, replace);
      }

      TcpStream::TcpStream(const int fd, std::shared_ptr<TcpStreamIf>& client) :
                              Socket(fd),
                              client(client),
                              waitingWriteEvent(false),
                              writeQueue({}),
                              disconnected(false),
                              readError(false),
                              writeError(false) {
            Socket::makeNonBlocking(fd);
      }

      TcpStream::~TcpStream() {
            logDebug("TcpStream::~TcpStream() " + std::to_string(getFd()));
            if(client != nullptr) {
                  client->disconnected();
            }
      }

      void TcpStream::handleRead() {
            std::lock_guard<std::mutex> sync(readLock);
            logDebug("TcpStream::handleRead()");

            if(readError) {
                  return;
            }
            auto ref = client;
            for(;;) {
                  Bytes data(MAX_PACKET_SIZE);
                  auto const actuallyRead = read(data);
                  if(actuallyRead > 0) {
                        if(ref) {
                              ref->received(data);
                        }
                  } else if(actuallyRead == 0 || actuallyRead == -1) {
                        break;
                  } else {
                        readError = true;
                        break;
                  }
            }
            if(readError) {
                  disconnect();
            }
      }

      void TcpStream::handleWrite() {
            writeHandler(true);
      }

      bool TcpStream::writeHandler(bool const fromTrigger) {
            std::lock_guard<std::mutex> sync(writeLock);
            bool notify = false;
            if(writeError) {
                  return false;
            }
            if(waitingWriteEvent && !fromTrigger) {
            } else {
                  for(;;) {
                        logDebug("TcpStream::writeHandler() " + std::to_string(writeQueue.len()) + " " + std::to_string(fd));
                        auto const q = writeQueue.removeAndIsEmpty();
                        auto const empty = std::get<1>(q);
                        if(empty) {
                              notify = true;
                              waitingWriteEvent = false;
                              break;
                        } else {
                              auto const data = std::get<0>(q);
                              auto const actuallySent = write(data);
                              if(actuallySent - data.size() == 0) {
                                    continue;
                              } else if(actuallySent >= 0) {
                                    writeQueue.addFirst(Bytes(data.begin() + actuallySent, data.end()));
                                    continue;
                              }else if(actuallySent == -1) {
                                    waitingWriteEvent = true;
                                    writeQueue.addFirst(data);
                                    break;
                              } else  {
                                    logDebug(std::string("TcpStream::write failed"));
                                    writeQueue.addFirst(data);
                                    writeError = true;
                                    notify = false;
                                    break;
                              }
                        }
                  }
            }
            if(writeError) {
                  disconnect();
            }
            return notify;
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
            logDebug("TcpStream::queueWrite() " + std::to_string(fd));
            writeQueue.addLast(data);
            if(writeHandler()) {
                  auto ref = client;
                  if(ref) {
                        ref->writeComplete();
                  }
            }
      }
      //
      //    void TcpStream::handleTimer(size_t const timerId) {
      //          logDebug("TcpListener::handleTimer() " + std::to_string(timerId) + " " + std::to_string(fd));
      //          client->timeout(timerId);
      //    }


}

