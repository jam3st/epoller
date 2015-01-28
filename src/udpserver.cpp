#include "udpserver.hpp"

namespace Sb {
	void UdpServer::create(const uint16_t port, std::shared_ptr<UdpDispatchIf> dispatcher) {
		auto ref = std::make_shared<UdpServer>(port, dispatcher);
		Engine::add(ref);
	}

	UdpServer::UdpServer(const uint16_t port, std::shared_ptr<UdpDispatchIf> dispatcher)
		: Socket(fd),
		  dispatcher(dispatcher) {
		reuseAddress();
		bind(port);
	}

	UdpServer::~UdpServer() {
	}

	void UdpServer::handleRead() {
	}

	void UdpServer::handleWrite() {
	}

	void UdpServer::handleError() {
	}

	void UdpServer::handleTimer(const size_t) {
	}
}
