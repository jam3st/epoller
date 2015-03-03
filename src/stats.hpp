﻿#pragma once
#include "utils.hpp"
#include "clock.hpp"

namespace Sb {
	class Stats final {
	public:
		explicit Stats();
		void MarkIdleStart();
		void MarkIdleEnd();
		void Notify(int num);
		NanoSecs GetBusyTimeNs() const;
	private:
		TimePointNs creation;
		TimePointNs mark;
		NanoSecs idleNs;
		NanoSecs busyNs;
		int counter;
		bool active;
	};

}
