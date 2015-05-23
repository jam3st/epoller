#include "event.hpp"
#include "engine.hpp"

namespace Sb {
      Runnable::Runnable() {
      }

      Runnable::~Runnable() {
      }

      Event::Event(std::shared_ptr<Runnable> const& owner, std::function<void()> const& func) : func(func), obj(owner), origOwner(owner.get()) {
      }

      Runnable* Event::owner() const {
            return origOwner;
      }
      void Event::operator()() const {
            auto const ref = obj.lock();
            if(ref) {
                  func();
            }
      }
}

