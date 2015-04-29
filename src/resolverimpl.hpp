#pragma once
#include <functional>
#include <unordered_map>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "resolver.hpp"
#include "types.hpp"
#include "clock.hpp"
#include "timeevent.hpp"
#include "udpsocket.hpp"
#include "query.hpp"

namespace Sb {
	class UdpResolver;
	class QueryHandlerIf {
	public:
		virtual void requestComplete(std::uint16_t const reqNo,  Query::Qanswer const& ans) = 0;
		virtual void requestError(std::uint16_t const reqNo) = 0;
	};

	class ResolverImpl
		: public TimeEvent,
		  public QueryHandlerIf {
	public:
		ResolverImpl();
		~ResolverImpl();
		void resolve
			(std::shared_ptr<ResolverIf> client, const std::string& name, const Resolver::AddrPref& prefs, const NanoSecs& timeout, const InetDest& nameServer);
		void cancel(const ResolverIf*client);
		void destroy();
		void requestComplete(uint16_t const reqNo, Query::Qanswer const& ans) override;
		void requestError(uint16_t const reqNo) override;
		void queryTimedout(std::uint16_t);

	private:
		class Names {
		public:
			bool get(const Resolver::AddrPref& prefs, IpAddr& addr) const;
			void put(const Query::Qanswer& ans);
			mutable size_t lastAccessIndex;
			std::vector<TimePointNs> expiry;
			std::vector<IpAddr> addrs;
		};

		class ResolverQuery {
		public:
			Resolver::AddrPref prefs;
			std::vector<std::shared_ptr<ResolverIf>> clients;
			std::vector<std::weak_ptr<UdpResolver>> resolvers;
			std::unique_ptr<Timer> timeout;
		};

		std::unordered_map<std::uint16_t, ResolverQuery> resQueries;
		std::unordered_map<std::string, Names> byName;
		std::map<TimePointNs, std::string const*> byExpiry;
		std::mutex lock;
		uint16_t request;
	};
}