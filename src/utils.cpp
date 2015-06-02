#include <string.h>
#include <sys/epoll.h>
#include "utils.hpp"
#include <errno.h>

namespace Sb {
      void assert(bool ok, const std::string error) {
            if (!ok) {
                  logError(std::string("Assertion failed: ") + error);
                  __builtin_trap();
            }
      }

      void pErrorThrow(int const error, int const fd) {
            if (error < 0) {
                  auto errorText = std::string("pErrorThrow: ") + std::string(::strerror(errno));
                  if (fd > 0) {
                        errorText += " " + std::to_string(fd);
                  }
                  logError(errorText);
                  assert(false, errorText);
            }
      }

      void pErrorLog(int const error, int const fd) {
            if (error < 0) {
                  auto errorText = std::string("pErrorLog: ") + std::string(::strerror(errno));
                  if (fd > 0) {
                        errorText += " " + std::to_string(fd);
                  }
                  logError(std::string(::strerror(errno)) + " " + std::to_string(fd));
            }
      }

      static std::string pollEventToString(const uint32_t event) {
            switch (event) {
                  case EPOLLIN:
                        return "EPOLLIN";
                  case EPOLLPRI:
                        return "EPOLLPRI";
                  case EPOLLOUT:
                        return "EPOLLOUT";
                  case EPOLLRDNORM:
                        return "EPOLLRDNORM";
                  case EPOLLRDBAND:
                        return "EPOLLRDBAND";
                  case EPOLLWRNORM:
                        return "EPOLLWRNORM";
                  case EPOLLWRBAND:
                        return "EPOLLWRBAND";
                  case EPOLLMSG:
                        return "EPOLLMSG";
                  case EPOLLERR:
                        return "EPOLLERR";
                  case EPOLLHUP:
                        return "EPOLLHUP";
                  case EPOLLRDHUP:
                        return "EPOLLRDHUP";
                  case EPOLLWAKEUP:
                        return "EPOLLWAKEUP";
                  case EPOLLONESHOT:
                        return "EPOLLONESHOT";
                  case EPOLLET:
                        return "EPOLLET";
            }
            return "";
      }

      std::string pollEventsToString(uint32_t const events) {
            std::stringstream stream;
            auto isFirst = true;
            for (int bitPos = sizeof(events) * NUM_BITS_PER_SIZE_T - 1; bitPos >= 0; bitPos--) {
                  uint32_t mask = 1 << bitPos;
                  std::string eventName = pollEventToString(events & mask);
                  if (eventName.size() > 0) {
                        if (isFirst) {
                              isFirst = false;
                        } else {
                              stream << " | ";
                        }
                        stream << eventName;
                  }
            }
            return stream.str().size() > 0 ? stream.str() : "EMPTY";
      }

      std::string toHexString(Bytes const&src) {
            constexpr auto numPerLine = 32;
            std::stringstream stream;
            stream << std::endl << std::setfill('0') << std::setw(8) << std::hex << 0;
            if (src.size() > 0) {
                  for (size_t i = 0; i < (src.size() - 1) / numPerLine + 1; ++i) {
                        auto rowStartPos = i * numPerLine;
                        stream << ":";
                        for (size_t j = rowStartPos; j < src.size() && j < rowStartPos + numPerLine; j += 2) {
                              stream << " " << std::setw(2) << std::setfill('0') << std::hex << static_cast<size_t>(src.at(j));
                              if ((j + 1) < src.size()) {
                                    stream << std::setw(2) << std::setfill('0') << std::hex << static_cast<size_t>(src.at(j + 1));
                              }
                        }
                        stream << std::endl << std::setfill('0') << std::setw(8) << std::hex << (i + 1) * numPerLine;
                  }
            }
            stream << std::endl;
            return stream.str();
      }
}
