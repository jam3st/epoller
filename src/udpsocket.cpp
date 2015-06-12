#include "udpsocket.hpp"
#include "engine.hpp"

namespace Sb {
      void UdpSocket::create(uint16_t const localPort, std::shared_ptr<UdpSocketIf> const& client) {
            std::shared_ptr<UdpSocket> ref = std::make_shared<UdpSocket>(client);
            ref->bindAndAdd(ref, localPort, client);
      }

      void UdpSocket::create(InetDest const& dest, std::shared_ptr<UdpSocketIf> const& client) {
            std::shared_ptr<UdpSocket> ref = std::make_shared<UdpSocket>(client);
            ref->connectAndAdd(ref, dest, client);
      }

      UdpSocket::UdpSocket(std::shared_ptr<UdpSocketIf> const& client) : Socket(UDP), client(client) {
            logDebug(std::string("UdpClient::UdpClient " + std::to_string(fd)));
            pErrorThrow(fd);
      }

      UdpSocket::~UdpSocket() {
      }

      void UdpSocket::connectAndAdd(std::shared_ptr<UdpSocket> const& me, InetDest const& dest, std::shared_ptr<UdpSocketIf> const& client) {
            client->udpSocket = me;
            makeTransparent();
            std::shared_ptr<Socket> sockRef = me;
            Engine::add(sockRef);
            connect(dest);
            client->connected(dest);
      }

      void UdpSocket::bindAndAdd(std::shared_ptr<UdpSocket> const& me, uint16_t const localPort, std::shared_ptr<UdpSocketIf> const& client) {
            client->udpSocket = me;
            makeTransparent();
            std::shared_ptr<Socket> sockRef = me;
            Engine::add(sockRef);
            bind(localPort);
      }

      void UdpSocket::handleRead() {
            std::lock_guard<std::mutex> sync(readLock);
            logDebug("UdpClient::handleRead");
            Bytes data(MAX_PACKET_SIZE);
            InetDest from = {{{}}, 0};
            const auto actuallyReceived = receiveDatagram(from, data);
            if (actuallyReceived < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
                  logDebug("UdpClient::handleRead would block");
                  return;
            }
            client->received(from, data);
      }

      void UdpSocket::queueWrite(const InetDest& dest, const Bytes& data) {
            logDebug("UdpSocket::queueWrite() " + dest.toString() + " " + std::to_string(fd));
            logDebug("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
            std::lock_guard<std::mutex> sync(writeLock);
            writeQueue.push_back(std::make_pair<const InetDest, const Bytes>(InetDest(dest), Bytes(data)));
            handleWrite();
      }

      void UdpSocket::disconnect() {
            logDebug("UdpSocket::disconnect() " + std::to_string(fd));
            Engine::remove(self);
      }

      void UdpSocket::doWrite(InetDest const& dest, Bytes const& data) {
            logDebug(std::string("UdpSocket::doWrite writing " + dest.toString() + " " + std::to_string(data.size())) + " " + std::to_string(fd));
            const auto actuallySent = sendDatagram(dest, data);
            logDebug(std::string("UdpSocket::doWrite actually wrote " + std::to_string(actuallySent)));
            pErrorLog(actuallySent, fd);
            if (actuallySent == -1) {
                  if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        logDebug(std::string("UdpSocket::doWrite would block"));
                        writeQueue.push_back(std::make_pair<const InetDest, const Bytes>(InetDest(dest), Bytes(data)));
                        return;
                  } else {
                        logDebug(std::string("UdpSocket::doWrite failed completely"));
                        client->notSent(dest, data);
                  }
            }
            decltype(actuallySent) dataLen = data.size();
            if (actuallySent == dataLen) {
                  logDebug("Write " + std::to_string(actuallySent) + " out of " + std::to_string(dataLen) + " on " + std::to_string(fd));
            } else if (actuallySent > 0) {
                  logDebug("Partial write of " + std::to_string(actuallySent) + " out of " + std::to_string(dataLen));
                  writeQueue.push_back(std::make_pair<const InetDest, const Bytes>(InetDest(dest), Bytes(data.begin() + actuallySent, data.end())));
            }
      }

      void UdpSocket::handleWrite() {
            logDebug("UdpSocket::handleWrite() " + std::to_string(writeQueue.size()) + " " + std::to_string(fd));
            for (; ;) {
                  if (writeQueue.size() == 0) {
                        logDebug("write queue is empty notifying client");
                        client->writeComplete();
                        return;
                  }
                  const auto data = writeQueue.front();
                  writeQueue.pop_front();
                  doWrite(data.first, data.second);
            }
      }

      bool UdpSocket::waitingOutEvent() {
            return false;
      }

      void UdpSocket::handleError() {
            logDebug("UdpSocket::handleError() is closed");
            client->disconnected();
      }
}
