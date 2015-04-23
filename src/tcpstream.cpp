#include "logger.hpp"
#include "tcpstream.hpp"

namespace Sb {
	void TcpStream::create(const int fd, const InetDest remote,
						 std::shared_ptr<TcpStreamIf> client, const bool replace) {
		auto ref = std::make_shared<TcpStream>(fd, remote, client);

		logDebug("TcpStream::create() my ref " + std::to_string(ref.use_count()) +
				 " their refs " + std::to_string(client.use_count())  + " dest " + remote.toString()  + " client " + intToHexString(client) + " "  + std::to_string(fd));
		client->tcpStream = ref;
		client->connected(remote);
		Engine::add(ref, replace);
		logDebug("TcpStream::create() my ref " + std::to_string(ref.use_count()) + " their refs " + std::to_string(client.use_count())+ " "  + std::to_string(fd));
	}

	TcpStream::TcpStream(const int fd, const InetDest remote, std::shared_ptr<TcpStreamIf>& client) :
						Socket(fd),
						remote(remote),
						client(client),
						waitingWriteEvent(false),
						writeQueue( {} ) {
		Socket::makeNonBlocking(fd);
		logDebug(std::string("TcpStream::TcpStream " + std::to_string(getFd())));
	}

	TcpStream::~TcpStream() {
		logDebug("TcpStream::~TcpStream() " + std::to_string(getFd()));
	}

	void TcpStream::handleRead() {
		std::lock_guard<std::mutex> sync(readLock);
		logDebug("TcpStream::handleRead()");
		for(;;) {
			Bytes data(MAX_PACKET_SIZE);
			auto actuallyRead = read(data);
			if(actuallyRead == 0) {
				logDebug("TcpStream::read failed - file closed " + std::to_string(fd));
				return;
			}
			if(actuallyRead == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
				logDebug(std::string("TcpStream::handleRead would block " + std::to_string(fd)));
				return;
			}
			auto tmp = client;
			if(tmp == nullptr) {
				logDebug("TcpStream::handleRead() client deleted.");
				return;
			}
			tmp->received(data);
			logDebug("On read completed " + std::to_string(getFd()));
		}
	}

	void TcpStream::handleWrite() {
		writeHandler(true);
	}

	bool TcpStream::writeHandler(bool const fromTrigger) {
		std::lock_guard<std::mutex> sync(writeLock);
		bool notify = false;
		if(waitingWriteEvent && !fromTrigger) {
			logDebug("TcpStream::writeHandler() write alread triggered  " + std::to_string(writeQueue.len()) + " " + std::to_string(fd));
		} else {
            bool canWriteMore = false;
            do {
                logDebug("TcpStream::writeHandler() " + std::to_string(writeQueue.len()) + " " + std::to_string(fd));
                auto data = writeQueue.removeAndIsEmpty();
                if (data.second) {
                    logDebug("write queue is empty notifying client");
                    notify = true;
					waitingWriteEvent = false;
                    canWriteMore = false;
                } else {
                    canWriteMore = doWrite(data.first);
                }
            } while(canWriteMore);
		}
		return notify;
	}

	void TcpStream::handleError() {
		logDebug("TcpStream::handleError() with remaining " + std::to_string(writeQueue.len()));
		disconnect();
	}

	void TcpStream::disconnect() {
		logDebug("TcpStream::disconnect() my ref " + std::to_string(disconnected) + " " + std::to_string(ref().use_count()) + " " + remote.toString() + " " +  std::to_string(fd));
		if(!disconnected) {
			disconnected = true;
			if (client != nullptr) {
				logDebug("TcpStream::disconnect()  my ref " + std::to_string(ref().use_count()) + " client ref " +
																								  std::to_string(client.use_count()) + " " + remote.toString() + " " +
																								  std::to_string(fd));
				client->disconnected();
				logDebug("TcpStream::disconnect()  my ref " + std::to_string(ref().use_count()) + " client ref " +
																								  std::to_string(client.use_count()) + " " + remote.toString() + " " +
																								  std::to_string(fd));
				client = nullptr;
			}
			Engine::remove(this);
		}
		logDebug("TcpStream::disconnect() my ref " + std::to_string(ref().use_count()) + " " + remote.toString() + " " +  std::to_string(fd));
	}

	void TcpStream::queueWrite(const Bytes& data) {
		logDebug("TcpStream::queueWrite() " + std::to_string(fd));
		writeQueue.add(data);
		if(writeHandler()) {
			if (client != nullptr) {
				client->writeComplete();
			}
		}
	}

	void TcpStream::handleTimer(size_t const timerId) {
		logDebug("TcpListener::handleTimer() " + std::to_string(timerId) + " " + std::to_string(fd));
		client->timeout(timerId);
	}

	bool TcpStream::doWrite(Bytes const& data) {
		logDebug(std::string("TcpStream::doWrite writing " + std::to_string(data.size())) + " "  + std::to_string(fd));
        bool canWriteMore = false;

        const auto actuallySent = write(data);

		logDebug(std::string("TcpStream::doWrite actually wrote " + std::to_string(actuallySent)
				 + " out of " + std::to_string(data.size()) + " on " + std::to_string(fd)));
		pErrorLog(getFd());
		if(actuallySent == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
			logDebug(std::string("TcpStream::queueWrite would block"));
			waitingWriteEvent = true;
			writeQueue.add(data);
		} else if(actuallySent < 0) {
			logDebug(std::string("TcpStream::write failed"));
		} else {
			const decltype(actuallySent) dataLen = data.size();
			if(actuallySent == dataLen) {
				logDebug("Write " + std::to_string(actuallySent) + " out of " + std::to_string(dataLen) + " on " + std::to_string(getFd()));
                canWriteMore = true;
			} else if (actuallySent > 0) {
				logDebug("Partial write of " + std::to_string(actuallySent) + " out of " + std::to_string(dataLen));
				writeQueue.add(Bytes(data.begin () + actuallySent, data.end ()));
			}
	    }
        return canWriteMore;
    }
}

