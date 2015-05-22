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
                  explicit Event(std::function<void()> const& func);
                  void operator()();
            private:
                  std::function<void()> const func;
                  std::weak_ptr<Runnable> owner;
      };
}
