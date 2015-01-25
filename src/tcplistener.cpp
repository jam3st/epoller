#include "logger.hpp"
#include "tcplistener.hpp"
#include "tcpstream.hpp"
#include "utils.hpp"


namespace Sb {
	void TcpListener::create(const uint16_t port,
												   std::function<std::shared_ptr<TcpSockIface>()> clientFactory) {
		Engine::add(std::make_shared<TcpListener>(port, clientFactory));
	}

	TcpListener::TcpListener(const uint16_t port, std::function<std::shared_ptr<TcpSockIface>()> clientFactory)
		: Epoll(Socket::createTcpSocket()),
				clientFactory(clientFactory) {
		logDebug(std::string("TcpListener::TcpListener " + intToString(port)));
		reuseAddress();
		bind(port);
		listen();
	}

	TcpListener::~TcpListener() {
		logDebug("TcpListener::~TcpListener()");
	}

	void TcpListener::handleRead() {
		logDebug("TcpListener::handleRead");
		const auto connFd = accept();
		pErrorLog(connFd);
		if(connFd < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
			logDebug("TcpListener::handleRead would block");
			return;
		}
		createStream(connFd);
	}

	void TcpListener::handleWrite() {
		logDebug("Epollable::handleWrite()");
	}

	void TcpListener::handleError() {
		logDebug("Epollable::handleWrite()");
	}

	void TcpListener::handleTimer(int timerId) {
		logDebug("Epollable::handleTimer() " + intToString(timerId));
	}


	void TcpListener::createStream(const int connFd) {
		logDebug("TcpListener::createStream " + intToString(connFd));
		if(connFd < 0) {
			logError("Invalid connection fd in TcpListener::createStream");
			return;
		}
		struct InetDest blah;
		TcpStream::create(connFd, blah, clientFactory());
	}
}
