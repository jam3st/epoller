#include "event.hpp"
#include "engine.hpp"

namespace Sb {
      Runnable::Runnable() {
      }

      Runnable::~Runnable() {
      }

      Event::Event(std::function<void()> const& func) : func(func) {
      }

      void Event::operator()() {
            auto const ref = owner.lock();
            if(ref) {
                  func();
            }
      }
}

