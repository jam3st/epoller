#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "socket.hpp"

namespace Sb {
      constexpr int SO_ORIGINAL_DST = 80;
      typedef union {
            struct sockaddr_in6 addrIn6;
            struct sockaddr_in addrIn;
            struct sockaddr addr;
      } SocketAddress;

      Socket::Socket(SockType const type, const int fd) : type(type), fd(fd) {
            assert(fd > 0, "Unitialized fd");
            makeNonBlocking();
      }

      Socket::Socket(SockType const type) : type(type), fd(createSocket(type)) {
            assert(fd > 0, "Unitialized fd");
            makeNonBlocking();
      }

      Socket::~Socket() {
            ::close(fd);
      }

      bool Socket::waitingOutEvent() {
            return false;
      }

      void Socket::handleRead() {
            assert(false, "Socket::handleRead()");
      }

      void Socket::handleWrite() {
            assert(false, "Socket::handleWrite()");
      }

      void Socket::handleError() {
            assert(false, "Socket::handleError()");
      }

      void Socket::makeNonBlocking() const {
            auto flags = ::fcntl(fd, F_GETFL, 0);
            pErrorThrow(flags, fd);
            flags |= O_NONBLOCK;
            pErrorThrow(::fcntl(fd, F_SETFL, flags), fd);
      }

      int Socket::createSocket(Socket::SockType const type) {
            int fd = ::socket(AF_INET6, type == UDP ? SOCK_DGRAM : SOCK_STREAM | SOCK_NONBLOCK, 0);
            pErrorThrow(fd, fd);
            int on = 1;
            switch (type) {
                  case TCP:
                        pErrorLog(::setsockopt(fd, SOL_TCP, TCP_NODELAY, &on, sizeof(on)), fd);
                        break;
                  case UDP:
                        pErrorLog(::setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)), fd);
                        pErrorLog(::setsockopt(fd, SOL_TCP, SO_ORIGINAL_DST, &on, sizeof(on)), fd);
                        break;
            }
            return fd;
      }

      void Socket::reuseAddress() const {
            const int yes = 1;
            pErrorThrow(::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes), fd);
      }

      ssize_t Socket::convertFromStdError(ssize_t const error) const {
            if (error >= 0) {
                  return error;
            } else if (error == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)) {
                  return -1;
            } else {
                  logError("errno " + std::to_string(errno));
                  return -2;
            }
      }

      ssize_t Socket::read(Bytes& data) const {
            int numRead = ::read(fd, &data[0], data.size());
            if (numRead >= 0 && (data.size() - numRead) > 0) {
                  data.resize(numRead);
            }
            return convertFromStdError(numRead);
      }

      ssize_t Socket::write(Bytes const& data) const {
            return convertFromStdError(::write(fd, &data[0], data.size()));
      }

      void Socket::listen() const {
            pErrorThrow(::listen(fd, LISTEN_MAX_PENDING), fd);
      }

      int Socket::accept() const {
            SocketAddress addr;
            socklen_t addrLen = sizeof addr.addrIn6;
            int aFd = ::accept(fd, &addr.addr, &addrLen);
            assert(addrLen == sizeof addr.addrIn6, "Buggy address len");
            return convertFromStdError(aFd);
      }

      int Socket::receiveDatagram(InetDest& whereFrom, Bytes& data) const {
            struct iovec iovec[]{{&data[0], data.size()}};
            uint8_t msgHeader[1024];
            SocketAddress addr;
            struct msghdr msg{&addr, sizeof(addr), &iovec[0], sizeof(iovec) / sizeof(iovec[0]), &msgHeader[0], sizeof(msgHeader) / sizeof(msgHeader[0]), 0};
            const auto numReceived = ::recvmsg(fd, &msg, 0);
            if (numReceived >= 0 && (data.size() - numReceived) > 0) {
                  data.resize(numReceived);
            }
            pErrorLog(numReceived, fd);

            struct cmsghdr* cmsg;
            for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr && cmsg->cmsg_level >= 0; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
                  if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO) {
                        auto const& pktInfo = *reinterpret_cast<struct in6_pktinfo*>(CMSG_DATA(cmsg));
                        std::array<uint8_t, IP_ADDRESS_BIT_LEN / NUM_BITS_PER_SIZE_T> fromNetOrder;
                        ::memcpy(&fromNetOrder, &pktInfo.ipi6_addr, sizeof(fromNetOrder));
                        InetDest x;
                        x.addr.set(fromNetOrder);
                        x.ifIndex = pktInfo.ipi6_ifindex;
                        logError("bound address if: " + std::to_string(x.ifIndex) + " " + x.toString());
                  }
            }
            std::array<uint8_t, IP_ADDRESS_BIT_LEN / NUM_BITS_PER_SIZE_T> fromNetOrder;
            ::memcpy(&fromNetOrder, &addr.addrIn6.sin6_addr, sizeof(fromNetOrder));
            whereFrom.addr.set(fromNetOrder);
            whereFrom.port = networkEndian(addr.addrIn6.sin6_port);
            return numReceived;
      }
      //            socklen_t addrLen = sizeof(addr.addrIn6);
      //            const auto numReceived = ::recvfrom(fd, &data[0], data.size(), 0, &addr.addr, &addrLen);
      //            pErrorLog(numReceived, fd);
      //            std::array<uint8_t, IP_ADDRESS_BIT_LEN / NUM_BITS_PER_SIZE_T> fromNetOrder;
      //            assert(addrLen == sizeof(addr.addrIn6), "Buggy address len");
      //            ::memcpy(&fromNetOrder, &addr.addrIn6.sin6_addr, sizeof(fromNetOrder));
      //            whereFrom.addr.set(fromNetOrder);
      //            whereFrom.port = networkEndian(addr.addrIn6.sin6_port);
      //            logDebug("Datagram from " + addressToString(addr) + " " + std::to_string(whereFrom.port));
      //            if (numReceived >= 0 && data.size() - numReceived > 0) {
      //                  data.resize(numReceived);
      //            }
      //            return numReceived;

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

      void Socket::bind(uint16_t const port) const {
            makeNonBlocking();
            SocketAddress addr;
            addr.addrIn6.sin6_family = AF_INET6;
            addr.addrIn6.sin6_port = networkEndian(port);
            addr.addrIn6.sin6_addr = IN6ADDR_ANY_INIT;
            pErrorThrow(::bind(fd, &addr.addr, sizeof addr.addrIn6), fd);
      }

      int Socket::connect(InetDest const& whereTo) const {
            makeNonBlocking();
            SocketAddress addr;
            addr.addrIn6.sin6_family = AF_INET6;
            addr.addrIn6.sin6_port = networkEndian(whereTo.port);
            auto destAddrNet = whereTo.addr.get();
            ::memcpy(&addr.addrIn6.sin6_addr, &destAddrNet, sizeof addr.addrIn6.sin6_addr);
            return convertFromStdError(::connect(fd, &addr.addr, sizeof addr.addrIn6));
      }

      int Socket::getLastError() const {
            socklen_t len = sizeof(errno);
            auto ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &errno, &len);
            pErrorLog(errno, fd);
            return ret;
      }

      InetDest Socket::destFromString(const std::string& where, const uint16_t port) {
            InetDest dest;
            dest.port = port;
            dest.valid = false;
            std::array<uint8_t, IP_ADDRESS_BIT_LEN / NUM_BITS_PER_SIZE_T> addr;
            if (inet_pton(AF_INET, &where[0], &addr) > 0) {
                  std::string af6Dest = "::ffff:" + where;
                  dest.valid = inet_pton(AF_INET6, &af6Dest[0], &addr) > 0;
            } else if (!dest.valid) {
                  dest.valid = inet_pton(AF_INET6, &where[0], &addr) > 0;
            }
            dest.addr.set(addr);
            return dest;
      }

      void Socket::makeTransparent() const {
            int const on = 1;
            pErrorLog(setsockopt(fd, SOL_IP, IP_TRANSPARENT, &on, sizeof(on)), fd);
      }

      InetDest Socket::originalDestination() const {
            assert(type == TCP, "Only valid for TCP");
            InetDest from;
            SocketAddress addr;

            socklen_t addrLen = sizeof(addr.addr);
            auto ret = ::getsockopt(fd, SOL_IP, SO_ORIGINAL_DST, &addr.addr, &addrLen);
            if (ret == 0) {
                  assert(addrLen == sizeof(addr.addrIn), "Buggy address len");
                  from.addr.setIpV4(addr.addrIn.sin_addr.s_addr);
                  from.port = networkEndian(addr.addrIn.sin_port);
                  from.valid = true;
            } else {
                  socklen_t addrLen = sizeof addr.addrIn6;
                  ret = ::getsockopt(fd, SOL_IPV6, SO_ORIGINAL_DST, &addr, &addrLen);
                  if(ret == 0) {
                        std::array<uint8_t, IP_ADDRESS_BIT_LEN / NUM_BITS_PER_SIZE_T> fromNetOrder;
                        assert(addrLen == sizeof(addr.addrIn6), "Buggy address len");
                        ::memcpy(&fromNetOrder, &addr.addrIn6.sin6_addr, sizeof(fromNetOrder));
                        from.addr.set(fromNetOrder);
                        from.port = networkEndian(addr.addrIn6.sin6_port);
                        from.valid = true;
                  }
            }
            if (ret < 0) {
                  pErrorLog(ret, fd);
                  from.valid = false;
            }
            return from;
      }

      void Socket::onReadComplete() {
            assert(false, "Implement onReadComplete");
      }

      void Socket::onWriteComplete() {
            assert(false, "Implement onWriteComplete");
      }
}
