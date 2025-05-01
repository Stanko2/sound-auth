#include <chrono>
#include <thread>
#include <functional>
#include <string>

int waitUntil(int timeout, std::function<bool()> condition) {
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(timeout)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (condition()) {
            return 1;
        }
    }

    return 0;
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
