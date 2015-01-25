#include <iostream>
#include <functional>

#include "engine.hpp"
#include "udpclient.hpp"
#include "tcplistener.hpp"
#include "udpserver.hpp"
#include "tcpconn.hpp"
#include "logger.hpp"

using namespace Sb;
namespace Sb {
	extern size_t test_ocb ();
}

void runUnit (const std::string id, std::function<void()>what) {
	try {
		Engine::init();
		what();
		Engine::start(0);
	} catch(const std::exception& e) {
		   std::cerr << e.what() << "\n";
	} catch ( ... ) {
		std::cerr <<  id << " " <<  "exception" << std::endl;
	}
}


class TcpEcho : public TcpSockIface {
	public:
		TcpEcho() {}
		virtual ~TcpEcho() {
			logDebug("~TcpEcho destroyed");
		}
		virtual void onConnect(TcpStream& s, const struct InetDest& ) {
			logDebug("TcpEcho onConnect");
			Bytes greeting = stringToBytes("Ello\n");
			s.queueWrite(greeting);
			s.disconnect();
		}

		virtual void onReadCompleted( TcpStream& s, const Bytes& w) {
			logDebug("TcpEcho onReadCompleted");
			s.queueWrite(w);
			s.disconnect();
		}

		virtual void onReadyToWrite( TcpStream&) {
			logDebug("TcpEcho onReadyToWrite");
		}

		virtual void onDisconnect( TcpStream& s) {
			logDebug("TcpEcho onDisconnect");
			s.disconnect();
		}

		virtual void onTimerExpired(TcpStream& s, int tid) {
			s.queueWrite( { 'x', 'p', '\n' });
			logDebug("blah expired " + intToString(tid));
		}
};

class TcpEchoWithGreeting : public TcpSockIface {
	public:
		TcpEchoWithGreeting() {}
		virtual ~TcpEchoWithGreeting() {
			logDebug("~TcpEchoWithGreeting destroyed");
		}
		virtual void onConnect(TcpStream& s, const struct InetDest& ) {
			logDebug("TcpEcho onConnect");
			Bytes greeting = stringToBytes("Ello\n");
			s.queueWrite(greeting);
			s.setTimer(0, NanoSecs(5000000000));
			s.setTimer(1, NanoSecs(1));
			s.setTimer(2, NanoSecs(4000000000));
			s.setTimer(3, NanoSecs(9000000000));
			s.setTimer(4, NanoSecs(5000000000));
			s.setTimer(5, NanoSecs(5000000000));
			s.setTimer(5, NanoSecs(5000000000));
		}

		virtual void onReadCompleted( TcpStream& s, const Bytes& x) {
			logDebug("TcpEcho onReadCompleted");
			s.queueWrite(x);
		}

		virtual void onReadyToWrite( TcpStream&) {
			logDebug("TcpEcho onReadyToWrite");
		}

		virtual void onDisconnect( TcpStream& s) {
			logDebug("TcpEcho onDisconnect");
			s.disconnect();
		}

		virtual void onTimerExpired(TcpStream& s, int tid) {
			s.queueWrite( { 'x', 'p', ' ', (unsigned char)('0' + tid), '\n' });
			logDebug("blah expired " + intToString(tid));

		}
};

int main (const int argc, const char* const argv[]) {
	if(argc > 1) {
		std::cerr << argv[0] <<  " invalid arguments.\n";
		return 1;
	}

	runUnit ("exp", [] () {
		TcpListener::create(1025, [] () { return std::make_shared<TcpEcho>(); } );
		TcpListener::create(1026, [] () { return std::make_shared<TcpEchoWithGreeting>(); } );


//		Engine::add(TcpConn::create("::1", 1025,
//					[] (TcpStream& stream, const Bytes& data) {
//			stream.doWrite(data);
//		},
//		[] (TcpStream& stream, const struct InetDest& from) {
//				(void)from;
//				stream.doWrite({ 'B', 'y', 'e', '\n'});
//				}));

	});
//		;(new UdpServer(1025))->start();
//		(new TcpConnector("127.0.0.1", 1025))->start ();
//        (new UdpServer(1025))->start();
//		(new TcpListener(1025	))->start ();
//		(new TcpConnector("127.0.0.1", 1025))->start ();
//		(new TcpConnector("::1", 1025))->start();
//		(new UdpServer(1025))->start ();
//		(new UdpClient("127.0.0.1", 1025, { 'T', 'E', 'S', 'T', '\n'}))->start ();
//		(new UdpClient("::1", 1025, { 'T', 'E', 'S', 'T', '\n'}))->start();
//		(new TcpConnector("2404:6800:4003:c02::93", 443))->start();
//        (new TcpConnector("::1", 4040))->start();

	std::cerr << argv[0] << " exited." << std::endl;

	return 0;
}


