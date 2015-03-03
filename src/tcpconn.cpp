#include "tcpconn.hpp"
#include "utils.hpp"
#include "types.hpp"

namespace Sb {
	namespace TcpConnection {
		void
		create(const struct InetDest& dest, std::shared_ptr<TcpStreamIf> client) {
			auto ref = std::make_shared<TcpConn>(client, dest.port);
			logDebug("TcpConn::create() " + dest.toString());
logDebug("resolved added " + dest.toString());
			ref->connect(dest);
			Engine::add(ref);
		}

		void create(const std::string& dest, const uint16_t port, std::shared_ptr<TcpStreamIf> client) {
			logDebug("TcpConn::create() " + dest);
			auto ref = std::make_shared<TcpConn>(client, port);
			std::shared_ptr<ResolverIf> res = ref;
			Engine::resolver().resolve(res, dest);
		}
	}

	TcpConn::TcpConn(std::shared_ptr<TcpStreamIf> client, uint16_t port)
		: Socket(Socket::createTcpSocket()),
		  client(client),
		  port(port) {
		logDebug("TcpConn::TcpConn() " + std::to_string(fd));

	}

	TcpConn::~TcpConn() {
		logDebug("TcpConn::~TcpConn() " + std::to_string(fd));
		Engine::resolver().cancel(this);
	}

	void
	TcpConn::handleRead() {
		logDebug("TcpListener::handleRead " + std::to_string(fd));
	}

	void
	TcpConn::handleWrite() {
		logDebug("TcpConn::handleWrite " + std::to_string(fd));
		struct InetDest blah;
		TcpStream::create(releaseFd(), blah, client, true);
		Engine::remove(this, true);
		logDebug("TcpConn::handleWrite()");
	}

	void
	TcpConn::handleError() {
		logDebug("TcpConnector::handleError " + std::to_string(fd));
		client->disconnected();
		Engine::remove(this);
		// TODO
	}

	void
	TcpConn::handleTimer(const size_t timerId) {
		logDebug("TcpConn::handleTimer " + std::to_string(timerId) + " " + std::to_string(fd));
		// TODO
	}

	void
	TcpConn::resolved(const IpAddr& addr) {
		logDebug("resolved addrress");
		auto tmp = std::dynamic_pointer_cast<Socket>(ref());
		InetDest dest { addr, port };
logDebug("resolved added " + dest.toString());
		connect(dest);
		Engine::add(tmp);
	}

	void
	TcpConn::error() {
logDebug("TcpConn error");
	}
}
