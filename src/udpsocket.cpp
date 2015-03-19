#include "udpsocket.hpp"
#include "utils.hpp"
#include "types.hpp"
#include "engine.hpp"

namespace Sb {
	void UdpSocket::create(const uint16_t localPort, std::shared_ptr<UdpSocketIf> client) {
		auto ref = std::make_shared<UdpSocket>(client);
		Engine::add(ref);
		logDebug("UdpSocket::create() server");
		client->udpSocket = ref;
		ref->bind(localPort);
	}

	void UdpSocket::create(const struct InetDest& dest, std::shared_ptr<UdpSocketIf> client) {
		auto ref = std::make_shared<UdpSocket>(client);
		Engine::add(ref);
		logDebug("UdpSocket::create() client");
		ref->connect(dest);
		client->udpSocket = ref;
		client->connected(dest);
	}

	UdpSocket::UdpSocket(std::shared_ptr<UdpSocketIf> client)
		: Socket(Socket::createUdpSocket()),
		  client(client),
		  writeQueue(std::make_pair<const InetDest, const Bytes>({}, {})) {
		logDebug(std::string("UdpClient::UdpClient " + std::to_string(fd)));
		pErrorThrow(fd);
		Socket::makeNonBlocking(fd);
	}

	UdpSocket::~UdpSocket() {
		logDebug("UdpSocket::~UdpSocket() " + std::to_string(fd));
	}

	void UdpSocket::handleRead() {
		logDebug("UdpClient::handleRead");
		Bytes data(MAX_PACKET_SIZE);
		InetDest from = { {} , 0 };
		const auto actuallyReceived = receiveDatagram(from, data);
		if(actuallyReceived < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
			logDebug("UdpClient::handleRead would block");
			return;
		}
		client->received(from, data);
	}

	void UdpSocket::queueWrite(const InetDest& dest, const Bytes& data) {
		logDebug("UdpSocket::queueWrite() " + dest.toString() + " " + std::to_string(fd));
		std::lock_guard<std::mutex> sync(writeLock);
		auto wasEmpty = writeQueue.isEmpty();
		writeQueue.add(std::make_pair<const InetDest, const Bytes>(InetDest(dest), Bytes(data)));
		if(wasEmpty) {
			handleWrite();
		}
	}


	void UdpSocket::disconnect() {
		logDebug("UdpSocket::disconnect() my ref " + std::to_string(this->ref().use_count()) +
				 " their refs " + std::to_string(client.use_count()));
		Engine::remove(this);
	}

	void UdpSocket::doWrite(const InetDest& dest, const Bytes& data) {
		logDebug(std::string("UdpSocket::doWrite writing " + dest.toString() +  " " + std::to_string(data.size())) + " "  + std::to_string(fd));
		const auto actuallySent = sendDatagram(dest, data);
		logDebug(std::string("UdpSocket::doWrite actually wrote " + std::to_string(actuallySent)));
		pErrorLog(getFd());
		if(actuallySent == -1) {
			if(errno == EWOULDBLOCK || errno == EAGAIN) {
				logDebug(std::string("UdpSocket::doWrite would block"));
//add to front
				writeQueue.add(std::make_pair<const InetDest, const Bytes>(InetDest(dest), Bytes(data)));
				return;
			} else {
				logDebug(std::string("UdpSocket::doWrite failed completely"));
				client->notSent(dest, data);
			}

		}

		decltype(actuallySent) dataLen = data.size();
		if(actuallySent == dataLen) {
			logDebug("Write " + std::to_string(actuallySent) + " out of " + std::to_string(dataLen) + " on " + std::to_string(getFd()));
		} else if (actuallySent > 0) {
			logDebug("Partial write of " + std::to_string(actuallySent) + " out of " + std::to_string(dataLen));
//add to front
			writeQueue.add(std::make_pair<const InetDest, const Bytes>(InetDest(dest), Bytes(data.begin () + actuallySent, data.end ())));
		}
	}

	void UdpSocket::handleWrite() {
		logDebug("UdpSocket::handleWrite() " + std::to_string(writeQueue.len()) + " " + std::to_string(fd));
		for(;;) {
			bool isEmpty = true;
			auto& data = writeQueue.removeAndIsEmpty(isEmpty);
			if(isEmpty) {
				logDebug("write queue is empty notifying client");
				auto ref = this->ref();
				client->writeComplete();
				return;
			}
			doWrite(data.first, data.second);
		}
	}

	void UdpSocket::handleError() {
		logDebug("UdpSocket::handleError() is closed");
		client->disconnected();
	}

	void UdpSocket::handleTimer(const size_t) {

	}
}
