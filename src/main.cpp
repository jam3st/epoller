#include <iostream>
#include <functional>
#include <regex>
#include <unistd.h>
#include "engine.hpp"
#include "udpsocket.hpp"
#include "tcplistener.hpp"
#include "tcpconn.hpp"
#include <random>

using namespace Sb;
namespace Sb {
      extern size_t test_ocb();
}

void runUnit(const std::string id, std::function<void()> what) {
      try {
            Engine::init();
            what();
            Engine::start();
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

class TcpSplat : public TcpStreamIf {
public:
      TcpSplat() : count(0) {
            logDebug("TcpSplat");
      }

      virtual ~TcpSplat() {
            logDebug("~TcpSplat destroyed");
      }

      virtual void received(const Bytes& x) override {
            logDebug("TcpSplat received " + std::to_string(x.size()));
      }

      virtual void disconnected() override {
            logDebug("TcpSplat onDisconnect");
      }

      virtual void writeComplete() override {
            std::uniform_int_distribution<uint32_t> randUint(1, 10000);
            auto thisRun = randUint(rng);
            logDebug("TcpSplat writeComplete " + std::to_string(thisRun));
            while(--thisRun != 0) {
                  std::stringstream stream;
                  stream << std::setfill('0') << std::setw(8) << std::dec << count++ << std::endl;
                  auto str = stream.str();
                  Bytes x(str.begin(), str.end());
                  auto ref = tcpStream.lock();
                  if(ref) {
                        ref->queueWrite(x);
                  }
            }
      }

private:
      size_t count;
      std::mt19937 rng;
};

class TcpSink : public TcpStreamIf {
public:
      TcpSink() {
            logDebug("TcpSink");
      }

      virtual ~TcpSink() {
            logDebug("~TcpSink destroyed");
      }

      virtual void received(const Bytes& x) override {
            logDebug("TcpSink received " + std::to_string(x.size()));
      }

      virtual void disconnected() override {
            logDebug("TcpSink onDisconnect");
      }

      virtual void writeComplete() override {
            logDebug("TcpSink writeComplete");
      }
};

class TcpEcho : public TcpStreamIf {
public:
      TcpEcho() : initWrite({}) {
            logDebug("TcpEcho created initial " + std::to_string(initWrite.size()));
      }

      virtual ~TcpEcho() {
            logDebug("~TcpEcho destroyed");
      }

      virtual void received(const Bytes& x) override {
            logDebug("TcpEcho received");
            auto ref = tcpStream.lock();
            if(ref) {
                  ref->queueWrite(x);
            }
      }

      virtual void disconnected() override {
      }

      virtual void writeComplete() override {
            logDebug("TcpEcho writeComplete " + std::to_string(initWrite.size()));
            std::lock_guard<std::mutex> sync(lock);
            auto ref = tcpStream.lock();
            if(initWrite.size() > 0) {
                  ref->queueWrite(initWrite);
                  initWrite.resize(0);
            }
      }

private:
      Bytes initWrite;
      std::mutex lock;
};

class Remote;

class HttpProxy : public TcpStreamIf {
public:
      virtual ~HttpProxy();
      virtual void received(const Bytes& x) override;
      virtual void writeComplete() override;
      virtual void disconnected() override;
      virtual void disconnect();
      virtual void queueWrite(Bytes const& x);
      virtual void disconnectRemote() const;
private:
      Bytes header;
      std::weak_ptr<Remote> ep;
      InetDest remoteDest;
      bool disconnectWhenCompleted = false;
      std::mutex lock;
};

class Remote : public TcpStreamIf {
public:
      Remote(const std::weak_ptr<TcpStreamIf> ep, Bytes& initWrite) : ep(ep), initWrite(initWrite) {
      }

      virtual ~Remote() {
      }

      virtual void disconnect() {
            assert(!epDisconnected, "Already disconnected");
            epDisconnected = true;
            auto ref = tcpStream.lock();
            if(ref) {
                  if(ref->writeQueueEmpty() && epDisconnected) {
                        ref->disconnect();
                  }
            }
      }

      virtual void received(const Bytes& x) override {
            auto ref = ep.lock();
            if(ref) {
                  reinterpret_cast<HttpProxy*>(ref.get())->queueWrite(x);
            }
      }

      virtual void disconnected() override {
            auto ref = ep.lock();
            if(ref) {
                  reinterpret_cast<HttpProxy*>(ref.get())->disconnect();
            }
      }

      virtual void writeComplete() override {
            std::lock_guard<std::mutex> sync(lock);
            auto ref = tcpStream.lock();
            if(ref) {
                  if(initWrite.size() > 0) {
                        ref->queueWrite(initWrite);
                        initWrite.resize(0);
                  } else if(epDisconnected) {
                        ref->disconnect();
                  }
            }
      }

      void doWrite(const Bytes& x) {
            std::lock_guard<std::mutex> sync(lock);
            auto ref = tcpStream.lock();
            if(ref) {
                  if(initWrite.size() > 0) {
                        ref->queueWrite(initWrite);
                        initWrite.resize(0);
                  }
                  ref->queueWrite(x);
            } else {
                  initWrite.insert(initWrite.end(), x.begin(), x.end());
            }
      }

private:
      std::weak_ptr<TcpStreamIf> ep;
      Bytes initWrite;
      std::mutex lock;
      bool epDisconnected = false;
};

HttpProxy::~HttpProxy() {
}

void HttpProxy::received(const Bytes& x) {
      auto ref = ep.lock();
      if(ref) {
            ref->doWrite(x);
            return;
      } else {
            bool first = header.size() == 0;
            if(!first)
                  logDebug("Adding b4: " + std::string(header.begin(), header.end()));
            header.insert(header.end(), x.begin(), x.end());
            if(!first)
                  logDebug("Adding l8: " + std::string(header.begin(), header.end()));
      }
      if(header.size() < 4) {
            logDebug("HttpProxy onReadCompleted is too short");
            logDebug("Received: " + std::string(header.begin(), header.end()));
            logDebug("Received: " + toHexString(header));
            return;
      }
      auto doubleCr = findFirstPattern(header.begin(), header.end(), {'\r', '\n', '\r', '\n'});
      if(doubleCr.first == doubleCr.second) {
            logDebug("HttpProxy onReadCompleted without double CR");
            logDebug("Received " + std::string(header.begin(), header.end()));
            logDebug("Received: " + toHexString(header));
            auto ref = tcpStream.lock();
            if(ref) {
                  ref->disconnect();
            }
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
            auto it = findFirstPattern(getPost.begin(), getPost.end(), {'H', 'O', 'S', 'T', ':', '?'});
            if(it.first != it.second) {
                  host = std::string {it.second + 1, getPost.end()};
                  auto delim = host.find_first_of(':');
                  if(delim != std::string::npos) {
                        host.resize(delim);
                  }
            }
            if(crPos + 1 == header.end()) {
                  break;
            }
            start = crPos + 1;
      }
      uint16_t port = 80;
      auto it = findFirstPattern(header.begin(), header.end(), {'C', 'O', 'N', 'N', 'E', 'C', 'T', ' '});
      if(it.first != it.second) {
            header.erase(header.begin(), doubleCr.second + 1);
            port = 443;
      };

      auto sp = std::make_shared<Remote>(shared_from_this(), header);
      TcpConnection::create(host, port, sp);
      ep = sp;
      if(port == 443) {
            auto ref = tcpStream.lock();
            if(ref) {
                  ref->queueWrite({'H', 'T', 'T', 'P', '/', '1', '.', '1', ' ', '2', '0', '0', ' ', 'O', 'K', '\n', '\n'});
            }
      } else {
            // todo
      }
}

void HttpProxy::writeComplete() {
      std::lock_guard<std::mutex> sync(lock);
      auto ref = tcpStream.lock();
      if(ref) {
            if(!remoteDest.valid && ref->didConnect()) {
                  remoteDest = ref->endPoint();
            }
            if(disconnectWhenCompleted) {
                  disconnectRemote();
            }
      }
}

void HttpProxy::disconnected() {
      std::lock_guard<std::mutex> sync(lock);
      disconnectRemote();
}

void HttpProxy::disconnect() {
      auto tcpRef = tcpStream.lock();
      if(tcpRef) {
            if(!tcpRef->writeQueueEmpty()) {
                  logDebug("DC NOT EMPTY");
                  disconnectWhenCompleted = true;
                  return;
            }
      }
}

void HttpProxy::queueWrite(Bytes const& x) {
      auto tcpRef = tcpStream.lock();
      if(tcpRef) {
            tcpRef->queueWrite(x);
      }
}

void HttpProxy::disconnectRemote() const {
      auto ref = ep.lock();
      if(ref) {
            ref->disconnect();
      }
}

class ResolveNameSy : public ResolverIf {
public:
      virtual void notResolved() override {
            logDebug("ResolveNameSy	error");
      }

      virtual void resolved(const IpAddr& /*addr*/) override {
            logDebug("ResolveNameSy	ok");
      }
};

class PingTimer : public Runnable {
public:
      PingTimer() {
      }

      void set() {
            timer = std::make_unique<Event>(shared_from_this(), std::bind(&PingTimer::timedout, this));
            Engine::setTimer(timer.get(), NanoSecs {ONE_SEC_IN_NS});
      }

private:
      void timedout() {
            logDebug("PING");
            Engine::setTimer(timer.get(), NanoSecs {ONE_SEC_IN_NS});
      }

private:
      std::unique_ptr<Event> timer;
};

class ExitTimer : public Runnable {
public:
      ExitTimer() {
      }

      void setTimers() {
            timer0 = std::make_unique<Event>(shared_from_this(), std::bind(&ExitTimer::timedout, this, 0));
            timer1 = std::make_unique<Event>(shared_from_this(), std::bind(&ExitTimer::timedout, this, 1));
            timer2 = std::make_unique<Event>(shared_from_this(), std::bind(&ExitTimer::timedout, this, 2));
            timer3 = std::make_unique<Event>(shared_from_this(), std::bind(&ExitTimer::timedout, this, 3));
            timer4 = std::make_unique<Event>(shared_from_this(), std::bind(&ExitTimer::timedout, this, 4));
            Engine::cancelTimer(timer0.get());
            Engine::setTimer(timer0.get(), NanoSecs {5 * ONE_SEC_IN_NS});
            Engine::setTimer(timer4.get(), NanoSecs {4 * ONE_SEC_IN_NS});
            Engine::setTimer(timer3.get(), NanoSecs {3 * ONE_SEC_IN_NS});
            Engine::setTimer(timer1.get(), NanoSecs {1 * ONE_SEC_IN_NS});
            Engine::setTimer(timer2.get(), NanoSecs {2 * ONE_SEC_IN_NS});
            Engine::cancelTimer(timer1.get());
      }

private:
      void timedout(int const id) {
            logDebug("timedout timer " + std::to_string(id));
      }

private:
      std::unique_ptr<Event> timer0;
      std::unique_ptr<Event> timer1;
      std::unique_ptr<Event> timer2;
      std::unique_ptr<Event> timer3;
      std::unique_ptr<Event> timer4;
};

class EchoUdp : public UdpSocketIf {
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

int main(const int, const char* const argv[]) {
      ::close(0);
      //      auto exitTimer = std::make_shared<ExitTimer>();
      //      runUnit("timer", [&exitTimer]() {
      //            exitTimer->setTimers();
      //      });
      //      exitTimer.reset();
      //      runUnit("resolve",[] () {
      //           Engine::resolver().resolve(std::make_shared<ResolveNameSy>(), "asdasdasd", Resolver::AddrPref::AnyAddr);
      //      });
      //      runUnit("resolve",[] () {
      //            Engine::resolver().resolve(std::make_shared<ResolveNameSy>(), "ipv6.google.com", Resolver::AddrPref::AnyAddr);
      //      });
      //      runUnit("echoudp", []() {
      //            UdpSocket::create(1024, std::make_shared<EchoUdp>());
      //      });
      //      runUnit("echotcp", []() {
      //            TcpListener::create(1024, []() {
      //                  return std::make_shared<TcpEcho>();
      //            });
      //      });
      //      runUnit("sinktcp", []() {
      //            TcpListener::create(1024, []() {
      //                  return std::make_shared<TcpSink>();
      //            });
      //      });
      //      runUnit("splattcp", []() {
      //            TcpListener::create(1024, []() {
      //                  return std::make_shared<TcpSplat>();
      //            });
      //      });
      runUnit("httpproxy", []() {
            TcpListener::create(1024, []() {
                  return std::make_shared<HttpProxy>();
            });
      });
      return 0;
}
