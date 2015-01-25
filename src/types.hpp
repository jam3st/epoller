#pragma once
#include <string>
#include <vector>
#include <atomic>

namespace Sb {
	typedef uint8_t byte;
	typedef std::vector<byte> Bytes;
	const ssize_t MAX_PACKET_SIZE = 4096;
	const ssize_t IP_ADDRESS_SIZE = 128 / 8;

	struct InetDest {
		uint8_t	addr[IP_ADDRESS_SIZE];
		uint16_t port;
	};

}
