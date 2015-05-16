#include "resolver.hpp"
#include "resolverimpl.hpp"
#include "engine.hpp"

namespace Sb {
      Resolver::Resolver() {
      }

      Resolver::~Resolver() {
      }

      void
      Resolver::init() {
            auto ref = impl.lock();

            if(!ref) {
                  auto sp = std::make_shared<ResolverImpl>();
                  Engine::addTimer(sp);
                  impl = sp;
            } else {
                  throw std::runtime_error("Already initialised - from a single threaded context");
            }
      }

      void
      Resolver::resolve(std::shared_ptr<ResolverIf> client, const std::string& name, const AddrPref& prefs, const NanoSecs& timeout, const InetDest& nameServer) {
            auto ref = impl.lock();

            if(ref) {
                  ref->resolve(client, name, prefs, timeout, nameServer);
            }
      }

      void
      Resolver::cancel(const ResolverIf* client) {
            auto ref = impl.lock();

            if(ref) {
                  ref->cancel(client);
            }
      }

      void
      Resolver::destroy() {
            auto ref = impl.lock();

            if(ref) {
                  Engine::removeTimer(ref.get());
            }
      }
}
