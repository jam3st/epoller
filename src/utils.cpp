#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

#include "utils.hpp"
#include "logger.hpp"

namespace Sb {
	int64_t driftCorrectionInNs = std::chrono::duration_cast<NanoSecs>(
									  std::chrono::system_clock::now().time_since_epoch() -
									  std::chrono::steady_clock::now().time_since_epoch()
									  ).count();

	void assert(bool ok, const std::string error) {
		if(!ok) {
			logError(std::string("Assertion failed: ") + error);
			throw std::runtime_error(error);
		}
	}

	void pErrorThrow(const int error) {
		if(error < 0) {
			auto errorText = ::strerror(errno);
			__builtin_trap();
			throw std::runtime_error(errorText);
		}
	}

	void pErrorLog(const int error) {
		if(error < 0) {
			logError(::strerror(errno));
		}
	}

	static std::string pollEventToString(const uint32_t event) {
		switch(event) {
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

	std::string pollEventsToString(const uint32_t events) {
		std::stringstream stream;
		auto isFirst = true;
		for(int bitPos =  sizeof events * numBitsPerByte - 1; bitPos >= 0; bitPos--) {
			uint32_t mask = 1 << bitPos;
			std::string eventName = pollEventToString(events & mask);
			if(eventName.size() > 0) {
				if(isFirst) {
					isFirst = false;
				} else {
					stream <<  " | ";
				}
				stream << eventName;
			}
		}
		return stream.str().size() > 0 ? stream.str() : "EMPTY";
	}

	Bytes stringToBytes(const std::string src) {
		return Bytes(src.begin(), src.end());
	}
}
