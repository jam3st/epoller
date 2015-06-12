#pragma once
#include <string>
#include <vector>
#include <array>
#include <iomanip>
#include <sstream>
#include "endians.hpp"

#include <iostream>
namespace Sb {
      typedef uint64_t bitsPerSecond;
      typedef uint8_t byte;
      typedef std::vector<byte> Bytes;
      constexpr ssize_t MAX_PACKET_SIZE = 4096;
      constexpr ssize_t IP_ADDRESS_BIT_LEN = 128;
      constexpr size_t NUM_BITS_PER_SIZE_T = 8;
      constexpr size_t ADDR_LEN_SIZE_T = IP_ADDRESS_BIT_LEN / sizeof(uint16_t) / NUM_BITS_PER_SIZE_T;

      struct IpAddr {
            size_t const IP4_PREFIX_LEN_SIZE_T = IP_ADDRESS_BIT_LEN / sizeof(uint16_t) / NUM_BITS_PER_SIZE_T - sizeof(uint32_t) / sizeof(uint16_t);
            std::array<uint16_t, ADDR_LEN_SIZE_T> const ip4Prefix { { 0, 0, 0, 0, 0, 0xffff, 0, 0 } };

            IpAddr& operator=(IpAddr const& rhs) {
                  d = rhs.d;
                  return *this;
            }

            void setIpV4(Bytes const& src) {
                  d = ip4Prefix;
                  d[IP4_PREFIX_LEN_SIZE_T] = networkEndian(src[1], src[0]);
                  d[IP4_PREFIX_LEN_SIZE_T + 1] = networkEndian(src[3], src[2]);
            }

            void setIpV4(uint32_t const addr) {
                  union {uint32_t d32; uint16_t s[sizeof(uint32_t)/sizeof(uint16_t)]; } a { addr};
                  d = ip4Prefix;
                  d[IP4_PREFIX_LEN_SIZE_T] = a.s[0];
                  d[IP4_PREFIX_LEN_SIZE_T + 1] = a.s[1];

            };

            void set(std::array<uint8_t, IP_ADDRESS_BIT_LEN / NUM_BITS_PER_SIZE_T> const& netOrder) {
                  for (size_t i = 0; i < sizeof(d) / sizeof(d[0]); ++i) {
                        d[i] = networkEndian(netOrder[i * 2 + 1], netOrder[i * 2]);
                  }
            }

            std::array<uint8_t, IP_ADDRESS_BIT_LEN / NUM_BITS_PER_SIZE_T> get() const {
                  union {
                        std::array<uint16_t, ADDR_LEN_SIZE_T> d16;
                        std::array<uint8_t, IP_ADDRESS_BIT_LEN / NUM_BITS_PER_SIZE_T> d8;
                  } netOrder { d };
                  return netOrder.d8;
            };

            bool isIpv4Addr() const {
                  for(size_t i = 0; i < IP4_PREFIX_LEN_SIZE_T; ++i) {
                        if(d[i] != ip4Prefix[i]) {
                              return false;
                        }
                  }
                  return true;
            }

            std::string toString() const {
                  std::stringstream stream;
                  for(size_t i = 0; i < sizeof(d) / sizeof(d[0]); ++i) {
                        stream << std::hex << networkEndian(d[i]);
                        if((i + 1) < sizeof(d) / sizeof(d[0])) {
                              stream << ':';
                        }
                  }

                  return stream.str();
            }

            std::array<uint16_t, ADDR_LEN_SIZE_T> d;
      };

      class InetDest {
      public:
            std::string toString() const {
                  return addr.toString() + "=>" + std::to_string(port);
            }
            InetDest& operator=(InetDest const& rhs) {
                  addr = rhs.addr;
                  valid = rhs.valid;
                  port = rhs.port;
                  ifIndex = rhs.ifIndex;
                  return *this;
            }
            IpAddr addr;
            bool valid;
            uint16_t port;
            unsigned int ifIndex;
      };
}
