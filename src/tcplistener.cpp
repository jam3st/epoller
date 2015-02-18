﻿#include <string>

#include "logger.hpp"
#include "tcplistener.hpp"
#include "tcpstream.hpp"
#include "utils.hpp"



namespace Sb {
	void TcpListener::create(const uint16_t port, std::function<std::shared_ptr<TcpStreamIf>()> clientFactory) {
		//Engine::add(std::make_shared<TcpListener>(port, clientFactory));
		auto tmp = std::make_shared<TcpListener>(port, clientFactory);
		Engine::add(tmp);
//		logDebug("TcpListener::create fd "	+ std::to_string(tmp->getFd()));
	}

	TcpListener::TcpListener(const uint16_t port, std::function<std::shared_ptr<TcpStreamIf>()> clientFactory)
		: Socket(Socket::createTcpSocket()),
				clientFactory(clientFactory) {
		logDebug(std::string("TcpListener::TcpListener " + std::to_string(port)));
		reuseAddress();
		bind(port);
		listen();
	}

	TcpListener::~TcpListener() {
		logDebug("TcpListener::~TcpListener()");
	}

	void TcpListener::handleRead() {
		logDebug("TcpListener::handleRead");
		const int connFd = accept();
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

	void TcpListener::handleTimer(const size_t timerId) {
		logDebug("Epollable::handleTimer() " + std::to_string(timerId));
	}

	void TcpListener::createStream(const int connFd) {
		logDebug("TcpListener::createStream " + std::to_string(connFd));
		if(connFd < 0) {
			logError("Invalid connection fd in TcpListener::createStream");
			return;
		}
		// TODO: FIX DEST
		struct InetDest blah;
		TcpStream::create(connFd, blah, clientFactory());
	}
}
