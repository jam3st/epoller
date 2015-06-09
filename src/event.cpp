#include "event.hpp"
#include "engine.hpp"

namespace Sb {
      Runnable::Runnable() {
      }

      Runnable::~Runnable() {
      }

      Event::Event() {
      }

      Event::Event(std::weak_ptr<Runnable> const& owner, std::function<void()> const& func) : obj(owner), func(func) {
      }

      void Event::operator()() const {
            auto const ref = obj.lock();
            if (ref) {
                  func();
            }
      }
}

