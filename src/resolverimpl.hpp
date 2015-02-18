#pragma once
#include "resolverimpl.hpp"
#include "types.hpp"
#include "clock.hpp"
#include <map>
#include <unordered_map>
#include <functional>
#include <string>

namespace Sb {
	class ResolverImpl
	{
		public:
			ResolverImpl();
			~ResolverImpl();
		private:
			struct Entry {
				TimePointNs expiry;
				uint8_t	addr[IP_ADDRESS_SIZE];
				const std::string name;
			};
			struct strPtrHash{
					std::size_t operator()(const std::string* const& str) const noexcept {
						std::hash<std::string> stringHash;
						return stringHash(*str);
					}
			};
			struct strPtrComp{
					bool operator()( const std::string* const& lhs, const std::string* const& rhs) const {
						return *lhs == *rhs;
					}
			};
			std::unordered_map<const std::string*, const Entry, strPtrHash, strPtrComp> byName;
			std::map<TimePointNs, const Entry> byExpiry;
	};
}
