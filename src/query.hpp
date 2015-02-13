#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "types.hpp"
#include "clock.hpp"

namespace Sb {
	namespace Query {
		class Qanswer {
			public:
				std::string name;
				std::vector<InetDest> dest;
				NanoSecs ttl;
				TimePointNs timeStamp;
		};

		std::vector<uint8_t> resolve(const std::string& name);
		Qanswer decode(const std::vector<uint8_t>& data);
	};
}
