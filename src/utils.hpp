#pragma once
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <string>
#include "logger.hpp"
#include "types.hpp"


namespace Sb {
	const int64_t NanoSecsInSecs = { 1000000000 };
	const int numBitsPerByte = 8; // By definition ISO/IEC 9899:TC3
	const int LISTEN_MAX_PENDING = 1;
	const size_t APPROX_PAGE_SIZE = 4096;

	typedef std::chrono::nanoseconds NanoSecs;
	typedef std::chrono::seconds Seconds;
	typedef std::chrono::high_resolution_clock HighResClock;
	typedef std::chrono::time_point<HighResClock, NanoSecs> TimePointNs;

	InetDest destFromString(const std::string& dest, const uint16_t port);

	void assert(bool ok, const std::string error);
	void pErrorThrow(const int error);
	void pErrorLog(const int error);
	std::string pollEventsToString(const uint32_t events);

	template< typename T >
	std::string intToString(const T number) {
		std::stringstream stream;
		stream << number;
		return stream.str();
	}

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

