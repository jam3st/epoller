#pragma once
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include "logger.hpp"
#include "types.hpp"

namespace Sb {
      InetDest destFromString(std::string const& dest, uint16_t const port);
      void assert(bool const ok, std::string const error);
      void pErrorThrow(int const error, int const fd = -1);
      void pErrorLog(int const error, int const fd);
      std::string pollEventsToString(uint32_t const events);
      std::string toHexString(Bytes const&src);

      template<typename T>
      std::string intToHexString(T const&number) {
            std::stringstream stream;
            stream << std::setfill('0') << std::hex << number;
            return stream.str();
      }
}

