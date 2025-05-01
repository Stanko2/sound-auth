#pragma once
#include "otp.cpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <openssl/rand.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include "../audio/communication.h"
#include "base64.hpp"
#include "../util.cpp"


int generateSecretCode() {
    std::string key = getSecretKey();
    if (key != "") {
        return 0;
    }
    std::ofstream output(CONFIG_LOCATION, std::fstream::out);
    if (!output.is_open()) {
        std::cerr << "Error writing secret code to file" << std::endl;
        return -1;
    }

    std::vector<unsigned char> secret(SECRET_KEY_SIZE);
    RAND_bytes(secret.data(), SECRET_KEY_SIZE);
    std::string encoded = boost::beast::detail::base64_encode(std::string(secret.begin(), secret.end()));
    std::string name = getUsername();
    output << name << ":" << encoded << std::endl;
    output.close();
    return 0;
}

std::vector<uint8_t> getSetupMessage() {
    std::stringstream ss;
    std::string name = getUsername();
    ss.write(new char[3] {1, 35, 43}, 3);
    ss << name << "@";
    std::string hostname = getHostname();
    ss << hostname << ":";
    ss << getSecretKey();
    return vectorFromString(ss.str());
}

int runSetup(Communication* c) {

    std::vector<std::uint8_t> message;
    if (generateSecretCode()) {
        std::cerr << "Error generating new secret code" << std::endl;
        return -1;
    }
    std::cout << "credentials for user " << getUsername() << " generated successfully" << std::endl;
    std::cout << "press [ENTER] when your phone is listening for the credentials" << std::endl;
    std::cin.get();
    std::vector<uint8_t> setup = getSetupMessage();
    std::cout << "Sending setup message..." << std::endl;
    std::cout << "Message size: " << setup.size() << std::endl;

    bool success = false;
    c->receive_callback = [&success]() {
        success = true;
        std::cout << "Setup data was transferred into phone successfully" << std::endl;
    };
    c->send_broadcast(setup);

    bool done = waitUntil(3000, [&success]() {
        return success;
    });

    if (!done) {
        std::cerr << "Timeout waiting for setup data to be transferred into phone" << std::endl;
        return -1;
    }

    return 0;
}
