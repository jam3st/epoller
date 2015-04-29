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
		Remote(const std::shared_ptr<TcpStream>& ep, Bytes& initWrite)
			:ep(ep),
			 initWrite(initWrite) {
			logDebug("Remote created");
		}
		virtual ~Remote() {
			logDebug("~Remote destroyed");
		}

		virtual void connected(const struct InetDest& dest) override {
			logDebug("Remote connected " + dest.toString());
			stream()->queueWrite(initWrite);
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
		}

		void doWrite(const Bytes& x) {
			auto ref = stream();
			if(ref == nullptr) {
				for(auto c : x) {
					initWrite.push_back(c);
				}
			} else {
				ref->queueWrite(x);
			}
		}

		virtual void timeout(const size_t /*timerId*/) override {
//			s.setTimer(5, NanoSecs(1000000000));
			logDebug("blah expired");
		}
	private:
		std::shared_ptr<TcpStream> ep;
		Bytes initWrite;
};

class HttpProxy : public TcpStreamIf {
	public:
		HttpProxy()	 {
		}

		virtual ~HttpProxy() {
			logDebug("~HttpProxy destroyed");
		}

		virtual void connected(const struct InetDest& dest) override {
			logDebug("HttpProxy connected " + dest.toString());
		}

		virtual void received(const Bytes& x) override {
			logDebug("HttpProxy onReadCompleted");
			if(ep != nullptr) {
				logDebug("HttpProxy onReadCompleted to stream");
				ep->doWrite(x);
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
					auto delim = host.find_first_of(':');
					if(delim != std::string::npos) {
						host.resize(delim);
					}
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
			ep = std::make_shared<Remote>(stream(), header);
			TcpConnection::create(host, port, ep);
			if(port == 443) {
				stream()->queueWrite( {'H', 'T', 'T', 'P', '/', '1', '.', '1', ' ',
									 '2', '0', '0', ' ', 'O', 'K', '\r', '\n', '\r', '\n' } );
			}
		}

		virtual void writeComplete() override {
			logDebug("HttpProxy onReadyToWrite");
		}

		virtual void disconnected() override {
			logDebug("HttpProxy onDisconnect");
		}

		virtual void timeout(const size_t /*timerId*/) override {
//			s.setTimer(5, NanoSecs(1000000000));
			logDebug("blah expired");
		}
	private:
		Bytes header;
		std::shared_ptr<Remote> ep;
};

class ResolveNameSy : public ResolverIf {
	public:
		virtual void error() override {
			logDebug("ResolveNameSy	error");
		}
		virtual void resolved(const IpAddr& /*addr*/) override {
			logDebug("ResolveNameSy	ok");
		}
};

class ExitTimer : public TimeEvent {
	void handleTimer()  {
std::cerr << "ExitTimer" << std::endl;
		Engine::removeTimer(this);
	}
};

int main (const int, const char* const argv[]) {
	::close(0);
	runUnit("timer", [] () { auto ref = std::make_shared<ExitTimer>(); Engine::addTimer(ref); ref->setTimer(std::mem_fn(handleTimer), NanoSecs{1'000'000'000 }); });
	runUnit("resolve", [] () { Engine::resolver().resolve(std::make_shared<ResolveNameSy>(), "www.google.com.au", Resolver::AddrPref::Ipv4Only); });
	runUnit("httpproxy", [] () { TcpListener::create(1024, [] () { return std::make_shared<HttpProxy>(); } ); });

	std::cerr << argv[0] << " exited." << std::endl;

	return 0;
}

//void operator  delete(void* /*ptr*/, std::size_t /*sz*/) noexcept {
//	__builtin_trap();
//}
