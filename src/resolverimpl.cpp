#include <utility>
#include "resolverimpl.hpp"
#include "udpsocket.hpp"
#include "engine.hpp"

namespace Sb {
      ResolverImpl::ResolverImpl() : request(0) {
      }

      ResolverImpl::~ResolverImpl() {
            logDebug("ResolverImpl::~ResolverImpl");
            std::lock_guard<std::mutex> sync(lock);
            resQueries.clear();
      }

      class UdpResolver : public UdpSocketIf {
            public:
                  UdpResolver(QueryHandlerIf& handler, const uint16_t requestNo, const std::string& name, const Query::Qtype qType) : handler(
                              handler), requestNo(requestNo), name(name), qType(qType) {
                  }

                  virtual void connected(const InetDest& to) override {
                        logDebug("UdpResolver::connected " + to.toString());
                        auto sock = udpSocket.lock();

                        if(sock) {
                              sock->queueWrite(to, Query::resolve(requestNo, name, qType));
                        }
                  }

                  virtual void received(const InetDest& from, const Bytes& w) override {
                        logDebug("UdpResolver::received " + from.toString() + " " + std::to_string(w.size()));
                        disconnect();
                        handler.requestComplete(requestNo, Query::decode(w));
                  }

                  virtual void notSent(const InetDest& to, const Bytes& w) override {
                        logDebug("UdpResolver::notSent " + to.toString() + " " + std::to_string(w.size()));
                        disconnect();
                        handler.requestError(requestNo);
                  }
                  virtual void writeComplete() override {
                        logDebug("UdpEcho::onWriteCompleted");
                  }

                  virtual void disconnected() override {
                        logDebug("UdpResolver::onWriteCompleted");
                  }

                  virtual void disconnect() override {
                        logDebug("UdpResolver::onWriteCompleted");
                        auto sock = udpSocket.lock();

                        if(sock) {
                              sock->disconnect();
                        }
                  }

            private:
                  QueryHandlerIf& handler;
                  const uint16_t requestNo;
                  const std::string name;
                  const Query::Qtype qType;
      };

      bool
      ResolverImpl::Names::get(Resolver::AddrPref const& prefs, IpAddr& addr) const {
            addr.d =  {};
            bool found = false;
            bool once = false;
            auto i = lastAccessIndex;
            const auto num = addrs.size();

            if(num == 0) {
                  return false;
            }

            while(!once || i != lastAccessIndex) {
                  if(++i >= num) {
                        i = 0;
                        once = true;
                  }

                  auto isIpv4 = addrs[i].isIpv4Addr();
                  if(prefs == Resolver::AddrPref::AnyAddr || (isIpv4 && prefs == Resolver::AddrPref::Ipv4Only)) {
                        addr.d = addrs[i].d;
                        found = true;
                        break;
                  } else if(!isIpv4 && prefs == Resolver::AddrPref::Ipv6Only) {
                        addr.d = addrs[i].d;
                        found = true;
                        break;
                  }
            }

            lastAccessIndex = i;
            return found;
      }

      void
      ResolverImpl::Names::put(const Query::Qanswer& ans) {
            logDebug("Resolver::put " + std::to_string(ans.addr.size()));

            for(const auto& addr : ans.addr) {
                  addrs.push_back(addr);
            }
      }

      void ResolverImpl::resolve(
            std::shared_ptr<ResolverIf> client,
            const std::string& name,
            const Resolver::AddrPref& prefs,
            const NanoSecs& timeout,
            const InetDest& nameServer) {
            std::lock_guard<std::mutex> sync(lock);
            auto it = byName.find(name);

            if(it != byName.end()) {
                  logDebug("resolve cache hit");
                  IpAddr addr;

                  if(it->second.get(prefs, addr)) {
                        client->resolved(addr);
                  } else {
                        client->error();
                  }
            } else {
                  logDebug("adding request " + std::to_string(request));
                  resQueries[request] = { prefs, { }, { }, std::make_unique<Event>(std::bind(&ResolverImpl::queryTimedout, this, request)) };
                  resQueries[request].prefs = prefs;

                  if(prefs != Resolver::AddrPref::Ipv4Only) {
                        auto resolver = std::make_shared<UdpResolver>(*this, request, name, Query::Qtype::Aaaa);
                        UdpSocket::create(nameServer, resolver);
                        resQueries[request].clients.push_back(client);
                  }

                  if(prefs != Resolver::AddrPref::Ipv6Only) {
                        auto resolver = std::make_shared<UdpResolver>(*this, request, name, Query::Qtype::A);
                        resQueries[request].resolvers.push_back(resolver);
                        UdpSocket::create(nameServer, resolver);
                        resQueries[request].clients.push_back(client);
                  }

                  Engine::setTimer(this, resQueries[request].timeout.get(), timeout);
                  request++;
            }
      }

      void
      ResolverImpl::cancel(const ResolverIf* /*client*/) {
            std::lock_guard<std::mutex> sync(lock);
            //          auto it = byName.find(&name);
      }

      void
      ResolverImpl::destroy() {
      }

      void ResolverImpl::queryTimedout(std::uint16_t queryId) {
            logDebug("ResolverImpl::queryTimedout " + std::to_string(queryId));
            std::shared_ptr<ResolverIf> client;
            {
                  std::lock_guard<std::mutex> sync(lock);
                  auto it = resQueries.find(queryId);

                  if(it == resQueries.end()) {
                        logDebug("queryTimedout request not found: " + std::to_string(queryId));
                        return;
                  }

                  while(it != resQueries.end()) {
                        client = it->second.clients.back();
                        auto resolver = it->second.resolvers.back().lock();

                        if(resolver) {
                              resolver->disconnect();
                        }

                        resQueries.erase(it++);
                  }
            }
            client->error();
      }

      void ResolverImpl::requestError(uint16_t const reqNo) {
            logDebug("ResolverImpl::requestError " + std::to_string(reqNo));
      }

      void
      ResolverImpl::requestComplete(std::uint16_t const reqNo, Query::Qanswer const& ans) {
            logDebug("ResolverImpl::requestComplete " + std::to_string(ans.reqNo));
            std::shared_ptr<ResolverIf> client;
            Resolver::AddrPref prefs;
            IpAddr ipAddr;
            bool requestComplete = false;
            bool completedError = true;
            {
                  std::lock_guard<std::mutex> sync(lock);
                  auto it = resQueries.find(reqNo);

                  if(it == resQueries.end()) {
                        logDebug("request not found: " + std::to_string(ans.reqNo));
                        return;
                  }

                  prefs = it->second.prefs;
                  client = it->second.clients.back();
                  it->second.clients.pop_back();
                  logDebug("is valid " + ans.name);
                  logDebug("is valid " + std::to_string(ans.valid));

                  if(ans.valid) {
                        auto bn = byName.find(ans.name);

                        if(bn == byName.end()) {
                              logDebug("adding new name " + ans.name);
                              byName.insert(std::make_pair(ans.name, Names()));
                              bn = byName.find(ans.name);
                        }

                        bn->second.put(ans);
                  }

                  requestComplete = it->second.clients.size() == 0;

                  if(requestComplete) {
                        auto bn = byName.find(ans.name);

                        if(bn == byName.end()) {
                              logDebug("name not found at all " + ans.name);
                              completedError = true;
                        } else {
                              if(bn->second.get(prefs, ipAddr)) {
                                    completedError = false;
                              } else {
                                    logDebug("name not found " + ans.name);
                                    completedError = true;
                              }
                        }

                        Engine::cancelTimer(this, it->second.timeout.get());
                        resQueries.erase(it);
                  }
            }

            if(requestComplete) {
                  logDebug("request Complete " + std::to_string(ans.reqNo));

                  if(completedError) {
                        logDebug("request complete with error " + std::to_string(ans.reqNo));
                        client->error();
                  } else {
                        logDebug("request complete as " + ipAddr.toString() + " " + std::to_string(ans.reqNo));
                        client->resolved(ipAddr);
                  }
            } else {
                  logDebug("request inComplete " + std::to_string(ans.reqNo));
            }
      }
}
