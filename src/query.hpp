#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "types.hpp"
#include "clock.hpp"

namespace Sb {
	namespace Query  {
		enum struct Qtype : uint16_t {
			A = 1,
			Ns = 2,
			Cname = 5,
			Soa = 6,
			Ptr = 12,
			Mx = 15,
			Txt = 16,
			Aaaa = 28,
			Srv = 33,
			Ds = 43,
			Rrsig  = 46,
			Dnskey = 48,
			Nsec3 =	50,
			Nsec3Param = 51,
			Tlsa = 52,
			Any = 255
		};

		struct Qanswer {
			public:
				bool valid;
				std::uint16_t reqNo;
				std::string name;
				std::vector<IpAddr> addr;
				NanoSecs ttl;
				TimePointNs timeStamp;
		};

		std::vector<uint8_t> resolve(uint16_t reqNo, const std::string& name, Qtype qType);
		Qanswer decode(const std::vector<uint8_t>& data);
	};
}
