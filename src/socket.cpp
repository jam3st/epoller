#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "socket.hpp"
#include "endians.hpp"

namespace Sb {
      typedef union {
            struct sockaddr_in6 addrIn6;
            struct sockaddr addr;
      } SocketAddress;

      static std::string addressToString(const SocketAddress& addr);

      Socket::Socket(int const fd) : fd(fd) {
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

      bool Socket::fdReleased() const {
            return fd == -1;
      }

      void Socket::makeNonBlocking(const int fd) {
            auto flags = ::fcntl(fd, F_GETFL, 0);
            pErrorThrow(flags, fd);
            flags |= O_NONBLOCK;
            pErrorThrow(::fcntl(fd, F_SETFL, flags), fd);
      }

      int Socket::createSocket(const Socket::SockType type) {
            int fd = ::socket(AF_INET6, type == UDP ? SOCK_DGRAM : SOCK_STREAM | SOCK_NONBLOCK, 0);

            if(type == TCP) {
                  size_t one = 1;
                  setsockopt(fd, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
            }

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
            pErrorThrow(::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes), fd);
      }

      ssize_t Socket::convertError(ssize_t const error) const {
            if(error >= 0) {
                  return error;
            } else if (error == -1 && (errno == EINTR || errno == EAGAIN || error == EWOULDBLOCK)) {
                  return -1;
            } else {
                  return -2;
            }
      }

      ssize_t Socket::read(Bytes& data) const {
            int numRead = ::read(fd, &data[0], data.size());
            if(numRead >= 0 && (data.size() - numRead) > 0) {
                  data.resize(numRead);
            }
            return convertError(numRead);
      }

      ssize_t Socket::write(const Bytes& data) const {
            return convertError(::write(fd, &data[0], data.size()));
      }

      void Socket::listen() const {
            pErrorThrow(::listen(fd, LISTEN_MAX_PENDING), fd);
      }

      int Socket::accept() const {
            SocketAddress addr;
            socklen_t addrLen = sizeof addr.addrIn6;
            int aFd = ::accept(fd, &addr.addr, &addrLen);
            assert(addrLen == sizeof addr.addrIn6, "Buggy address len");
            return convertError(aFd);
      }

      int Socket::receiveDatagram(InetDest& whereFrom, Bytes& data) const {
            SocketAddress addr;
            socklen_t addrLen = sizeof(addr.addrIn6);
            const auto numReceived = ::recvfrom(fd, &data[0], data.size(), 0, &addr.addr, &addrLen);
            pErrorLog(numReceived, fd);
            std::array<uint8_t, IP_ADDRESS_BIT_LEN / NUM_BITS_PER_SIZE_T> fromNetOrder;
            assert(addrLen == sizeof(addr.addrIn6), "Buggy address len");
            ::memcpy(&fromNetOrder, &addr.addrIn6.sin6_addr, sizeof(fromNetOrder));
            whereFrom.addr.set(fromNetOrder);
            whereFrom.port = networkEndian(addr.addrIn6.sin6_port);
            logDebug("Datagram from " + addressToString(addr) + " " + std::to_string(whereFrom.port));

            if(numReceived >= 0 && data.size() - numReceived > 0) {
                  data.resize(numReceived);
            }

            return numReceived;
      }

      int Socket::sendDatagram(const InetDest& whereTo, const Bytes& data) const {
            SocketAddress addr;
            addr.addrIn6.sin6_family = AF_INET6;
            addr.addrIn6.sin6_port = networkEndian(whereTo.port);
            auto destAddrNet = whereTo.addr.get();
            ::memcpy(&addr.addrIn6.sin6_addr, &destAddrNet, sizeof addr.addrIn6.sin6_addr);
            const auto numSent = ::sendto(fd, &data[0], data.size(), 0, &addr.addr, sizeof addr.addrIn6);
            pErrorLog(numSent, fd);
            logDebug("sent " + std::to_string(numSent));
            return numSent;
      }

      void Socket::bind(const uint16_t port) const {
            logDebug("Socket::bind " + std::to_string(fd));
            makeNonBlocking(fd);
            SocketAddress addr;
            addr.addrIn6.sin6_family = AF_INET6;
            addr.addrIn6.sin6_port = networkEndian(port);
            addr.addrIn6.sin6_addr = IN6ADDR_ANY_INIT;
            pErrorThrow(::bind(fd, &addr.addr, sizeof addr.addrIn6), fd);
      }

      void Socket::connect(const InetDest& whereTo) const {
            makeNonBlocking(fd);
            SocketAddress addr;
            addr.addrIn6.sin6_family = AF_INET6;
            addr.addrIn6.sin6_port = networkEndian(whereTo.port);
            auto destAddrNet = whereTo.addr.get();
            ::memcpy(&addr.addrIn6.sin6_addr, &destAddrNet, sizeof addr.addrIn6.sin6_addr);
            ::connect(fd, &addr.addr, sizeof addr.addrIn6);
      }

      InetDest Socket::destFromString(const std::string& where, const uint16_t port) {
            InetDest dest;
            dest.port = port;
            dest.valid = false;

            std::array<uint8_t, IP_ADDRESS_BIT_LEN / NUM_BITS_PER_SIZE_T> addr;
            if(inet_pton(AF_INET, &where[0], &addr) > 0) {
                  std::string af6Dest = "::ffff:" + where;
                  dest.valid = inet_pton(AF_INET6, &af6Dest[0], &addr) > 0;
            } else if(!dest.valid) {
                  dest.valid = inet_pton(AF_INET6, &where[0], &addr) > 0;
            }
            dest.addr.set(addr);
            return dest;
      }

      std::string addressToString(const SocketAddress& addr) {
            char buf[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &addr.addrIn6.sin6_addr, buf, sizeof buf);
            return std::string(buf);
      }
}
