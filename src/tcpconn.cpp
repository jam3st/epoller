#include "tcpconn.hpp"
#include "utils.hpp"
#include "types.hpp"

namespace Sb {
	void TcpConn::create(const struct InetDest& dest, std::shared_ptr<TcpStreamIf> client) {
		auto ref = std::make_shared<TcpConn>(client);
		Engine::add(ref);
		logDebug("TcpConn::create()");
		ref->connect(dest);
	}

	TcpConn::TcpConn(std::shared_ptr<TcpStreamIf> client)
		: Socket(Socket::createTcpSocket()),
		  client(client),
		  writeQueue({}) {
	}

	TcpConn::~TcpConn() {
		logDebug("TcpConn::~TcpConn()");
	}

	void TcpConn::handleRead() {
		logDebug("TcpListener::handleRead");
	}

	void TcpConn::handleWrite() {
		logDebug("TcpConn::handleWrite");
		struct InetDest blah;
		TcpStream::create(releaseFd(), blah, client, true);
		Engine::remove(this, true);
		logDebug("TcpConn::handleWrite()");
	}

	void TcpConn::handleError() {
		logDebug("TcpConnector::handleError ");
		client->onError();
		Engine::remove(this);
		// TODO
	}

	void TcpConn::handleTimer(const size_t timerId) {
		logDebug("TcpConn::handleTimer " + std::to_string(timerId));
		// TODO
	}

}
