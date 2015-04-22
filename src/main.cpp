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
		Remote(const std::weak_ptr<TcpStream> ep, Bytes& initWrite)
			:ep(ep),
			 initWrite(initWrite) {
			logDebug("Remote created initial " + std::to_string(initWrite.size()));
		}
		virtual ~Remote() {
			logDebug("~Remote destroyed");
		}

		virtual void connected(const struct InetDest& dest) override {
			logDebug("Remote connected " + dest.toString() + " initial " + std::to_string(initWrite.size()));
			if(auto ref = tcpStream.lock()) {
				ref->queueWrite(initWrite);
			}
		}

		virtual void disconnect() override {
			logDebug("Remote disconnect");
			if(auto ref = tcpStream.lock()) {
				ref->disconnect();
			}
		}

		virtual void received(const Bytes& x) override {
			logDebug("Remote received");
			if(auto ref = ep.lock()) {
				ref->queueWrite(x);
			}
		}

		virtual void writeComplete() override {
			logDebug("Remote onReadyToWrite");
		}

		virtual void disconnected() override {
			logDebug("Remote onDisconnect");
			if(auto ref = ep.lock()) {
				ref->disconnect();
			}
			logDebug("Remote onDisconnect ok");
		}

		void doWrite(const Bytes& x) {
			auto ref = tcpStream.lock();
			if(ref != nullptr) {
				ref->queueWrite(x);
			} else {
				logDebug("Remote doWrite deferred");
				for(auto c : x) {
					initWrite.push_back(c);
				}
			}
		}

		virtual void timeout(const size_t /*timerId*/) override {
//			s.setTimer(5, NanoSecs(1000000000));
			logDebug("blah expired");
		}
	private:
		std::weak_ptr<TcpStream> ep;
		Bytes initWrite;
};

class HttpProxy : public TcpStreamIf {
	public:
		HttpProxy() {
			logDebug("HttpProxy::HttpProxy");
		}

		virtual ~HttpProxy() {
			logDebug("~HttpProxy destroyed");
		}

		virtual void connected(const struct InetDest& dest) override {
			logDebug("HttpProxy connected " + dest.toString());
		}

		virtual void disconnect() override {
			logDebug("HttpProxy disconnect");
			auto ref = tcpStream.lock();
			if(ref != nullptr) {
				ref->disconnect();
			}
		}

		virtual void received(const Bytes& x) override {
			logDebug("HttpProxy onReadCompleted");
			auto ref = ep.lock();
			if(ref != nullptr) {
				logDebug("HttpProxy onReadCompleted to stream");
				ref->doWrite(x);
				return;
			}
			std::cerr << std::string(x.begin(), x.end());
			header.insert(header.end(), x.begin(), x.end());

			if(header.size() < 4) {
				logDebug("HttpProxy onReadCompleted without enough data");
				return;
			}
			auto doubleCr = findFirstPattern(header.begin(), header.end(), { '\r', '\n', '\r', '\n'});
			if(doubleCr.first == doubleCr.second) {
				logDebug("HttpProxy onReadCompleted without double CR");
				return;
			}
			std::string host;
			for(auto start = header.begin(); start != header.end();) {
				while(*start == '\n') {
					++start;
				}
				auto crPos = std::find(start, header.end(), '\r');
				if(crPos == header.end()) {
					break;
				}
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

				if(crPos + 1 == header.end()) {
					break;
				}
				start = crPos + 1;
			}

			uint16_t port = 80;
			auto it = findFirstPattern(header.begin(), header.end(), { 'C', 'O', 'N', 'N', 'E', 'C', 'T', ' ' });
			if(it.first != it.second) {
				header.resize(0);
				port = 443;
			}
			auto sp = std::make_shared<Remote>(tcpStream, header);
			ep = sp;
			TcpConnection::create(host, port, sp);
			if(port == 443) {
				auto ref = tcpStream.lock();
				if(ref != nullptr) {
					ref->queueWrite({'H', 'T', 'T', 'P', '/', '1', '.', '1', ' ',
										   '2', '0', '0', ' ', 'O', 'K', '\r', '\n', '\r', '\n'});
				} else {
					logDebug("HttpProxy tcpStream missing on connect");
				}
			}
		}

		virtual void writeComplete() override {
			logDebug("HttpProxy onReadyToWrite");
		}

		virtual void disconnected() override {
			logDebug("HttpProxy::disconnected");
		}

		virtual void timeout(const size_t /*timerId*/) override {
//			s.setTimer(5, NanoSecs(1000000000));
			logDebug("blah expired");
		}
	private:
		Bytes header;
		std::weak_ptr<Remote> ep;
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
	void handleTimer(const size_t) override {
std::cerr << "Exit" << std::endl;
		Engine::removeTimer(this);
	}
};

int main (const int, const char* const argv[]) {
	::close(0);

//	runUnit("timer", [] () { auto ref = std::make_shared<ExitTimer>(); Engine::addTimer(ref); ref->setTimer(0, NanoSecs{1'000'000'000 }); });
//	runUnit("resolve", [] () { Engine::resolver().resolve(std::make_shared<ResolveNameSy>(), "www.google.com.au", Resolver::AddrPref::Ipv4Only); });
	runUnit("httpproxy", [] () { TcpListener::create(1024, [] () {
		std::cerr << "Creating httproxy" << std::endl;
		return std::make_shared<HttpProxy>(); } ); });

	std::cerr << argv[0] << " exited." << std::endl;

	return 0;
}

//void operator  delete(void* /*ptr*/, std::size_t /*sz*/) noexcept {
//	__builtin_trap();
//}
