#include "tcpconn.hpp"

namespace Sb {
	namespace TcpConnection {
		void create(const struct InetDest& dest, std::shared_ptr<TcpStreamIf> client) {
			auto ref = std::make_shared<TcpConn>(client, dest.port);
			logDebug("TcpConn::create() " + dest.toString() + " " + std::to_string(ref->getFd()));
			ref->connect(dest);
			Engine::add(ref);
		}

		void create(const std::string& dest, const uint16_t port, std::shared_ptr<TcpStreamIf> client) {
			InetDest inetDest = Socket::destFromString(dest, port);
			if(inetDest.valid) {
				create(inetDest, client);
			} else {
				auto ref = std::make_shared<TcpConn>(client, port);
				logDebug("TcpConn::create() " + dest + " " + std::to_string(ref->getFd()));
				std::shared_ptr<ResolverIf> res = ref;
				Engine::resolver().resolve(res, dest);
			}
		}
	}

	TcpConn::TcpConn(std::shared_ptr<TcpStreamIf> client, uint16_t port) : Socket(Socket::createTcpSocket()), client(client), port(port), added(false) {
		logDebug("TcpConn::TcpConn() " + std::to_string(fd));
	}

	TcpConn::~TcpConn() {
		logDebug("TcpConn::~TcpConn() " + std::to_string(fd));
		Engine::resolver().cancel(this);
		auto ref = client;
		if(ref != nullptr) {
			ref->disconnected();
		}
	}

	void
	TcpConn::handleRead() {
		logDebug("TcpConn::handleRead " + std::to_string(fd));
		//		createStream();
	}

	void TcpConn::createStream() {
		logDebug("TcpConn::createStream() " + intToHexString(client));
		auto ref = client;
		if(ref != nullptr) {
			struct InetDest blah;
			TcpStream::create(releaseFd(), blah, ref, true);
			Engine::remove(this, true);
			added = false;
			client = nullptr;
		}
	}

	void
	TcpConn::handleWrite() {
		logDebug("TcpConn::handleWrite " + std::to_string(fd));
		createStream();
	}

	void
	TcpConn::handleError() {
		logDebug("TcpConn::handleError " + std::to_string(fd));
		if(added) {
			added = false;
			Engine::remove(this);
		}
		auto ref = client;
		if(ref != nullptr) {
			ref->disconnected();
			client = nullptr;
		}
	}

	void
	TcpConn::resolved(const IpAddr& addr) {
		logDebug("resolved addrress");
		auto tmp = std::dynamic_pointer_cast<Socket>(ref());
		InetDest dest { addr, true, port };
		logDebug("resolved added " + dest.toString());
		connect(dest);
		Engine::add(tmp);
		added = true;
	}

	void
	TcpConn::error() {
		logDebug("TcpConn resolver error not connecting");
		handleError();
	}
}
