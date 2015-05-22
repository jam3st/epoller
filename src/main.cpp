#include <iostream>
#include <functional>
#include <regex>
#include <unistd.h>

#include "engine.hpp"
#include "udpsocket.hpp"
#include "tcplistener.hpp"
#include "tcpconn.hpp"


using namespace Sb;
namespace Sb {
      extern size_t test_ocb();
}

void runUnit(const std::string id, std::function<void()> what) {
      try {
            Engine::init();
            what();
            Engine::start(1);
      } catch(const std::exception& e) {
            std::cerr << e.what() << "\n";
      } catch(...) {
            std::cerr << id << " " << "exception" << std::endl;
      }
}

std::pair<const Bytes::iterator, const Bytes::iterator> findFirstPattern(const Bytes::iterator begin, const Bytes::iterator end, const Bytes& pattern) {
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

      return std::make_pair(end, end);
}

class Remote : public TcpStreamIf {
      public:
            Remote(const std::weak_ptr<TcpStream> ep, Bytes& initWrite) : ep(ep), initWrite(initWrite) {
                  logDebug("Remote created initial " + std::to_string(initWrite.size()));
            }
            virtual ~Remote() {
                  logDebug("~Remote destroyed");
            }

            virtual void disconnect() override {
                  logDebug("Remote disconnect");
                  auto ref = tcpStream.lock();

                  if(ref) {
                        ref->disconnect();
                  }
            }

            virtual void received(const Bytes& x) override {
                  logDebug("Remote received");
                  auto ref = ep.lock();

                  if(ref) {
                        ref->queueWrite(x);
                  }
            }


            virtual void writeComplete() override {
                  logDebug("Remote onReadyToWrite");
            }

            virtual void disconnected() override {
                  logDebug("Remote onDisconnect");
                  auto ref = ep.lock();
                  if(ref) {
                        ref->disconnect();
                  }
            }

            void doWrite(const Bytes& x) {
                  auto ref = tcpStream.lock();

                  if(ref) {
                        if(initWrite.size() > 0) {
                              ref->queueWrite(initWrite);
                              initWrite.resize(0);
                        }
                        ref->queueWrite(x);
                  } else {
                        for(auto c : x) {
                              initWrite.push_back(c);
                        }
                  }
            }

            virtual void timeout(const size_t /*timerId*/) override {
                  //                s.setTimer(5, NanoSecs(1000000000));
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

            virtual void disconnect() override {
                  logDebug("HttpProxy disconnect");
                  auto ref = tcpStream.lock();

                  if(ref) {
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

                  header.insert(header.end(), x.begin(), x.end());

                  if(header.size() < 4) {
                        logDebug("HttpProxy onReadCompleted without enough data");
                        return;
                  }

                  auto doubleCr = findFirstPattern(header.begin(), header.end(), { '\r', '\n', '\r', '\n' });

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
                        logDebug("line: " + std::string { getPost.begin(), getPost.end() });
                        auto it = findFirstPattern(getPost.begin(), getPost.end(), { 'H', 'O', 'S', 'T', ':', '?' });

                        if(it.first != it.second) {
                              host = std::string { it.second + 1, getPost.end() };
                              auto delim = host.find_first_of(':');

                              if(delim != std::string::npos) {
                                    host.resize(delim);
                              }

                              logDebug("host: " + host);
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
                  TcpConnection::create(host, port, sp);
                  ep = sp;

                  if(port == 443) {
                        auto ref = tcpStream.lock();

                        if(ref) {
                              ref->queueWrite({ 'H', 'T', 'T', 'P', '/', '1', '.', '1', ' ', '2', '0', '0', ' ', 'O', 'K', '\r', '\n', '\r', '\n' });
                        } else {
                              disconnected();
                        }
                  }
            }

            virtual void writeComplete() override {
                  logDebug("HttpProxy onReadyToWrite");
            }

            virtual void disconnected() override {
                  logDebug("HttpProxy::disconnected");
                  auto ref = ep.lock();

                  if(ref) {
                        ref->disconnect();
                  }
            }

            virtual void timeout(const size_t /*timerId*/) override {
                  //                s.setTimer(5, NanoSecs(1000000000));
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
      public:
            ExitTimer() : timer0 { std::bind(&ExitTimer::timedout0, this) },
                          timer1 { std::bind(&ExitTimer::timedout1, this) },
                          timer2 { std::bind(&ExitTimer::timedout2, this) },
                          timer3 { std::bind(&ExitTimer::timedout3, this) },
                          timer4 { std::bind(&ExitTimer::timedout4, this) } {
            }

            void setTimer() {
                  Engine::cancelTimer(this, &timer0);
                  Engine::setTimer(this, &timer0, NanoSecs { 10'000'000'000 });
                                   Engine::setTimer(this, &timer4, NanoSecs{ 4'000'000'000
                                                           });
                  Engine::setTimer(this, &timer3, NanoSecs { 3'000'000'000 });
                                   Engine::setTimer(this, &timer1, NanoSecs{ 1'000'000'000
                                                           });
                  Engine::setTimer(this, &timer2, NanoSecs { 2'000'000'000 });
                                   Engine::cancelTimer(this, &timer1);

                 }

     private:
     void timedout0() {
     std::cerr << "timedout0" << std::endl;
}
     void timedout1() {
     std::cerr << "timedout1" << std::endl;
}
     void timedout2() {
     std::cerr << "timedout2" << std::endl;
}
     void timedout3() {
     std::cerr << "timedout3" << std::endl;
}
     void timedout4() {
     std::cerr << "timedout4" << std::endl;
}
     Timer timer0;
     Timer timer1;
     Timer timer2;
     Timer timer3;
     Timer timer4;

};

class NameServer : public UdpSocketIf {
      public:
            virtual void connected(const InetDest&) override {
                  logDebug("NameServer::connected");
            }

            virtual void disconnected() override {
                  logDebug("NameServer::disconnected");
            }
            virtual void received(InetDest const& addr, Bytes const& what) override {
                  logDebug("NameServer::received from " + addr.toString() + toHexString(what));
            }
            virtual void notSent(InetDest const& addr, const Bytes&) override {
                  logDebug("NameServer::notSent");
            }
            virtual void writeComplete() override {
                  logDebug("NameServer::writeComplete");
            }

            virtual void disconnect() override {
                  logDebug("NameServer::disconnect");
            }
};

int main(const int, const char*const argv[]) {
	  ::close(0);
//	runUnit("timer", [ ]() {
//		auto ref = std::make_shared<ExitTimer>();
//		Engine::addTimer(ref);
//		ref->setTimer();
//	});
//      runUnit("resolve",[] () {
//            Engine::resolver().resolve(std::make_shared<ResolveNameSy>(), "ipv6.google.com", Resolver::AddrPref::AnyAddr);
//      });
      runUnit("httpproxy",[] () {
            TcpListener::create(1024, [ ]() {
                   return std::make_shared<HttpProxy>();
            });
      });
//      runUnit("nameserver",[] () {
//            UdpSocket::create(1024, std::make_shared<NameServer>());
//      });
      return 0;
}