#include <utility>
#include "resolverimpl.hpp"
#include "logger.hpp"
#include "udpsocket.hpp"
#include "engine.hpp"

namespace Sb {
	ResolverImpl::ResolverImpl()
		: request(0) {
	}

	ResolverImpl::~ResolverImpl() {
	}

	inline
	bool
	isIpv4Addr(const IpAddr& addr) {
		const int prefixLen = 12;
		std::array<uint8_t, prefixLen> ip4Pref { {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff} };
		for(auto i = 0; i < prefixLen; ++i)	{
			if(addr[i] != ip4Pref[i]) {
				return false;
			}
		}
		return true;
	}

	class UdpResolver : public UdpSocketIf {
		public:
			UdpResolver(QueryHandlerIf& handler, const uint16_t requestNo, const std::string& name, const NanoSecs& timeout, const Query::Qtype qType)
				: handler(handler),
				  requestNo(requestNo),
				  name(name),
				  queryTimeout(timeout),
				  qType(qType) {
			}

			virtual void disconnected() override {
				logDebug("UdpResolver::disconnected");
			}

			virtual void connected(const InetDest& to) override {
				logDebug("UdpResolver::connected " + to.toString());
				auto sock = socket();
				sock->setTimer(0, queryTimeout);
				sock->queueWrite(to, Query::resolve(requestNo, name, qType));
			}

			virtual void received(const InetDest& from, const Bytes& w) override {
				logDebug("UdpEcho::received " + from.toString() + " " + std::to_string(w.size()));
				handler.requestComplete(requestNo, Query::decode(w));
				socket()->disconnect();
			}

			virtual void notSent(const InetDest& to, const Bytes& w) override {
				logDebug("UdpEcho::notSent " + to.toString() + " " + std::to_string(w.size()));
			}
			virtual void writeComplete() override {
				logDebug("UdpEcho::onWriteCompleted");
			}

			virtual void timeout(const size_t) override {
				logDebug("UdpEcho::onTimerExpired");
			}

		private:
			QueryHandlerIf& handler;
			const uint16_t requestNo;
			const std::string name;
			const NanoSecs queryTimeout;
			const Query::Qtype qType;
	};

	bool
	ResolverImpl::Names::get(const Resolver::AddrPref& prefs, IpAddr& addr) const {
		addr =  IpAddr { {} };
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
			auto isIpv4 = isIpv4Addr(addrs[i]);
			if(prefs == Resolver::AddrPref::AnyAddr || (isIpv4 && prefs == Resolver::AddrPref::Ipv4Only)) {
				addr = addrs[i];
				found = true;
				break;
			} else if(!isIpv4 && prefs == Resolver::AddrPref::Ipv6Only) {
				addr = addrs[i];
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
InetDest dest { addr, 12444 };
logDebug("resolved added " + dest.toString());
		}
	}

	void ResolverImpl::resolve(std::shared_ptr<ResolverIf> client, const std::string& name, const Resolver::AddrPref& prefs, const NanoSecs& timeout, const InetDest& nameServer) {
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
			requests[request] = { prefs, {} };
			requests[request].prefs = prefs;
			if(prefs == Resolver::AddrPref::AnyAddr || prefs == Resolver::AddrPref::Ipv6Only) {
				UdpSocket::create(nameServer, std::make_shared<UdpResolver>(*this, request, name, timeout, Query::Qtype::Aaaa));
				requests[request].clients.push_back(client);
			}
			if(prefs == Resolver::AddrPref::AnyAddr || prefs == Resolver::AddrPref::Ipv4Only) {
				UdpSocket::create(nameServer, std::make_shared<UdpResolver>(*this, request, name, timeout, Query::Qtype::A));
				requests[request].clients.push_back(client);
			}
			request++;
		}
	}

	void
	ResolverImpl::cancel(const ResolverIf* /*client*/) {
		std::lock_guard<std::mutex> sync(lock);
//		auto it = byName.find(&name);
	}

	void
	ResolverImpl::destroy() {
		logDebug("ResolverImpl::destroy");
		std::lock_guard<std::mutex> sync(lock);
		requests.clear();
	}

	void
	ResolverImpl::handleTimer(const size_t /*timerId*/) {
//		client->timeout();
	}

	void
	ResolverImpl::requestComplete(const std::uint16_t reqNo, const Query::Qanswer& ans) {
		logDebug("requestComplete " + std::to_string(ans.reqNo));
		std::shared_ptr<ResolverIf> client;
		Resolver::AddrPref prefs;
		bool requestComplete = false;
		{
			std::lock_guard<std::mutex> sync(lock);
			auto it = requests.find(reqNo);
			if(it == requests.end()) {
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
					logDebug("adding new name" + ans.name);
					byName.insert(std::make_pair(ans.name, Names() ));
					bn = byName.find(ans.name);
				}
				bn->second.put(ans);
			}
			requestComplete = it->second.clients.size() == 0;
		}
		if(requestComplete) {
			logDebug("fully resolved " + std::to_string(ans.reqNo));
			auto bn = byName.find(ans.name);
			if(bn == byName.end()) {
				client->error();
			} else {
				IpAddr ipAddr;
				if(bn->second.get(prefs, ipAddr)) {
					client->resolved(ipAddr);
				} else {
					logDebug("name not found " + ans.name);
					client->error();
				}
			}
		}

	}
}
