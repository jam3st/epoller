#pragma once
#include <cstdint>
#include <memory>
#include "utils.hpp"

namespace Sb {
	class TimeEvent : public std::enable_shared_from_this<TimeEvent> {
		public:
			explicit TimeEvent();
			virtual ~TimeEvent();
			virtual void setTimer(const int timerId, const NanoSecs& timeout) final;
			virtual void cancelTimer(const int timerId) final;
		protected:
			virtual std::shared_ptr<TimeEvent> ref() final;
		private:
			friend class Engine;
			virtual void handleTimer(const size_t timerId) = 0;
	};
}
