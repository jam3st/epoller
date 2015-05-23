#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include "clock.hpp"

namespace Sb {
      class Event;

      class Runnable : public std::enable_shared_from_this<Runnable> {
            public:
                  explicit Runnable();
                  virtual ~Runnable() = 0;
            private:
                  friend class Engine;
      };

      class Event final {
            public:
                  Event() = delete;
                  explicit Event(std::shared_ptr<Runnable> const& owner, std::function<void()> const& func);
                  void operator()() const;
                  Runnable* owner() const;
            private:
                  std::function<void()> const func;
                  std::weak_ptr<Runnable> const obj;
                  Runnable* const origOwner;
      };
}
