#include "epoll.hpp"
#include "engine.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include <sys/epoll.h>
#include <unistd.h>

namespace Sb {
	Epoll::Epoll(const int fd)
		: fd(fd) {
		pErrorThrow(fd);
		logDebug("Epollable::Epollable()");
	}

	Epoll::~Epoll() {
		logDebug("Epollable::~Epollable() " + intToString(fd));
		::close(fd);
	}

	void Epoll::run(const uint32_t events) {
		logDebug("Epollable::Run " + pollEventsToString(events));
		if((events & EPOLLIN) != 0) {
			handleRead();
		}
		if((events & EPOLLRDHUP) != 0 || (events & EPOLLERR) != 0) {
			handleError();
		}
		if((events & EPOLLOUT) != 0) {
			handleWrite();
		}
	}

	void Epoll::timeout(const int timerId) {
		logDebug("Epoll::timeout " + intToString(timerId));
	}

//	void Epoll::handleError() {
//		logDebug("Epollable::handleError()");
//	}

//	void Epoll::handleRead() {
//		logDebug("Epollable::handleRead()");
//	}

//	void Epoll::handleWrite() {
//		logDebug("Epollable::handleWrite()");
//	}

//	void Epoll::handleTimer(int timerId) {
//		logDebug("Epollable::handleTimer() " + intToString(timerId));
//	}

	void Epoll::setTimer(const int timerId, const NanoSecs& timeout) {
		assert(timeout.count() > 0, " Epoll::setTimer timeout must be > 0");
		logDebug("Epollable::setTimer() " + intToString(timerId));
		Engine::setTimer(this, timerId, timeout);
	}

	void Epoll::cancelTimer(const int timerId) {
		logDebug("Epollable::cancelTimer() " + intToString(timerId));
		Engine::cancelTimer(this, timerId);
	}
	int Epoll::dupFd() const  {
		logDebug("Epollable::dupFd");
		auto dup = ::dup(fd);
		pErrorThrow(dup);
		return dup;
	}

	int Epoll::getFd() const {
		return fd;
	}

	std::shared_ptr<Epoll> Epoll::ref() {
		return shared_from_this();
	}
}

