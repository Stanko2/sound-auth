#pragma once
#include "otp.cpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <openssl/rand.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include "base64.hpp"


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

std::string getSetupMessage() {
    std::stringstream ss;
    std::string name = getUsername();
    ss << name << "@";
    // std::string hostname = getHostname();
    // ss << hostname << ":";
    ss << getSecretKey();
    return ss.str();
}
