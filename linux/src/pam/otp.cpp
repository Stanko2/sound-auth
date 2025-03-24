#pragma once
#include "base64.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <unistd.h>
#include <vector>

#define CHALLENGE_SIZE 16
#define SECRET_KEY_SIZE 16
#define CONFIG_LOCATION "./config"

std::vector<unsigned char> get_challenge() {
    std::vector<unsigned char> ret(CHALLENGE_SIZE, 0);
    RAND_bytes(ret.data(), CHALLENGE_SIZE);

    return ret;
}

std::string getUsername () {
    char* name = (char*)malloc(10);
    if (getlogin_r(name, 10)){
        throw "Failed to get username";
    }
    std::string s(name);
    free(name);
    return s;
}

std::string getHostname () {
    char* name = (char*)malloc(10);
    if (gethostname(name, 10)){
        throw "Failed to get username";
    }
    std::string s(name);
    free(name);
    return s;
}

std::string getSecretKey() {
    std::fstream f;
    std::string key = "";
    f.open(CONFIG_LOCATION);
    if (!f.is_open()) {
        throw "File not opened";
    }
    std::string username = getUsername();
    while(!f.eof()) {
        std::string line;
        std::getline(f, line);
        if (line.compare(0, username.size(), username) == 0) {
            key = line.substr(username.size()+1);
            std::string key = boost::beast::detail::base64_decode(key);
            break;
        }
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
    }
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
