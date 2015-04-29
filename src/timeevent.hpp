#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include "clock.hpp"

namespace Sb {
	class Timer;

	class TimeEvent : public std::enable_shared_from_this<TimeEvent> {
	public:
		explicit TimeEvent();
		virtual ~TimeEvent() = 0;
		virtual void setTimer(Timer* const timerId, NanoSecs const& timeout) final;
		virtual void cancelTimer(Timer* const timerId) final;
	protected:
		virtual std::shared_ptr<TimeEvent> ref() final;
	private:
		friend class Engine;
	};

	class Timer final {
	public:
		Timer() = delete;
		explicit Timer(std::function<void()> const& func);
		void operator()();
	private:
		std::function<void()> const func;
	};
}
