#include "udpclient.hpp"
#include "utils.hpp"
#include "types.hpp"

namespace Sb {
	UdpClient::UdpClient(const std::string& address, const uint16_t port, const Bytes& initialData)
		: Socket(Socket::createUdpSocket()) {
		(void) address;
		logDebug(std::string("UdpClient::UdpClient " + intToString(port)));
		pErrorThrow(fd);
//		dest = destFromString(address, port);
		// todo make async
		sendDatagram(initialData, dest);
	}

	UdpClient::~UdpClient() {
		logDebug("UdpClient::~UdpClient()");
	}

	void UdpClient::handleRead() {
		logDebug("UdpClient::handleRead");
		Bytes data(MAX_PACKET_SIZE);
		InetDest from = { {} , 0 };
		const auto actuallyReceived = receiveDatagram(data, from);
		if(actuallyReceived < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
			logDebug("UdpClient::handleRead would block");
			return;
		}
		sendDatagram(data, from);
	}

	void UdpClient::handleWrite() {

	}

	void UdpClient::handleError() {

	}

	void UdpClient::handleTimer(const size_t) {

	}
}
