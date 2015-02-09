#include "logger.hpp"
#include "tcpstream.hpp"
#include "utils.hpp"


namespace Sb {
	void TcpStream::create(const int fd, const InetDest remote,
						 std::shared_ptr<TcpStreamIf> client, const bool replace) {
		auto ref = std::make_shared<TcpStream>(fd, remote, client);
		Engine::add(ref, replace);
		logDebug("TcpStream::create() my ref " + intToString(ref.use_count()) +
				 " their refs " + intToString(client.use_count()));
		client->onConnect(*ref.get(), remote);
	}

	TcpStream::TcpStream(const int fd, const InetDest remote, std::shared_ptr<TcpStreamIf> client) :
						Socket(fd),
						remote(remote),
						client(client) {
		Socket::makeNonBlocking(fd);
		logDebug(std::string("TcpStream::TcpStream"));
	}

	TcpStream::~TcpStream() {
		logDebug("TcpStream::~TcpStream()");
	}

	void TcpStream::handleRead() {
		logDebug("TcpStream::handleRead()");
		for(;;) {
			Bytes data(MAX_PACKET_SIZE);
			auto actuallyRead = read(data);
			if(actuallyRead == 0) {
				logDebug(std::string("TcpStream::read failed - file closed"));
				return;
			}
			if(actuallyRead == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
				return;
			}
			auto ref = this->ref();
			client->onReadCompleted(*this, data);
			logDebug("On read completed");
		}
	}

	void TcpStream::handleWrite() {
		logDebug("TcpStream::handleWrite() " + intToString(writeQueue.len()));
		for(;;) {
			bool isEmpty = true;
			auto& data = writeQueue.removeAndIsEmpty(isEmpty);
			if(isEmpty) {
				logDebug("write queue is empty notifying client");
				auto ref = this->ref();
//				client->onWriteCompleted(*this);
				return;
			}
			doWrite(data);
		}
	}

	void TcpStream::handleError() {
		logDebug("TcpStream::handleError() " + intToString(writeQueue.len()));
		auto ref = this->ref();
		client->onDisconnect(*this);
	}

	void TcpStream::disconnect() {
		(void)remote;
		logDebug("TcpStream::disconnect() my ref " + intToString(this->ref().use_count()) +
				 " their refs " + intToString(client.use_count()));
		Engine::remove(this);
	}

	void TcpStream::queueWrite(const Bytes& data) {
		logDebug("TcpStream::queueWrite() " + intToString(getFd()));
		std::lock_guard<std::mutex> sync(writeLock);
		auto wasEmpty = writeQueue.isEmpty();
		writeQueue.add(data);
		if(wasEmpty) {
			handleWrite();
		}
	}

	void TcpStream::handleTimer(const size_t timerId) {
		logDebug("TcpListener::handleTimer() " + intToString(timerId));
		client->onTimerExpired(*this, timerId);
	}

	void TcpStream::doWrite(const Bytes& data) {
		logDebug(std::string("TcpStream::queueWrite writing " + intToString(data.size())));
		const auto actuallySent = write(data);
		logDebug(std::string("TcpStream::queueWrite actually wrote " + intToString(actuallySent)));
		pErrorLog(getFd());
		if(actuallySent == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
			logDebug(std::string("TcpStream::queueWrite would block"));
			writeQueue.add (data);
			return;
		}
		if(actuallySent < 0) {
			logDebug(std::string("TcpStream::write failed"));
			return;
		}

		decltype(actuallySent) dataLen = data.size();
		if(actuallySent == dataLen) {
			logDebug("Write " + intToString(actuallySent) + " out of " + intToString(dataLen) + " on " + intToString(getFd()));
		} else if (actuallySent > 0) {
			logDebug("Partial write of " + intToString(actuallySent) + " out of " + intToString(dataLen));
			writeQueue.add(Bytes(data.begin () + actuallySent, data.end ()));
		}
	}

}
