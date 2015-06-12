#pragma once
#include <string>
#include <memory>
#include <mutex>
#include "clock.hpp"
#include "types.hpp"
#include "socket.hpp"

namespace Sb {
      class ResolverIf {
      public:
            virtual void notResolved() = 0;
            virtual void resolved(IpAddr const&addr) = 0;
      };

      class ResolverImpl;

      class Resolver final {
      public:
            friend class Engine;

            enum struct AddrPref {
                  AnyAddr, Ipv4Only, Ipv6Only
            };
            void init();
            void resolve(std::shared_ptr<ResolverIf> const& client, std::string const&, AddrPref const&prefs = AddrPref::AnyAddr,
            NanoSecs const& timeout = NanoSecs{2000000000}, InetDest const& nameServer = Socket::destFromString("::1", 53));
            void cancel(const ResolverIf* client);
      private:
            void destroy();
            Resolver();
            ~Resolver();
      private:
            std::shared_ptr<ResolverImpl> impl;
      };
}