#pragma once
#include <botan/credentials_manager.h>
#include <botan/x509self.h>
#include <botan/rsa.h>
#include <botan/dsa.h>
#include <botan/srp6.h>
#include <botan/srp6_files.h>
#include <botan/ecdsa.h>
#include <iostream>
#include <fstream>
#include <memory>

namespace  Sb {
    inline bool constTimeValueExists(const std::vector<std::string>& vec,
                                     const std::string& val) {
        bool exists(false);
        std::for_each(vec.begin(), vec.end(),
                      [&] (std::string i) {
            if(i == val) {
                exists = true;
            }
        });
        return exists;
    }

    class TlsCredentialsManager : public Botan::Credentials_Manager   {
        public:
          TlsCredentialsManager(Botan::RandomNumberGenerator& rng);

          std::string srp_identifier(const std::string& type,
                                     const std::string& hostname);
          bool attempt_srp(const std::string& type,
                           const std::string& hostname);

          std::vector<Botan::Certificate_Store*>
          trusted_certificate_authorities(const std::string& type,
                                          const std::string& /*hostname*/);

          void verify_certificate_chain(
             const std::string& type,
             const std::string& purported_hostname,
             const std::vector<Botan::X509_Certificate>& cert_chain);
          std::string srp_password(const std::string& type,
                                   const std::string& hostname,
                                   const std::string& identifier);
          bool srp_verifier(const std::string& /*type*/,
                            const std::string& context,
                            const std::string& identifier,
                            std::string& group_id,
                            Botan::BigInt& verifier,
                            std::vector<Botan::byte>& salt,
                            bool generate_fake_on_unknown);
          std::string psk_identity_hint(const std::string&,
                                        const std::string&);

          std::string psk_identity(const std::string&, const std::string&,
                                   const std::string& identity_hint);

          Botan::SymmetricKey psk(const std::string& type, const std::string& context,
                                  const std::string& identity);

          std::pair<Botan::X509_Certificate,Botan::Private_Key*>
          load_or_make_cert(const std::string& hostname,
                            const std::string& key_type,
                            Botan::RandomNumberGenerator& rng);

          std::vector<Botan::X509_Certificate> cert_chain(
             const std::vector<std::string>& cert_key_types,
             const std::string& type,
             const std::string& context);
          Botan::Private_Key* private_key_for(const Botan::X509_Certificate& cert,
                                              const std::string& /*type*/,
                                              const std::string& /*context*/);
        private:
          Botan::RandomNumberGenerator& rng;
          Botan::SymmetricKey session_ticket_key;
          std::map<Botan::X509_Certificate, Botan::Private_Key*> certs_and_keys;
          std::vector<Botan::Certificate_Store*> m_certstores;
   };

}
