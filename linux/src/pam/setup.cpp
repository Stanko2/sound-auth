#pragma once
#include "otp.cpp"
#include <cstdint>
#include <iostream>
#include <openssl/rand.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include "../audio/communication.h"
#include "../util.h"
#include "../config.h"
#include "./dh.cpp"

DHKey* setupKey;

int generateSecretCode() {
    std::vector<unsigned char> secret(SECRET_KEY_SIZE);
    RAND_bytes(secret.data(), SECRET_KEY_SIZE);
    std::string name = getUsername();
    AuthConfig::instance().setSecretKey(name, secret);
    return 0;
}

std::vector<uint8_t> getSetupMessage() {
    std::ostringstream ss;
    std::string name = getUsername();
    setupKey = generate_DH_key();
    const std::vector<uint8_t> address = AuthConfig::instance().getAddress();
    std::cout << "Addr Size: " << address.size() << std::endl;
    uint8_t setupBytes[1] = {0x01};
    ss.write(reinterpret_cast<const char*>(setupBytes), sizeof(setupBytes));
    ss << name << "@";
    std::string hostname = getHostname();
    ss << hostname << ":";
    std::cout << ss.str().size() << std::endl;
    // auto key = AuthConfig::instance().getSecretKey(name.c_str());
    // if (key.size() == 0){
    //     std::cout << "credentials for user " << getUsername() << " generated successfully" << std::endl;
    //     generateSecretCode();
    //     key = AuthConfig::instance().getSecretKey(name.c_str());
    // }

    ss.write(reinterpret_cast<const char*>(setupKey->public_key), DH_KEY_SIZE);
    printHex(ss.str());
    return vectorFromString(ss.str());
}

int runSetup(Communication* c) {

    std::vector<std::uint8_t> message;
    std::cout << "press [ENTER] when your phone is listening for the credentials" << std::endl;
    std::cin.get();
    std::vector<uint8_t> setup = getSetupMessage();
    std::cout << "Sending setup message..." << std::endl;
    std::cout << "Message: " << vectorToHexString(setup) << std::endl;

    bool success = false;
    c->send_broadcast(setup);

    c->recv(5 + DH_KEY_SIZE);

    std::vector<uint8_t> v;
    int ret = c->get_data(const_cast<std::vector<uint8_t>&>(v));
    std::cout << "Message: " << vectorToHexString(v) << std::endl;
    if (ret == 5 + DH_KEY_SIZE) {
        success = true;
        std::vector<uint8_t> a(2);
        a[0] = v[2]; a[1] = v[3];
        AuthConfig::instance().setAddress(getUsername(), a);
        std::cout << "Setup data was transferred into phone successfully" << std::endl;
        std::vector<uint8_t> secret = generate_shared_secret(setupKey, reinterpret_cast<uint8_t*>(v.data()) + 5);

        std::cout << "Got secret: " << vectorToHexString(secret) << std::endl;
        AuthConfig::instance().setSecretKey(getUsername(), secret);
        c->stop();
    } else {
        std::cerr << "Invalid message received: " << vectorToHexString(v) << std::endl;
    }




    return 0;
}
