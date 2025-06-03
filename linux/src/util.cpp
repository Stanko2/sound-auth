#include "util.h"

bool waitUntil(int timeout, std::function<bool()> condition) {
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(timeout)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (condition()) {
            return true;
        }
    }

    return false;
}

std::vector<uint8_t> vectorFromString(const std::string& str) {
    std::vector<uint8_t> vec;
    std::copy(str.begin(), str.end(), std::back_inserter(vec));
    return vec;
}

std::string stringFromVector(const std::vector<uint8_t>& vec) {
    std::string str;
    std::copy(vec.begin(), vec.end(), std::back_inserter(str));
    return str;
}


std::string vectorToHexString(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (uint8_t byte : data) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(byte);
    }
    return oss.str();
}
std::vector<uint8_t> hexStringToVector(const std::string& str) {
    std::vector<uint8_t> vec;
    for (size_t i = 0; i < str.length(); i += 2) {
        uint8_t byte = std::stoi(str.substr(i, 2), nullptr, 16);
        vec.push_back(byte);
    }
    return vec;
}

void printHex(const std::string& data) {
    std::cout << "Hex dump (" << data.size() << " bytes): ";
    for (unsigned char c : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(c) << " ";
    }
    std::cout << std::dec << std::endl;  // reset to decimal
}
