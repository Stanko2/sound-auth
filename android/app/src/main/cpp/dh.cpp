// Diffie-Hellman key exchange implementation
//
#ifndef DH
#define DH
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <vector>
#define DH_KEY_SIZE 32

struct DHKey {
    unsigned char public_key[DH_KEY_SIZE];
    unsigned char private_key[DH_KEY_SIZE];
};


// Generate keys for DH
DHKey* generate_DH_key() {
    DHKey* key = new DHKey();
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        delete key;
        return nullptr;
    }

    EVP_PKEY* evp_key = NULL;



    if (EVP_PKEY_keygen_init(ctx) != 1){
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(ctx);
        delete key;
        return nullptr;
    }

    if (EVP_PKEY_keygen(ctx, &evp_key) != 1){
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(ctx);
        delete key;
        return nullptr;
    }

    size_t len = DH_KEY_SIZE;
    EVP_PKEY_get_raw_public_key(evp_key, key->public_key, &len);
    EVP_PKEY_get_raw_private_key(evp_key, key->private_key, &len);

    EVP_PKEY_free(evp_key);
    EVP_PKEY_CTX_free(ctx);

    return key;
}


// Get shared secret from my key and other side public key
std::vector<uint8_t> generate_shared_secret(const DHKey* my_key, uint8_t* other_pub_key) {
    std::vector<uint8_t> shared_secret(DH_KEY_SIZE);
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();


    EVP_PKEY* evp_my_key = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, my_key->private_key, DH_KEY_SIZE);
    EVP_PKEY* evp_other_key = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, NULL, other_pub_key, DH_KEY_SIZE);

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(evp_my_key, NULL);
    EVP_PKEY_derive_init(ctx);
    EVP_PKEY_derive_set_peer(ctx, evp_other_key);

    size_t secret_len;
    EVP_PKEY_derive(ctx, shared_secret.data(), &secret_len);

    EVP_PKEY_free(evp_my_key);
    EVP_PKEY_free(evp_other_key);
    EVP_PKEY_CTX_free(ctx);

    return shared_secret;
}

#endif