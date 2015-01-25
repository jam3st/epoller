#include "udpserver.hpp"
#include "utils.hpp"
#include "types.hpp"

namespace Sb {
	UdpServer::UdpServer(const uint16_t port)
		: Epoll(Socket::createUdpSocket()) {
		logDebug(std::string("UdpServer::UdpServer " + intToString(port)));
		reuseAddress();
		bind(port);
	}

	UdpServer::~UdpServer() {
		logDebug("UdpServer::~UdpServer()");
	}

	void UdpServer::handleRead() {
		logDebug("UdpServer::handleRead");
		Bytes data(MAX_PACKET_SIZE);
		InetDest from ;
		const auto actuallyReceived = receiveDatagram(data, from);
		if(actuallyReceived < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
			logDebug("UdpServer::handleRead would block");
			return;
		}
		sendDatagram(data, from);
	}

	void UdpServer::handleWrite() {
		logDebug("UdpServer::handleWrite()");
	}

	void UdpServer::handleError() {
		logDebug("TcpConnector::handleError ");
		// TODO
	}

	void UdpServer::handleTimer(int timerId) {
		logDebug("UdpServer::handleTimer " + intToString(timerId));
		// TODO
	}
}
