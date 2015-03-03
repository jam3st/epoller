//#include "logger.hpp"
//#include "tlstcpstream.hpp"
//#include "utils.hpp"


//namespace Sb {
//	TlsTcpStream::TlsTcpStream(const int fd, const Bytes& initialData)
//		: Socket(fd) {
//		logDebug(std::string("TlsTcpStream::TlsTcpStream"));
//		queueWrite(initialData);
//	}

//	TlsTcpStream::~TlsTcpStream() {
//		logDebug("TlsTcpStream::~TlsTcpStream()");
//	}

//	void TlsTcpStream::handleRead() {
//		logDebug("TlsTcpStream::handleRead()");
//		Bytes data(MAX_PACKET_SIZE);
//		auto actuallyRead = read(data);
//		if(actuallyRead <= 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
//			return;
//		}
//		// Todo
//		queueWrite(data);
//	}


//	void TlsTcpStream::handleError() {
//	}

//	void TlsTcpStream::handleTimeout(int ) {
//	}

//	void TlsTcpStream::queueWrite(const Bytes& data) {
//		logDebug(std::string("TlsTcpStream::queueWrite"));
//		std::lock_guard<std::mutex> sync(writeQueueLock);
//		writeQueue.push(data);
//	}

//	void TlsTcpStream::handleWrite() {
//		logDebug("TlsTcpStream::handleWrite()");
//		writeQueueLock.lock();
//		if(writeQueue.empty()) {
//			writeQueueLock.unlock();
//			logDebug("nothing to write");
//			return;
//		}
//		auto data = writeQueue.front();
//		writeQueueLock.unlock();
//		logDebug(std::string("TlsTcpStream::queueWrite writing " + std::to_string(data.size())));
//		const auto actuallySent = write(data);
//		logDebug(std::string("TlsTcpStream::queueWrite actually wrote " + std::to_string(actuallySent)));
//		pErrorLog(getFd());
//		if(actuallySent <= 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
//			logDebug(std::string("TlsTcpStream::queueWrite would block"));
//			return;
//		}
//		decltype(actuallySent) dataLen = data.size();
//		if(actuallySent == dataLen) {
//			std::lock_guard<std::mutex> sync(writeQueueLock);
//			writeQueue.pop();
//		} else if (actuallySent > 0) {
//			logDebug("Partial write of " + std::to_string(actuallySent) + " out of " + std::to_string(dataLen));
//			std::lock_guard<std::mutex> sync(writeQueueLock);
//			std::vector<decltype(data)::value_type>(data.begin() + actuallySent, data.end()). swap(data);
//		}
//	}
//}
