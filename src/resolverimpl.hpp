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
#include "query.hpp"



namespace Sb {
	class QueryHandlerIf {
		public:
			virtual void requestComplete(const std::uint16_t reqNo, const Query::Qanswer& ans) = 0;
	};

	class ResolverImpl : public TimeEvent, public QueryHandlerIf {
		public:
			ResolverImpl();
			~ResolverImpl();
			void resolve(std::shared_ptr<ResolverIf> client, const std::string& name, const Resolver::AddrPref& prefs, const NanoSecs& timeout, const InetDest& nameServer);
			void cancel(const ResolverIf* client);
			void destroy();
			void handleTimer(const size_t timerId) override;
			void requestComplete(const uint16_t reqNo, const Query::Qanswer& ans) override;

		private:
			class Names {
				public:
					bool get(const Resolver::AddrPref& prefs, IpAddr& addr) const;
					void put(const Query::Qanswer& ans);
					mutable size_t lastAccessIndex;
					std::vector<TimePointNs> expiry;
					std::vector<IpAddr> addrs;
			};

			class Request {
				public:
					Resolver::AddrPref prefs;
					std::vector<std::shared_ptr<ResolverIf>> clients;
			};

			std::unordered_map<std::uint16_t, Request> requests;
			std::unordered_map<std::string, Names> byName;
			std::map<TimePointNs, const std::string*> byExpiry;
			std::mutex lock;
			uint16_t request;
	};
}
