#include "logger.hpp"
#include "tcpstream.hpp"
#include "utils.hpp"


namespace Sb {
	void TcpStream::create(const int fd, const InetDest remote,
						 std::shared_ptr<TcpStreamIf> client, const bool replace) {
		auto ref = std::make_shared<TcpStream>(fd, remote, client);
		Engine::add(ref, replace);
		logDebug("TcpStream::create() my ref " + std::to_string(ref.use_count()) +
				 " their refs " + std::to_string(client.use_count()) + " "  + std::to_string(fd));
		client->tcpStream = ref;
		client->connected(remote);
	}

	TcpStream::TcpStream(const int fd, const InetDest remote, std::shared_ptr<TcpStreamIf>& client) :
						Socket(fd),
						remote(remote),
						client(client),
						writeQueue({}) {
		Socket::makeNonBlocking(fd);
		logDebug(std::string("TcpStream::TcpStream " + std::to_string(getFd())));
	}

	TcpStream::~TcpStream() {
		logDebug("TcpStream::~TcpStream() " + std::to_string(getFd()));
	}

	void TcpStream::handleRead() {
		logDebug("TcpStream::handleRead()");
		for(;;) {
			Bytes data(MAX_PACKET_SIZE);
			auto actuallyRead = read(data);
			if(actuallyRead == 0) {
				logDebug("TcpStream::read failed - file closed " + std::to_string(fd));
				return;
			}
			if(actuallyRead == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
				logDebug(std::string("TcpStream::handleRead would block"));
				return;
			}
			auto ref = this->ref();
			client->received(data);
			logDebug("On read completed " + std::to_string(getFd()));
		}
	}

	void TcpStream::handleWrite() {
		logDebug("TcpStream::handleWrite() " + std::to_string(writeQueue.len()) + " " + std::to_string(fd));
		for(;;) {
			bool isEmpty = true;
			auto& data = writeQueue.removeAndIsEmpty(isEmpty);
			if(isEmpty) {
				logDebug("write queue is empty notifying client");
				auto ref = this->ref();
				client->writeComplete();
				return;
			}
			doWrite(data);
		}
	}

	void TcpStream::handleError() {
		logDebug("TcpStream::handleError() " + std::to_string(writeQueue.len()));
		auto ref = this->ref();
		client->disconnected();
		disconnect();
	}

	void TcpStream::disconnect() {
		logDebug("TcpStream::disconnect() my ref " + std::to_string(this->ref().use_count()) +
				 " their refs " + std::to_string(client.use_count()) + " " + remote.toString() + " " +  std::to_string(fd));
		Engine::remove(this);
	}

	void TcpStream::queueWrite(const Bytes& data) {
		logDebug("TcpStream::queueWrite() " + std::to_string(fd));
		std::lock_guard<std::mutex> sync(writeLock);
		auto wasEmpty = writeQueue.isEmpty();
		writeQueue.add(data);
		if(wasEmpty) {
			handleWrite();
		}
	}

	void TcpStream::handleTimer(const size_t timerId) {
		logDebug("TcpListener::handleTimer() " + std::to_string(timerId) + " " + std::to_string(fd));
		client->timeout(timerId);
	}

	void TcpStream::doWrite(const Bytes& data) {
		logDebug(std::string("TcpStream::queueWrite writing " + std::to_string(data.size())) + " "  + std::to_string(fd));
		const auto actuallySent = write(data);
		logDebug(std::string("TcpStream::queueWrite actually wrote " + std::to_string(actuallySent)));
		pErrorLog(getFd());
		if(actuallySent == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
			logDebug(std::string("TcpStream::queueWrite would block"));
			writeQueue.add(data);
			return;
		}
		if(actuallySent < 0) {
			logDebug(std::string("TcpStream::write failed"));
			return;
		}

		decltype(actuallySent) dataLen = data.size();
		if(actuallySent == dataLen) {
			logDebug("Write " + std::to_string(actuallySent) + " out of " + std::to_string(dataLen) + " on " + std::to_string(getFd()));
		} else if (actuallySent > 0) {
			logDebug("Partial write of " + std::to_string(actuallySent) + " out of " + std::to_string(dataLen));
			writeQueue.add(Bytes(data.begin () + actuallySent, data.end ()));
		}
	}
}
