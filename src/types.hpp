#pragma once
#include <string>
#include <vector>
#include <array>
#include <iomanip>
#include <sstream>

namespace Sb {
	typedef uint8_t byte;
	typedef std::vector<byte> Bytes;
	typedef std::array<uint8_t, 128 / 8> IpAddr;
	constexpr ssize_t MAX_PACKET_SIZE = 4096;

	struct InetDest {
		IpAddr addr;
		bool valid;
		uint16_t port;

		std::string toString() const {
			std::stringstream stream;
			for(size_t i = 0; i < sizeof(addr); i = i + 2) {
				stream << std::hex << std::setfill('0') << std::setw(2) << static_cast<size_t>(addr[i]) << std::hex << std::setfill('0') << std::setw(2) <<
				static_cast<size_t>(addr[i + 1]) << ':';
			}
			stream << std::setw(-1) << std::dec << port;
			return stream.str();
		}
	};

	inline std::string toString(IpAddr const& addr) {
		std::stringstream stream;
		for(size_t i = 0; i < sizeof(addr); i = i + 2) {
			stream << std::hex << std::setfill('0') << std::setw(2) << static_cast<size_t>(addr[i]) << std::hex << std::setfill('0') << std::setw(2) <<
			static_cast<size_t>(addr[i + 1]) << ':';
		}
		return stream.str();
	}
}
