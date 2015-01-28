#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "socket.hpp"
#include "logger.hpp"


namespace Sb {
	typedef union {
		struct sockaddr_in6 addrIn6;
		struct sockaddr addr;
	} SocketAddress;

	static std::string inetAddressToString(const SocketAddress& addr);

	Socket::Socket(const int fd)
	 : fd(fd) {
		pErrorThrow(fd);
		logDebug("Socket::Socket()");
		Socket::makeSocketNonBlocking(fd);
	}

	Socket::~Socket() {
		logDebug("Socket::~Socket() closed and shutdown	" + intToString(fd));
		::close(fd);
	}

	int Socket::dupFd() const  {
		logDebug("Epollable::dupFd");
		auto dup = ::dup(fd);
		pErrorThrow(dup);
		return dup;
	}

	int Socket::getFd() const {
		return fd;
	}


	void Socket::makeSocketNonBlocking(const int fd) {
		auto flags = ::fcntl(fd, F_GETFL, 0);
		pErrorThrow(flags);
		flags |= O_NONBLOCK;
		pErrorThrow(::fcntl(fd, F_SETFL, flags));
	}

	int Socket::createSocket(const Socket::SockType type) {
		int fd = ::socket(AF_INET6, type == UDP ? SOCK_DGRAM : SOCK_STREAM  | SOCK_NONBLOCK, 0);
		makeSocketNonBlocking(fd);
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
		logDebug("Asked to read with buffer size of  "  + intToString(data.size ()));
		int numRead = ::read(fd, &data[0], data.size());
		pErrorLog(numRead);
		logDebug("Read "  + intToString(numRead));
		if(numRead >= 0 && (data.size() - numRead) > 0) {
			logDebug("Resize "  + intToString(numRead));
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
		auto accepteFd = ::accept(fd, &addr.addr, &addrLen);
		assert(addrLen == sizeof addr.addrIn6, "Buggy address len");
		logDebug("New connection from "  + inetAddressToString(addr));
		return accepteFd;
	}

	int Socket::receiveDatagram(Bytes& data, InetDest& whereFrom) const {
		SocketAddress addr;
		socklen_t addrLen = sizeof addr.addrIn6;
		const auto numReceived = ::recvfrom(fd, &data[0], data.size(), 0, &addr.addr, &addrLen);
		pErrorLog(numReceived);
		assert(addrLen == sizeof addr.addrIn6, "Buggy address len");
		::memcpy(&whereFrom.addr, &addr.addrIn6.sin6_addr, sizeof whereFrom.addr);
		whereFrom.port = ntohs(addr.addrIn6.sin6_port);
		logDebug("Datagram from: " + inetAddressToString(addr) + " " + intToString(whereFrom.port));

		if(numReceived >= 0 && data.size() - numReceived > 0) {
			data.resize(numReceived);
		}
		return numReceived;
	}

	int Socket::sendDatagram(const Bytes& data, const InetDest& whereTo) const {
		SocketAddress addr;
		addr.addrIn6.sin6_family = AF_INET6;
		addr.addrIn6.sin6_port = htons(whereTo.port);
		::memcpy(&addr.addrIn6.sin6_addr, &whereTo.addr, sizeof addr.addrIn6.sin6_addr);
		const auto numSent = ::sendto(fd, &data[0], data.size(), 0, &addr.addr, sizeof addr.addrIn6);
		pErrorLog(numSent);
		logDebug("sent " + intToString(numSent));
		return numSent;
	}

	void Socket::bind(const uint16_t port) const {
		SocketAddress addr;
		addr.addrIn6.sin6_family = AF_INET6;
		addr.addrIn6.sin6_port = htons(port);
		addr.addrIn6.sin6_addr = IN6ADDR_ANY_INIT;
		pErrorThrow(::bind(fd, &addr.addr, sizeof addr.addrIn6));
	}

	void Socket::connect(const InetDest& whereTo) const {
		SocketAddress addr;
		addr.addrIn6.sin6_family = AF_INET6;
		addr.addrIn6.sin6_port = htons(whereTo.port);
		::memcpy(&addr.addrIn6.sin6_addr, &whereTo.addr, sizeof addr.addrIn6.sin6_addr);
		::connect(fd, &addr.addr, sizeof addr.addrIn6);
	}

	InetDest Socket::destFromString(const std::string& where, const uint16_t port) {
		InetDest dest;
		dest.port = port;
		inet_pton(AF_INET6, &where[0], &dest.addr);
		return dest;
	}

	static std::string inetAddressToString(const SocketAddress& addr) {
		char buf[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &addr.addrIn6.sin6_addr, buf, sizeof buf);
		return std::string(buf);

	}
}
