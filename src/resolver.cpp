#include "resolver.hpp"
#include "resolverimpl.hpp"
#include "engine.hpp"

namespace Sb {
      Resolver::Resolver() {
      }

      Resolver::~Resolver() {
      }

      void Resolver::init() {
            if(!impl) {
                  impl = std::make_shared<ResolverImpl>();
            } else {
                  throw std::runtime_error("Already initialised - from a single threaded context");
            }
      }

      void Resolver::resolve(std::shared_ptr<ResolverIf>& client, std::string const& name, AddrPref const& prefs, NanoSecs const& timeout,
                             InetDest const& nameServer) {
            if(impl) {
                  impl->resolve(client, name, prefs, timeout, nameServer);
            }
      }

      void Resolver::cancel(ResolverIf const* client) {
            if(impl) {
                  impl->cancel(client);
            }
      }

      void Resolver::destroy() {
            if(impl) {
                  impl.reset();
            }
      }
}
