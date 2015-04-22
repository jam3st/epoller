#pragma once
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include "logger.hpp"
#include "types.hpp"


namespace Sb {
	constexpr size_t numBitsPerByte = 8;

	InetDest destFromString(const std::string& dest, const uint16_t port);

	void assert(bool ok, const std::string error);
	void pErrorThrow(const int error);
	void pErrorLog(const int error);
	std::string pollEventsToString(const uint32_t events);

	std::string toHexString(Bytes const& src);

	template< typename T >
	std::string intToHexString(const T number) {
//		constexpr bool needsCast = sizeof(T) < sizeof(size_t);
		std::stringstream stream;
		stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << number;
		return stream.str();
	}
}

