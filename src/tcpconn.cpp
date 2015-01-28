#include "tcpconn.hpp"
#include "utils.hpp"
#include "types.hpp"

namespace Sb {
	void TcpConn::create(const struct InetDest& dest, std::function<std::shared_ptr<TcpStreamIf>()> clientFactory) {
		auto ref = std::make_shared<TcpConn>(clientFactory);
		Engine::add(ref);
		logDebug("TcpConn::create()");
		ref->connect(dest);
	}

	TcpConn::TcpConn(std::function<std::shared_ptr<TcpStreamIf>()> clientFactory)
		:	Socket(fd),
		  clientFactory(clientFactory) {
	}

	TcpConn::~TcpConn() {
		logDebug("TcpConn::~TcpConn()");
	}

	void TcpConn::handleRead() {
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

	void TcpConn::handleWrite() {
		logDebug("UdpServer::handleWrite()");
	}

	void TcpConn::handleError() {
		logDebug("TcpConnector::handleError ");
		// TODO
	}

	void TcpConn::handleTimer(const size_t timerId) {
		logDebug("UdpServer::handleTimer " + intToString(timerId));
		// TODO
	}
}
