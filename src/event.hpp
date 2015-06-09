#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <atomic>
#include "clock.hpp"

namespace Sb {
      class Event;

      class Runnable : public std::enable_shared_from_this<Runnable> {
      public:
            explicit Runnable();
            virtual ~Runnable();
      private:
            friend class Engine;
      };

      class Event final {
      public:
            Event();
            explicit Event(std::weak_ptr<Runnable> const& owner, std::function<void()> const& func);
            void operator()() const;
            bool operator==(Event const& rhs) = delete;
      public:
            std::weak_ptr<Runnable> obj;
            std::function<void()> func;
      };
}
