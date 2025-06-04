#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <unistd.h>
#include <vector>
#include "../config.h"
#include "../audio/communication.h"
#include "../util.h"

#define CHALLENGE_SIZE 16
#define SECRET_KEY_SIZE 16
#define CONFIG_LOCATION "./config"

std::vector<uint8_t> get_challenge() {
    std::vector<uint8_t> ret(CHALLENGE_SIZE+1, 0);
    ret[0] = 0x02;
    RAND_bytes(ret.data()+1, CHALLENGE_SIZE);

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
    char* name = (char*)malloc(20);
    if (gethostname(name, 20)){
        throw "Failed to get username";
    }
    std::string s(name);
    free(name);
    return s;
}

std::vector<unsigned char> generate(std::vector<unsigned char>& challenge) {
    std::vector<unsigned char> message(SECRET_KEY_SIZE + CHALLENGE_SIZE, 0);
    auto key = AuthConfig::instance().getSecretKey(getUsername().c_str());
    // std::cout << "Key  : " << vectorToHexString(key) << std::endl;
    // std::cout << "Chall: " << vectorToHexString(challenge) << std::endl;
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
    // std::cout << "Hash: " << vectorToHexString(r) << std::endl;
    // std::cout << "Pass: " << vectorToHexString(password) << std::endl;

    for (size_t i = 0; i < r.size(); i++) {
        if (r[i] != password[i + 5]) {
            return false;
        }
    }
    return true;
}
bool runAuth(Communication* c) {
    AuthConfig cfg = AuthConfig::instance();
    std::vector<uint8_t> challenge = get_challenge();
    std::vector<uint8_t> message;
    bool running = true;
    std::vector<uint8_t> dest = cfg.GetPhoneAddress(getUsername());
    if (dest.data() == NULL) {
        std::cout << "You need to run 'sound-auth setup' first to generate key and transfer it to phone" << std::endl;
        return false;
    }

    c->receive_callback = [&challenge, c, &running](){
        std::vector<uint8_t> data;
        int ret = c->get_data(const_cast<std::vector<uint8_t>&>(data));
        challenge.erase(challenge.begin());
        bool success = verify(data, challenge);

        if (success) {
            std::cout << "Authentication successful" << std::endl;
            running = false;
        } else {
            std::cout << "Authentication failed" << std::endl;
        }
        c->stop();
    };
    c->send_message(challenge, dest.data());

    return !running;
    // return waitUntil(3000, [&running](){
    //     return running;
    // });
}
