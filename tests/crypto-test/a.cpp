#include <fstream>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <iostream>
#include <vector>

int main() {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr);
    EVP_PKEY *key = nullptr;

    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_keygen(ctx, &key);

    unsigned char pub[32];
    size_t len = sizeof(pub);
    EVP_PKEY_get_raw_public_key(key, pub, &len);

    std::ofstream file("a_public_key.bin", std::ios::binary);
    file.write(reinterpret_cast<char*>(pub), len);

    std::vector<unsigned char> priv(len);
    EVP_PKEY_get_raw_private_key(key, priv.data(), &len);

    std::ofstream priv_file("a_private_key.bin", std::ios::binary);
    priv_file.write(reinterpret_cast<char*>(priv.data()), len);

    EVP_PKEY_free(key);
    EVP_PKEY_CTX_free(ctx);

    ERR_free_strings();
    EVP_cleanup();

    return 0;
}
