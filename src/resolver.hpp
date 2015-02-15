#pragma once
#include <string>

namespace Sb {
	class ResolverImpl;
	class Resolver final {
		public:
			enum struct AddrPref {
				AnyAddr,
				Ipv4Only,
				Ipv6Only
			};
			void resolve(const std::string&, const AddrPref& pres = AddrPref::AnyAddr);
			Resolver();
			~Resolver();
		private:
			ResolverImpl* impl;
	};
}
