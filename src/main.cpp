#include <iostream>
#include <functional>
#include <regex>
#include <unistd.h>

#include "engine.hpp"
#include "udpsocket.hpp"
#include "tcplistener.hpp"
#include "udpsocket.hpp"
#include "tcpconn.hpp"
#include "logger.hpp"
#include "query.hpp"

using namespace Sb;
namespace Sb {
	extern size_t test_ocb ();
}

void runUnit(const std::string id, std::function<void()>what) {
	try {
		Engine::init();
		what();
		Engine::start(1);
	} catch(const std::exception& e) {
		   std::cerr << e.what() << "\n";
	} catch ( ... ) {
		std::cerr <<  id << " " <<  "exception" << std::endl;
	}
}


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

class Remote : public TcpStreamIf {
	public:
		Remote(const std::shared_ptr<TcpStream>& xxx, Bytes& initWrite)
			:ep(xxx),
			 initWrite(initWrite) {
if(xxx == nullptr) {
	__builtin_trap();
}
		}
		virtual ~Remote() {
			logDebug("~Remote destroyed");
		}

		virtual void connected(std::shared_ptr<TcpStream> str, const struct InetDest& dest) override {
			stream = str;
			logDebug("Remote onConnect " + dest.toString());
			str->queueWrite(initWrite);
		}

		virtual void received(const Bytes& x) override {
			logDebug("Remote received");
			ep->queueWrite(x);
		}

		virtual void writeComplete() override {
			logDebug("Remote onReadyToWrite");
		}

		virtual void disconnected() override {
			logDebug("Remote onDisconnect");
			stream = nullptr;
		}

		virtual void timeout(const size_t /*timerId*/) override {
//			s.setTimer(5, NanoSecs(1000000000));
			logDebug("blah expired");
		}
	private:
		std::shared_ptr<TcpStream> ep;
		const Bytes& initWrite;
};

class HttpProxy : public TcpStreamIf {
	public:
		HttpProxy()	 {
			stream.reset();
		}

		virtual ~HttpProxy() {
			logDebug("~HttpProxy destroyed");
		}

		virtual void connected(std::shared_ptr<TcpStream> str, const struct InetDest& dest) override {
			logDebug("HttpProxy onConnect " + dest.toString());
			stream = str;
//			s.setTimer(0, NanoSecs(300000000000));
//			s.setTimer(5, NanoSecs(10000000000));
//			s.setTimer(5, NanoSecs(10000000000));
//			s.setTimer(5, NanoSecs(10000000000));
		}

		virtual void received(const Bytes& x) override {
			logDebug("HttpProxy onReadCompleted");
			if(ep.get() != nullptr) {
				logDebug("HttpProxy onReadCompleted to stream");
				ep->getStream()-> queueWrite(x);
				return;
			}
			std::cerr << std::string(x.begin(), x.end());
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
			std::string host;
			for(auto start = header.begin(); start != header.end(); start = crPos + 1) {
				while(*start == '\n') {
					++start;
				}
				crPos = std::find(start, header.end(), '\r');
								Bytes getPost(start, crPos);
				logDebug("line: " + std::string{getPost.begin(), getPost.end() } );
				auto it = findFirstPattern(getPost.begin(), getPost.end(), { 'H', 'O', 'S', 'T', ':', '?'});
				if(it.first != it.second) {
					host = std::string{it.second + 1, getPost.end()};
					logDebug("host: " +  host );
				}

				if(crPos == header.end()) {
					break;
				}
			}
			uint16_t port = 80;
			auto it = findFirstPattern(header.begin(), header.end(), { 'C', 'O', 'N', 'N', 'E', 'C', 'T', ' ' });
			if(it.first != it.second) {
				header.resize(0);
				port = 443;
			}

			std::cout << " ************************** host " << host  << " " << port << "**************************" << std::endl;
			ep =  std::make_shared<Remote>(stream, header);
			TcpConnection::create(host, port, ep);
		}

		virtual void writeComplete() override {
			logDebug("HttpProxy onReadyToWrite");
		}

		virtual void disconnected() override {
			logDebug("HttpProxy onDisconnect");
			stream.reset();
			ep.reset();
		}

		virtual void timeout(const size_t /*timerId*/) override {
//			s.setTimer(5, NanoSecs(1000000000));
			logDebug("blah expired");
		}
	private:
		Bytes header;
		std::shared_ptr<TcpStreamIf> ep;
};


int main (const int, const char* const argv[]) {
	::close(0);
//	runUnit("connect", [] () { TcpConnection::create("www.google.com", 80, std::make_shared<TcpEchoWithGreeting>()); });
//	runUnit("connect", [] () { TcpConnection::create(Socket::destFromString("::ffff:127.0.0.1", 80), std::make_shared<TcpEchoWithGreeting>()); });
//	runUnit("udpconn", [] () { UdpSocket::create(Socket::destFromString("::ffff:127.0.0.1", 1223), std::make_shared<UdpEcho>()); });
//	runUnit("udpconn", [] () { UdpSocket::create(1223, std::make_shared<UdpEcho>()); });
//	runUnit("udpconn", [] () { UdpSocket::create(Socket::destFromString("::ffff:127.0.0.1", 53), std::make_shared<UdpEcho>()); });
//	runUnit ("tcpechotest", [] () {
//		TcpListener::create(1025, [] () { return std::make_shared<TcpEchoWithGreeting>(); } );
//		TcpConn::create(Socket::destFromString("::ffff:127.0.0.1", 1025), std::make_shared<TcpEchoWithGreeting>()); });
//		TcpConn::create( Socket::destFromString("::1", 1025), std::make_shared<TcpEchoWithGreeting>());
	runUnit("httpproxy", [] () { TcpListener::create(1024, [] () { return std::make_shared<HttpProxy>(); } ); });
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

//void operator  delete(void* /*ptr*/, std::size_t /*sz*/) noexcept {
//	__builtin_trap();
//}
