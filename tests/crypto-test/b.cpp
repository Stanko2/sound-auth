#include <cstddef>
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

    std::ofstream file("b_public_key.bin", std::ios::binary);
    file.write(reinterpret_cast<char*>(pub), len);

    std::vector<unsigned char> priv(len);
    EVP_PKEY_get_raw_private_key(key, priv.data(), &len);

    std::ofstream priv_file("b_private_key.bin", std::ios::binary);
    priv_file.write(reinterpret_cast<char*>(priv.data()), len);

    std::ifstream pub_file("a_public_key.bin", std::ios::binary);
    std::vector<unsigned char> pub_key(len);
    pub_file.read(reinterpret_cast<char*>(pub_key.data()), len);

    EVP_PKEY* a_pub_key = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr, pub_key.data(), len);

    EVP_PKEY_CTX* derive_ctx = EVP_PKEY_CTX_new(key, nullptr);
    EVP_PKEY_derive_init(derive_ctx);
    EVP_PKEY_derive_set_peer(derive_ctx, a_pub_key);

    size_t secret_len = 32;
    EVP_PKEY_derive(derive_ctx, nullptr, &secret_len);
    std::vector<unsigned char> secret(secret_len);

    EVP_PKEY_derive(derive_ctx, secret.data(), &secret_len);

    std::ofstream secret_file("b_secret.bin", std::ios::binary);
    secret_file.write(reinterpret_cast<char*>(secret.data()), secret_len);

    EVP_PKEY_free(key);
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_CTX_free(derive_ctx);

    ERR_free_strings();
    EVP_cleanup();

    return 0;
}
