#include "timeevent.hpp"
#include "engine.hpp"

namespace Sb {
      TimeEvent::TimeEvent() {
      }

      TimeEvent::~TimeEvent() {
      }

      Timer::Timer(std::function<void()> const& func) : func(func) {
      }

      void Timer::operator()() {
            func();
      }
}

