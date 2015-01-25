//#pragma once
//#include "tlscredentials.hpp"
//#include <botan/auto_rng.h>
//#include <botan/tls_client.h>
//#include "tcpstream.hpp"

//namespace Sb {
//    class TlsClientWrapper {
//        public:
//            TlsClientWrapper(TcpStream& stream);
//            std::function<ssize_t(const Bytes&)> getCb();
//        private:
//            Botan::AutoSeeded_RNG rng;
//            TlsCredentialsManager creds;
//            Botan::TLS::Policy policy;
//            Botan::TLS::Session_Manager_In_Memory sessionManager;
//            Botan::TLS::Client client;
//    };
//}
