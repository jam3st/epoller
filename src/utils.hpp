#pragma once
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include "logger.hpp"
#include "types.hpp"


namespace Sb {
	const int numBitsPerByte = 8;

	InetDest destFromString(const std::string& dest, const uint16_t port);

	void assert(bool ok, const std::string error);
	void pErrorThrow(const int error);
	void pErrorLog(const int error);
	std::string pollEventsToString(const uint32_t events);

	template< typename T >
	std::string intToHexString(const T number) {
		std::stringstream stream;
		stream << "0x"
			   << std::setfill ('0') << std::setw(sizeof(T) * 2)
			   << std::hex << number;
		  return stream.str();
	}

	Bytes stringToBytes(const std::string src);
}

