#include "resolver.hpp"
#include "resolverimpl.hpp"

namespace Sb {
	Resolver::Resolver() {
		impl = new ResolverImpl;
	}

	Resolver::~Resolver() {
		delete impl;
	}
}
