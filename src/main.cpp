#include <iostream>
#include <functional>
#include <regex>

#include "engine.hpp"
#include "udpsocket.hpp"
#include "tcplistener.hpp"
#include "udpsocket.hpp"
#include "tcpconn.hpp"
#include "logger.hpp"

using namespace Sb;
namespace Sb {
	extern size_t test_ocb ();
}

void runUnit(const std::string id, std::function<void()>what) {
	try {
		Engine::init();
		what();
		Engine::start(0);
	} catch(const std::exception& e) {
		   std::cerr << e.what() << "\n";
	} catch ( ... ) {
		std::cerr <<  id << " " <<  "exception" << std::endl;
	}
}


class TcpEcho : public TcpStreamIf {
	public:
		TcpEcho() {}
		virtual ~TcpEcho() {
			logDebug("~TcpEcho destroyed");
		}

		virtual void onError() override {

		}

		virtual void onConnect(TcpStream& s, const struct InetDest& ) {
			logDebug("TcpEcho onConnect");
			Engine::stop();
			Bytes greeting = stringToBytes("Ello\n");
			s.queueWrite(greeting);
//			s.disconnect();
		}

		virtual void onReadCompleted( TcpStream& s, const Bytes& w) {
			logDebug("TcpEcho onReadCompleted");
			s.queueWrite(w);
//			s.disconnect();
		}

		virtual void onWriteCompleted( TcpStream&) {
			logDebug("TcpEcho onReadyToWrite");
		}

		virtual void onDisconnect(TcpStream& /*s*/) {
			logDebug("TcpEcho onDisconnect");
//			s.disconnect();
		}

		virtual void onTimerExpired(TcpStream& s, int tid) {
			s.queueWrite( { 'x', 'p', '\n' });
			logDebug("blah expired " + intToString(tid));
		}
};

class TcpEchoWithGreeting : public TcpStreamIf {
	public:
		TcpEchoWithGreeting() {}
		virtual void onError() override {
			logDebug("~TcpEchoWithGreeting onError");
		}

		virtual ~TcpEchoWithGreeting() {
			logDebug("~TcpEchoWithGreeting destroyed");
		}
		virtual void onConnect(TcpStream& s, const struct InetDest& ) {
			logDebug("TcpEcho onConnect");
			Bytes greeting = stringToBytes("get http://127.0.0.1/\r\n\r\n");
			s.queueWrite(greeting);
//			s.setTimer(5, NanoSecs(1000000000));
		}

		virtual void onReadCompleted(TcpStream& /*s*/, const Bytes&) {
			logDebug("TcpEcho onReadCompleted");
		}

		virtual void onWriteCompleted( TcpStream&) {
			logDebug("TcpEcho onReadyToWrite");
		}

		virtual void onDisconnect( TcpStream& s) {
			logDebug("TcpEcho onDisconnect");
			s.disconnect();
		}

		virtual void onTimerExpired(TcpStream& /*s*/, int tid) {
			logDebug("blah expired " + intToString(tid));
		}
};

std::pair<const Bytes::iterator, const Bytes::iterator>
findFirstPattern(const Bytes::iterator begin, const Bytes::iterator end, const Bytes& pattern) {
	size_t matchPos = 0;
	auto matchBegin = begin;
	for(auto pos = begin; pos != end; ++pos) {
		if(::tolower(*pos) == ::tolower(pattern[matchPos]) || pattern[matchPos] == '?') {
			if(++matchPos == pattern.size()) {
				return std::make_pair(matchBegin, pos);
			}
		} else {
			matchBegin = pos;
			matchPos = 0;
		}
	}
	return std::make_pair(end,end);
}

class HttpProxy : public TcpStreamIf {
	public:
		HttpProxy() {}
		virtual ~HttpProxy() {
			logDebug("~TcpEchoWithGreeting destroyed");
		}
		virtual void onError() override {
		}

		virtual void onConnect(TcpStream& s, const struct InetDest& ) {
			logDebug("TcpEcho onConnect");
			s.setTimer(0, NanoSecs(300000000000));
			s.setTimer(5, NanoSecs(10000000000));
			s.setTimer(5, NanoSecs(10000000000));
			s.setTimer(5, NanoSecs(10000000000));
		}

		virtual void onReadCompleted( TcpStream& , const Bytes& x) {
			logDebug("TcpEcho onReadCompleted");
			header.insert(header.end(), x.begin(), x.end());

			bool doubleCr = false;
			if(header.size() < 4) {
				return;
			}
			bool prevCrLf = false;
			bool prevlF = *header.end() == '\n';
			//				std::find(header.begin(), header.end(), Bytes{ '\r', '\n', '\r', '\n' } );
			for(auto it = std::end(header) - 1;	it != std::begin(header); --it) {
				if(prevlF && *it == '\r' && prevCrLf) {
					doubleCr = true;
					logDebug("Got double CRLF");
					break;
				} else if(prevlF && *it == '\r') {
					prevCrLf = true;
					prevlF = false;
				} else if(!prevlF && *it == '\n') {
					prevlF = true;
				} else {
					prevCrLf = false;
					prevlF = false;
				}
			}
			if(!doubleCr) {
				return;
			}
			auto crPos = header.end();
			for(auto start = header.begin(); start != header.end(); start = crPos + 1) {
				while(*start == '\n') {
					++start;
				}
				crPos = std::find(start, header.end(), '\r');
								Bytes getPost(start, crPos);
				logDebug("line: " + std::string{getPost.begin(), getPost.end() } );
				auto host = findFirstPattern(getPost.begin(), getPost.end(), { 'H', 'o', 's', 't', ':', '?'});
				if(host.first != host.second) {
					logDebug("host: " + std::string{host.second + 1, getPost.end()} );
				}

				if(crPos == header.end()) {
					break;
				}
			}
			std::regex re("GET.*");
			std::smatch m;
			std::string tmp(header.begin(), header.end());
			std::regex_search(tmp, m, re);
			 std::cout << " ECMA (depth first search) match: " << m[0] << '\n';
		}

		virtual void onWriteCompleted( TcpStream&) {
			logDebug("TcpEcho onReadyToWrite");
		}

		virtual void onDisconnect( TcpStream& s) {
			logDebug("TcpEcho onDisconnect");
			s.disconnect();
		}

		virtual void onTimerExpired(TcpStream& s, int tid) {
			s.setTimer(5, NanoSecs(1000000000));
			logDebug("blah expired " + intToString(tid));
		}
	private:
		Bytes header;
};

class UdpEcho : public UdpSocketIf {
		virtual void onError() override {
			logDebug("UdpEcho::onError");
		}
		virtual void onConnect(UdpSocket& s, const InetDest& from) override {
			logDebug("UdpEcho::onConnect");
			s.queueWrite(Bytes({'e', 'l', 'l', 'o', '\r', '\n'} ), from);
		}
		virtual void onReadCompleted(UdpSocket& s, const InetDest& from, const Bytes& w) override {
			logDebug("UdpEcho::onReadCompleted");
			s.queueWrite(w, from);
		}
		virtual void onWriteCompleted(UdpSocket&) override {
			logDebug("UdpEcho::onWriteCompleted");
		}
		virtual void onTimerExpired(UdpSocket&, int) override {
			logDebug("UdpEcho::onTimerExpired");
		}
};

int main (const int, const char* const argv[]) {
//	runUnit("connect", [] () { TcpConn::create(Socket::destFromString("::ffff:216.58.220.100", 80), std::make_shared<TcpEchoWithGreeting>()); });
//	runUnit("udpconn", [] () { UdpSocket::create(Socket::destFromString("::ffff:127.0.0.1", 1223), std::make_shared<UdpEcho>()); });
	runUnit("udpconn", [] () { UdpSocket::create(1223, std::make_shared<UdpEcho>()); });
//	runUnit ("connect", [] () {	TcpListener::create(1025, [] () { return std::make_shared<TcpEchoWithGreeting>(); } ); 	});
//		TcpConn::create( Socket::destFromString("::1", 1025), std::make_shared<TcpEchoWithGreeting>());
//		TcpListener::create(1024, [] () { return std::make_shared<HttpProxy>(); } );
//		TcpConn::create(Socket::destFromString("::ffff:216.58.220.100", 80), std::make_shared<TcpEchoWithGreeting>());



//		;(new UdpServer(1025))->start();
//		(new TcpConnector("127.0.0.1", 1025))->start ();
//        (new UdpServer(1025))->start();
//		(new TcpListener(1025	))->start ();
//		(new TcpConnector("127.0.0.1", 1025))->start ();
//		(new TcpConnector("::1", 1025))->start();
//		(new UdpServer(1025))->start ();
//		(new UdpClient("127.0.0.1", 1025, { 'T', 'E', 'S', 'T', '\n'}))->start ();
//		(new UdpClient("::1", 1025, { 'T', 'E', 'S', 'T', '\n'}))->start();
//		(new TcpConnector("2404:6800:4003:c02::93", 443))->start();
//        (new TcpConnector("::1", 4040))->start();

	std::cerr << argv[0] << " exited." << std::endl;

	return 0;
}


