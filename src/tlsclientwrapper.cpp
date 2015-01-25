//#include "tlsclientwrapper.hpp"
//#include "tcpstream.hpp"

//namespace Sb {
//    TlsClientWrapper::TlsClientWrapper(TcpStream& stream) :
//        creds(rng),
//        sessionManager(rng),
//        client([&](const Botan::byte data[], size_t len) { stream.queueWrite(Bytes(data, data + len)); },
//               [&](const Botan::byte data[], size_t len) { stream.onRead(Bytes(data, data + len)); },
//               [] (Botan::TLS::Alert alert, const Botan::byte[], size_t) {
//                    logDebug("Alert " + intToString(alert.type()));
//               },
//               [] (const Botan::TLS::Session& session)  -> bool  { logDebug("handshake compete"); (void)session; return false; },
//               sessionManager,
//               creds,
//               policy,
//               rng) {
//    }

//    std::function<ssize_t(const Bytes&)> TlsClientWrapper::getCb(){
//        return [&] (const Bytes& data) {
//            logDebug("Running callback TlsClientWrapper::getCb with " + intToString(data.size()));
//            return client.received_data(&data[0], data.size());
//        };
//    }
//}
