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
                  explicit Event(std::shared_ptr<Runnable> const& owner, std::function<void()> const& func);
                  void operator()() const;
                  Runnable* owner() const;
                  bool operator==(Event const& rhs) const;
                  Event& operator=(Event const& rhs);
            private:
                  std::function<void()> func;
                  std::weak_ptr<Runnable> obj;
                  Runnable* runnable;
      };
}
