#include "event.hpp"
#include "engine.hpp"

namespace Sb {
      Runnable::Runnable() {
      }

      Runnable::~Runnable() {
      }

      Event::Event(std::shared_ptr<Runnable> const &owner, std::function<void()> const &func) : func(func), obj(owner), runnable(owner.get()) {
      }

      Runnable *Event::owner() const {
            return runnable;
      }

      Event& Event::operator=(Event const &rhs) {
            func = rhs.func;
            runnable = rhs.runnable;
            obj = rhs.obj;
            return *this;
      }

      bool Event::operator==(Event const &rhs) const {
            return runnable == rhs.runnable;
      }

      void Event::operator()() const {
            auto const ref = obj.lock();
            if(ref) {
                  func();
            }
      }
}

