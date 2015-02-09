#include "udpsocket.hpp"
#include "utils.hpp"
#include "types.hpp"
#include "engine.hpp"

namespace Sb {
	void UdpSocket::create(const uint16_t localPort, std::shared_ptr<UdpSocketIf> client) {
		auto ref = std::make_shared<UdpSocket>(client);
		Engine::add(ref);
		logDebug("UdpSocket::create() server");
		ref->bind(localPort);
	}

	void UdpSocket::create(const struct InetDest& dest, std::shared_ptr<UdpSocketIf> client) {
		auto ref = std::make_shared<UdpSocket>(client);
		Engine::add(ref);
		logDebug("UdpSocket::create() client");
		ref->connect(dest);
		client->onConnect(*ref, dest);
	}

	UdpSocket::UdpSocket(std::shared_ptr<UdpSocketIf> client)
		: Socket(Socket::createUdpSocket()),
		  client(client) {
		logDebug(std::string("UdpClient::UdpClient " + intToString(fd)));
		pErrorThrow(fd);
		Socket::makeNonBlocking(fd);
	}

	UdpSocket::~UdpSocket() {
		logDebug("UdpSocket::~UdpSocket()");
	}

	void UdpSocket::handleRead() {
		logDebug("UdpClient::handleRead");
		Bytes data(MAX_PACKET_SIZE);
		InetDest from = { {} , 0 };
		const auto actuallyReceived = receiveDatagram(data, from);
		if(actuallyReceived < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
			logDebug("UdpClient::handleRead would block");
			return;
		}
		client->onReadCompleted(*this, from, data);
	}

	void UdpSocket::queueWrite(const Bytes& data, const InetDest& dest) {
		sendDatagram(data, dest);
	}

	void UdpSocket::handleWrite() {

	}

	void UdpSocket::handleError() {

	}

	void UdpSocket::handleTimer(const size_t) {

	}
}
