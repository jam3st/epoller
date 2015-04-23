/*
This file is part of PoLLaX.

PoLLaX is free software: you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

PoLLaX is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with PoLLaX.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <chrono>
#include "clock.hpp"
#include "types.hpp"


namespace Sb {
	namespace Clock {
		static int64_t driftCorrectionInNs = std::chrono::duration_cast<NanoSecs>(
									  std::chrono::system_clock::now().time_since_epoch() -
									  std::chrono::steady_clock::now().time_since_epoch()
									  ).count();

		uint64_t
		now() {
			return std::chrono::duration_cast<NanoSecs>(SteadyClock::now().time_since_epoch()).count() + driftCorrectionInNs;

		}
	}
}
