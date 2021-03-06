﻿#include "logger.hpp"
#include "tcpstream.hpp"

namespace Sb {
      void TcpStream::create(std::shared_ptr<TcpStreamIf> const& client, int const fd) {
            auto ref = std::make_shared<TcpStream>(client, fd);
            client->tcpStream = ref;
            ref->notifyWriteComplete = Event(ref, std::bind(&TcpStream::asyncWriteComplete, ref));
            ref->activity = Event(ref, std::bind(&TcpStream::asyncDisconnect, ref));
            ref->egress = Event(ref, std::bind(&TcpStream::asyncEgress, ref));
            ref->connected = true;
            Engine::runAsync(ref->notifyWriteComplete);
            { ref->originalDestination();
             ref->egressRate = 4096 * 1024; }
            std::shared_ptr<Socket> sockRef = ref;
            Engine::add(sockRef);
      }

      void TcpStream::create(std::shared_ptr<TcpStreamIf> const& client, InetDest const& dest) {
            auto ref = std::make_shared<TcpStream>(client);
            client->tcpStream = ref;
            ref->notifyWriteComplete = Event(ref, std::bind(&TcpStream::asyncWriteComplete, ref));
            ref->activity = Event(ref, std::bind(&TcpStream::asyncDisconnect, ref));
            ref->egress = Event(ref, std::bind(&TcpStream::asyncEgress, ref));

            auto const err = ref->connect(dest);
            if(err >= 0) {
                  ref->connected = true;
                  Engine::setTimer(ref->activity, ref->inactivityTimeout);
            } else if(err == -1) {
                  ref->connected = false;
                  Engine::setTimer(ref->activity, ref->inactivityTimeout);
            } else {
                  pErrorLog(err, ref->fd);
                  return;
            }
            std::shared_ptr<Socket> sockRef = ref;
            Engine::add(sockRef);
      }

      TcpStream::TcpStream(std::shared_ptr<TcpStreamIf> const& client) : Socket(TCP), client(client) {
      }

      TcpStream::TcpStream(std::shared_ptr<TcpStreamIf> const& client, int const fd) : Socket(TCP, fd), client(client) {
            makeNonBlocking();
      }

      TcpStream::~TcpStream() {
            std::lock_guard<std::mutex> syncWrite(writeLock);
            std::lock_guard<std::mutex> syncRead(readLock);
            client->disconnected();
            client = nullptr;
            Engine::cancelTimer(activity);
            Engine::cancelTimer(egress);
      }

      void TcpStream::handleRead() {
            std::lock_guard<std::mutex> sync(readLock);
            for(; ;) {
                  Bytes data(MAX_PACKET_SIZE);
                  auto const actuallyRead = read(data);
                  if(actuallyRead > 0) {
                        counters.notifyIngress(actuallyRead);
                        Engine::setTimer(activity, inactivityTimeout);
                        if(client) {
                              client->received(data);
                        }
                   } else if(actuallyRead == 0) {
                        break;
                  } else if(actuallyRead == -1) {
                        break;
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
                  auto const start = SteadyClock::now();
                  ssize_t totalWritten = 0;
                  bool wasEmpty = (writeQueue.size() == 0);
                  for(; ;) {
                        if(writeQueue.size() == 0) {
                              break;
                        } else {
                              auto const data = writeQueue.front();
                              writeQueue.pop_front();
                              auto const actuallySent = write(data);
                              if(actuallySent >= 0) {
                                    if((actuallySent - data.size()) != 0) {
                                          writeQueue.push_front(Bytes(data.begin() + actuallySent, data.end()));
                                    }
                                    counters.notifyEgress(actuallySent);
                                    Engine::setTimer(activity, inactivityTimeout);
                                    totalWritten += actuallySent;
                                    decltype(egressRate) elapsedNs = Clock::elapsed(start, SteadyClock::now()).count();
                                    if(elapsedNs <= 0) {
                                          elapsedNs = 1;
                                    };
                                    decltype(egressRate) currentRate = totalWritten * ONE_SEC_IN_NS / elapsedNs;
                                    if(egressRate != 0 && currentRate > egressRate) {
                                          writeTriggered = true;
                                          blocked = true;
                                          auto const nextWriteAt = MAX_PACKET_SIZE * ONE_SEC_IN_NS / egressRate;
                                          Engine::setTimer(egress, NanoSecs{nextWriteAt});
                                          break;
                                    } else {
                                          continue;
                                    }
                              } else if(actuallySent == -1) {
                                    writeQueue.push_front(data);
                                    blocked = true;
                                    break;
                              } else {
                                    break;
                              }
                        }
                  }
                  bool isEmpty = (writeQueue.size() == 0);
                  if(!once || (!wasEmpty && isEmpty)) {
                        once = true;
                        Engine::runAsync(notifyWriteComplete);
                  }
            } else {
                  connected = true;
                  Engine::setTimer(activity, inactivityTimeout);
                  if(writeQueue.size() > 0) {
                        writeTriggered = true;
                        Engine::triggerWrites(this);
                  } else {
                        Engine::runAsync(notifyWriteComplete);
                  }
            }
      }

      bool TcpStream::waitingOutEvent() {
            std::lock_guard<std::mutex> sync(writeLock);
            return (blocked || !once || !connected) && !disconnecting && (!connected || egressRate == 0);
      }

      void TcpStream::asyncEgress() {
            std::lock_guard<std::mutex> sync(writeLock);
            assert(writeTriggered, "Cannot run without being triggered");
            Engine::triggerWrites(this);
      }

      void TcpStream::asyncWriteComplete() {
            if(client) {
                  client->writeComplete();
            }
      }

      void TcpStream::asyncDisconnect() {
            disconnect();
      }

      void TcpStream::handleError() {
            getLastError();
            disconnect();
      }

      void TcpStream::disconnect() {
            std::lock_guard<std::mutex> sync(writeLock);
            if(!disconnecting) {
                  disconnecting = true;
                  Engine::remove(self);
            }
      }

      void TcpStream::queueWrite(Bytes const& data) {
            std::lock_guard<std::mutex> sync(writeLock);
            if(data.size() == 0) {
                  return;
            }
            writeQueue.push_back(data);
            if(connected && !blocked && !writeTriggered) {
                  writeTriggered = true;
                  Engine::triggerWrites(this);
            }
      }

      bool TcpStream::writeQueueEmpty() {
            std::lock_guard<std::mutex> sync(writeLock);
            return writeQueue.size() == 0;
      }

      bool TcpStream::didConnect() const {
            return connected;
      }

      InetDest TcpStream::endPoint() const {
            return originalDestination();
      }
}

