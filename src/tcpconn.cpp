/*
This file is part of PoLLaX.

PoLLaX is free software: you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

PoLLaX is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with PoLLaX.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tcpconn.hpp"
#include "utils.hpp"
#include "types.hpp"

namespace Sb {
	namespace TcpConnection {
		void
		create(const struct InetDest& dest, std::shared_ptr<TcpStreamIf> client) {
			auto ref = std::make_shared<TcpConn>(client, dest.port);
			logDebug("TcpConn::create() " + dest.toString() + " " +  std::to_string(ref->getFd()));
			ref->connect(dest);
			Engine::add(ref);
		}

		void
		create(const std::string& dest, const uint16_t port, std::shared_ptr<TcpStreamIf> client) {
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

	TcpConn::TcpConn(std::shared_ptr<TcpStreamIf> client, uint16_t port)
		: Socket(Socket::createTcpSocket()),
		  client(client),
		  port(port) {
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
		logDebug("TcpConnector::handleError " + std::to_string(fd));
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
	TcpConn::handleTimer(const size_t timerId) {
		logDebug("TcpConn::handleTimer " + std::to_string(timerId) + " " + std::to_string(fd));
		// TODO Engine::resolver().cancel(this);
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
		logDebug("TcpConn resolver error, not connected");
		handleError();
	}
}
