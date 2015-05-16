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
                  virtual void error() = 0;
                  virtual void resolved(IpAddr const& addr) = 0;
      };

      class ResolverImpl;

      class Resolver final {
            public:
                  friend class Engine;
                  enum struct AddrPref {
                        AnyAddr, Ipv4Only, Ipv6Only
                  };
                  void init();
                  void resolve(
                        std::shared_ptr<ResolverIf> client,
                        const std::string&,
                        const AddrPref& prefs = AddrPref::AnyAddr,
                        const NanoSecs& timeout = NanoSecs {
                                                        2'000'000'000 }, const InetDest& nameServer = Socket::destFromString("::1", 53) );
                                                        void cancel(const ResolverIf* client);
                                                        private:
                                                        void destroy();
                                                        Resolver();
                                                        ~Resolver();
                                                        private:
                                                        std::weak_ptr<ResolverImpl> impl;
                                                  };
                                                  }