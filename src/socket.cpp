#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "socket.hpp"
#include "logger.hpp"
#include "endians.hpp"


namespace Sb {
	typedef union {
		struct sockaddr_in6 addrIn6;
		struct sockaddr addr;
	} SocketAddress;

	static std::string addressToString(const SocketAddress &addr);

	Socket::Socket(const int fd)
	 : fd(fd) {
		pErrorThrow(fd);
		assert(fd > 0, "Unitialised fd");
		logDebug("Socket::Socket()");
		Socket::makeNonBlocking(fd);
	}

	Socket::~Socket() {
		logDebug("Socket::~Socket() closed and shutdown " + std::to_string(fd));
		if(fd >= 0) {
			logDebug("Socket::~Socket() closed fd " + std::to_string(fd));
			::close(fd);
		}
	}

	int Socket::getFd() const {
		return fd;
	}

	int Socket::releaseFd() const {
		int ret = fd;
		fd = -1;
		return ret;
	}

	void Socket::makeNonBlocking(const int fd) {
		auto flags = ::fcntl(fd, F_GETFL, 0);
		pErrorThrow(flags);
		flags |= O_NONBLOCK;
		pErrorThrow(::fcntl(fd, F_SETFL, flags));
	}

	int Socket::createSocket(const Socket::SockType type) {
		int fd = ::socket(AF_INET6, type == UDP ? SOCK_DGRAM : SOCK_STREAM  | SOCK_NONBLOCK, 0);
		makeNonBlocking(fd);
		return fd;
	}

	int Socket::createUdpSocket() {
		return createSocket(Socket::UDP);
	}

	int Socket::createTcpSocket() {
		return createSocket(Socket::TCP);
	}

	void Socket::reuseAddress() const {
		const int yes = 1;
		pErrorThrow(::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes));
	}

	int Socket::read(Bytes& data) const {
		logDebug("Asked to read with buffer size of  "  + std::to_string(data.size()) + " on " + std::to_string(fd));
		int numRead = ::read(fd, &data[0], data.size());
		pErrorLog(numRead);
		logDebug("Read "  + std::to_string(numRead) + " on " + std::to_string(fd));
		if(numRead >= 0 && (data.size() - numRead) > 0) {
			logDebug("Resize "  + std::to_string(numRead));
			data.resize(numRead);
		}
		return numRead;
	}

	int Socket::write(const Bytes& data) const {
		int numWritten = ::write(fd, &data[0], data.size());
		pErrorLog(numWritten);
		return numWritten;
	}

	void Socket::listen() const {
		pErrorThrow(::listen(fd, LISTEN_MAX_PENDING));
	}

	int	Socket::accept() const {
		SocketAddress addr;
		socklen_t addrLen = sizeof addr.addrIn6;
		int aFd = ::accept(fd, &addr.addr, &addrLen);
		assert(addrLen == sizeof addr.addrIn6, "Buggy address len");
		logDebug("Accept connection from "  + addressToString(addr) + " on fd " + std::to_string(aFd));
		return aFd;
	}

	int Socket::receiveDatagram(InetDest& whereFrom, Bytes& data) const {
		SocketAddress addr;
		socklen_t addrLen = sizeof addr.addrIn6;
		const auto numReceived = ::recvfrom(fd, &data[0], data.size(), 0, &addr.addr, &addrLen);
		pErrorLog(numReceived);
		assert(addrLen == sizeof addr.addrIn6, "Buggy address len");
		::memcpy(&whereFrom.addr, &addr.addrIn6.sin6_addr, sizeof whereFrom.addr);
		whereFrom.port = networkEndian(addr.addrIn6.sin6_port);
		logDebug("Datagram from: " + addressToString(addr) + " " + std::to_string(whereFrom.port));

		if(numReceived >= 0 && data.size() - numReceived > 0) {
			data.resize(numReceived);
		}
		return numReceived;
	}

	int Socket::sendDatagram(const InetDest& whereTo, const Bytes& data) const {
		SocketAddress addr;
		addr.addrIn6.sin6_family = AF_INET6;
		addr.addrIn6.sin6_port = networkEndian(whereTo.port);
		::memcpy(&addr.addrIn6.sin6_addr, &whereTo.addr, sizeof addr.addrIn6.sin6_addr);
		const auto numSent = ::sendto(fd, &data[0], data.size(), 0, &addr.addr, sizeof addr.addrIn6);
		pErrorLog(numSent);
		logDebug("sent " + std::to_string(numSent));
		return numSent;
	}

	void Socket::bind(const uint16_t port) const {
		makeNonBlocking(fd);
		SocketAddress addr;
		addr.addrIn6.sin6_family = AF_INET6;
		addr.addrIn6.sin6_port = networkEndian(port);
		addr.addrIn6.sin6_addr = IN6ADDR_ANY_INIT;
		pErrorThrow(::bind(fd, &addr.addr, sizeof addr.addrIn6));
	}

	void Socket::connect(const InetDest& whereTo) const {
		makeNonBlocking(fd);
		SocketAddress addr;
		addr.addrIn6.sin6_family = AF_INET6;
		addr.addrIn6.sin6_port = networkEndian(whereTo.port);
		::memcpy(&addr.addrIn6.sin6_addr, &whereTo.addr, sizeof addr.addrIn6.sin6_addr);
		::connect(fd, &addr.addr, sizeof addr.addrIn6);
		logDebug("Connect to "  + addressToString(addr) + ":" + std::to_string(whereTo.port) + " on fd " + std::to_string(fd));
	}

	InetDest Socket::destFromString(const std::string& where, const uint16_t port) {
		InetDest dest;
		dest.port = port;
		dest.valid = false;
		if(inet_pton(AF_INET, &where[0], &dest.addr) > 0) {
			std::string af6Dest = "::ffff:" + where;
			dest.valid = inet_pton(AF_INET6, &af6Dest[0], &dest.addr) > 0;
		} else if(!dest.valid) {
			dest.valid = inet_pton(AF_INET6, &where[0], &dest.addr) > 0;
		}
		return dest;
	}

	std::string addressToString(const SocketAddress &addr) {
		char buf[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &addr.addrIn6.sin6_addr, buf, sizeof buf);
		return std::string(buf);
	}
}
