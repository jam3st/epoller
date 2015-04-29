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

	InetDest destFromString(std::string const& dest, uint16_t const port);

	void assert(bool ok, std::string const error);
	void pErrorThrow(const int error);
	void pErrorLog(const int error);
	std::string pollEventsToString(uint32_t const events);

	std::string toHexString(Bytes const& src);

	template<typename T>
	std::string intToHexString(T const& number) {
		std::stringstream stream;
		stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << number;
		return stream.str();
	}
}

