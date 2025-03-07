#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <vector>

#define CHALLENGE_SIZE 16
#define SECRET_KEY_SIZE 8

std::vector<unsigned char> get_challenge() {
    std::vector<unsigned char> ret(CHALLENGE_SIZE, 0);
    RAND_bytes(ret.data(), CHALLENGE_SIZE);

    return ret;
}



std::vector<unsigned char> getSecretKey() {
    std::fstream f;
    std::vector<unsigned char> key(SECRET_KEY_SIZE, 0);
    f.open("secret");
    for (size_t i = 0; i < SECRET_KEY_SIZE; i++) {
        f >> key[i];
    }
    f.close();
    return key;
}

std::vector<unsigned char> generate(std::vector<unsigned char>& challenge) {
    std::vector<unsigned char> message(SECRET_KEY_SIZE + CHALLENGE_SIZE, 0);
    auto key = getSecretKey();
    for (size_t i = 0; i < CHALLENGE_SIZE; i++) {
        message[i] = challenge[i];
    }
    for (size_t i = 0; i < SECRET_KEY_SIZE; i++) {
        message[i + CHALLENGE_SIZE] = key[i];
    }

    EVP_MD_CTX* ctx;

    if ((ctx = EVP_MD_CTX_new()) == NULL) {
        throw "Error creating MD_CTX";
    }


    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) {
        EVP_MD_CTX_free(ctx);
        throw "Error EVP_DigestInit";
    }
    EVP_DigestUpdate(ctx, message.data(), message.size());

    unsigned char *digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size(EVP_sha256()));
    if (digest == NULL) {
        EVP_MD_CTX_free(ctx);
        throw "Error allocating memory for digest";
    }

    unsigned int digest_len;
    if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw "Error computing digest";
    }

    EVP_MD_CTX_free(ctx);
    std::vector<unsigned char> d(digest_len);

    for (size_t i = 0; i < digest_len; i++) {
        d[i] = digest[i];
        // std::cout << (int)d[i] << ' ';
    }
    // std::cout << std::endl;
    return d;
}

bool verify(std::vector<uint8_t> password, std::vector<uint8_t> challenge) {
    std::vector<unsigned char> r = generate(challenge);
    for (size_t i = 0; i < r.size(); i++) {
        if (r[i] != password[i]) {
            return false;
        }
    }
    return true;
}
