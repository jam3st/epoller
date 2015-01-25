#include "logger.hpp"
#include <botan/ocb.h>
#include <botan/hex.h>
//#include <botan/sha2_32.h>
#include <botan/aes.h>
#include <iostream>

#include <botan/auto_rng.h>
#include <botan/pubkey.h>
#include <botan/ecdh.h>
#include <botan/rsa.h>

namespace Sb {
    std::vector<Botan::byte>
    ocb_decrypt(const Botan::SymmetricKey& key,
                              const std::vector<Botan::byte>& nonce,
                              const Botan::byte ct[], size_t ct_len,
                              const Botan::byte ad[], size_t ad_len) {
        Botan::OCB_Decryption ocb(new Botan::AES_128);
        ocb.set_key(key);
        ocb.set_associated_data(ad, ad_len);
        ocb.start(&nonce[0], nonce.size());
        Botan::secure_vector<Botan::byte> buf(ct, ct+ct_len);
        ocb.finish(buf, 0);
        return unlock(buf);
    }

    std::vector<Botan::byte>
    ocb_encrypt(const Botan::SymmetricKey& key,
                              const std::vector<Botan::byte>& nonce,
                              const Botan::byte pt[], size_t pt_len,
                              const Botan::byte ad[], size_t ad_len) {
        Botan::OCB_Encryption ocb(new Botan::AES_128);
        ocb.set_key(key);
        ocb.set_associated_data(ad, ad_len);
        ocb.start(&nonce[0], nonce.size());
        Botan::secure_vector<Botan::byte> buf(pt, pt+pt_len);
        ocb.finish(buf, 0);
        std::vector<Botan::byte> pt2 = ocb_decrypt(key, nonce, &buf[0], buf.size(), ad, ad_len);
        if(pt_len != pt2.size() || !Botan::same_mem(pt, &pt2[0], pt_len)) {
            logError("OCB decrypt failed");
        }
        return unlock(buf);
    }

    template<typename Alloc, typename Alloc2>
    std::vector<Botan::byte> ocb_encrypt(const Botan::SymmetricKey& key,
                                            const std::vector<Botan::byte>& nonce,
                                            const std::vector<Botan::byte, Alloc>& pt,
                                            const std::vector<Botan::byte, Alloc2>& ad) {
        return ocb_encrypt(key, nonce, &pt[0], pt.size(), &ad[0], ad.size());
    }

    template<typename Alloc, typename Alloc2>
    std::vector<Botan::byte> ocb_decrypt(const Botan::SymmetricKey& key,
                                            const std::vector<Botan::byte>& nonce,
                                            const std::vector<Botan::byte, Alloc>& pt,
                                            const std::vector<Botan::byte, Alloc2>& ad) {
        return ocb_decrypt(key, nonce, &pt[0], pt.size(), &ad[0], ad.size());
    }

    std::vector<Botan::byte>
    ocb_encrypt(Botan::OCB_Encryption& ocb,
                              const std::vector<Botan::byte>& nonce,
                              const std::vector<Botan::byte>& pt,
                              const std::vector<Botan::byte>& ad) {
        ocb.set_associated_data(&ad[0], ad.size());
        ocb.start(&nonce[0], nonce.size());
        Botan::secure_vector<Botan::byte> buf(pt.begin(), pt.end());
        ocb.finish(buf, 0);
        return unlock(buf);
    }

    void test_ecdha(Botan::AutoSeeded_RNG& rng) {
         try {
            Botan::ECDH_PrivateKey a(rng, Botan::EC_Group("secp521r1"));
            Botan::ECDH_PrivateKey b(rng, Botan::EC_Group("secp521r1"));

            Botan::PK_Key_Agreement pka(a, "TLS-12-PRF(SHA-256)");
            Botan::PK_Key_Agreement pkb(b, "TLS-12-PRF(SHA-256)");
            Botan::SymmetricKey ka = pka.derive_key(16, b.public_value());
            Botan::SymmetricKey kb = pkb.derive_key(16, a.public_value());
            std::cout << "Secret key of " << kb.as_string() << std::endl;
            if(ka != kb) {
                std::cout << "key mismatch" << std::endl;
            }

        }
        catch(std::exception& e) {
            std::cout << "Error testing key derivation" << " - " << e.what() << std::endl;
        }
    }

    void test_rsa_enc(Botan::AutoSeeded_RNG& rng) {
        const size_t bits = 2048;
        try {
            Botan::RSA_PrivateKey key(rng, bits);
            Botan::PK_Encryptor_EME encryptor(key, "EME1(SHA-256)");
            std::vector<Botan::byte> data = { 'C', 'r', 'u', 'f', 't' };
            std::vector<Botan::byte> enc = encryptor.encrypt(data, rng);
            std::cout << "encrpyted: " << enc.size() << " " << Botan::hex_encode(enc) << std::endl;
            Botan::PK_Decryptor_EME decryptor(key, "EME1(SHA-256)");
            auto dec = decryptor.decrypt(enc);
            std::cout << "decrpyted: " << dec.size() << " " << dec.data() << std::endl;
            dec = decryptor.decrypt(enc);
            dec[2] = 0;
            std::cout << "decrpyted: " << dec.size() << " " << dec.data() << std::endl;

        } catch(std::exception& e) {
            std::string errorMessage(e.what());
            std::cout << errorMessage << std::endl;
        }
    }

    void test_rsa_sign(Botan::AutoSeeded_RNG& rng) {
        const size_t bits = 2048;
        try {
//            Botan::OID oid("1.3.132.0.35"); // secp521r1
//            Botan::EC_Group dom_pars(oid);
//            Botan::ECDH_PrivateKey a(rng, dom_pars);

            Botan::RSA_PrivateKey key(rng, bits);
            Botan::PK_Signer signer(key, "EMSA3(SHA-256)");
            std::vector<Botan::byte> data = { 'C', 'r', 'u', 'f', 't' };
            std::vector<Botan::byte> sign = signer.sign_message(data, rng);
            std::cout << "signed: " << sign.size() << " " << Botan::hex_encode(sign) << std::endl;
            Botan::PK_Verifier ver(key, "EMSA3(SHA-256)");
            std::cout << "Signature is: " << (ver.verify_message(data,sign) ? "OK" : "stuffed") << std::endl;

        } catch(std::exception& e) {
            std::string errorMessage(e.what());
            std::cout << errorMessage << std::endl;
        }

    }



    void test_ocb() {
        std::cout << std::endl;
        Botan::AutoSeeded_RNG rng;
        test_rsa_enc(rng);
        test_rsa_sign(rng);
        test_ecdha(rng);


        const std::vector<Botan::byte> empty;
        const std::vector<Botan::byte> crab({0x01});
                                                             // 01234567890123456789012345678901
        std::vector<Botan::byte> nonce(8);
        nonce = Botan::hex_decode("012345678901234567");
        Botan::SymmetricKey key("01234567890123456789012345678901");
        const std::vector<Botan::byte> plainText({ 0x01, 0x02});
        std::vector<Botan::byte> cipherText = ocb_encrypt(key, nonce, plainText, crab);
        std::cout << " ENC of " << Botan::hex_encode(cipherText) << std::endl;

        try {
            std::cout << " DEC of " << Botan::hex_encode(ocb_decrypt(key, nonce, cipherText, empty)) << std::endl;
        } catch(std::exception& e) {
            std::string errorMessage(e.what());
            std::cout << errorMessage << std::endl;
        }
   }
}
