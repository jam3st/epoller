#include "udpsocket.hpp"
#include "engine.hpp"

namespace Sb {
      void UdpSocket::create(const uint16_t localPort, std::shared_ptr<UdpSocketIf> client) {
            auto ref = std::make_shared<UdpSocket>(client);
            logDebug("UdpSocket::create() server");
            client->udpSocket = ref;
            ref->bind(localPort);
            Engine::add(ref);
      }

      void UdpSocket::create(const struct InetDest& dest, std::shared_ptr<UdpSocketIf> client) {
            auto ref = std::make_shared<UdpSocket>(client);
            logDebug("UdpSocket::create() client");
            ref->connect(dest);
            client->udpSocket = ref;
            ref->makeTransparent();
            client->connected(dest);
            Engine::add(ref);
      }

      UdpSocket::UdpSocket(std::shared_ptr<UdpSocketIf> client) : Socket(UDP), client(client) {
            logDebug(std::string("UdpClient::UdpClient " + std::to_string(fd)));
            pErrorThrow(fd);
      }

      UdpSocket::~UdpSocket() {
            logDebug("UdpSocket::~UdpSocket() " + std::to_string(fd));
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

      void UdpSocket::doWrite(const InetDest& dest, const Bytes& data) {
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
