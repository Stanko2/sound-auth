#include <chrono>
#include <thread>
#include <functional>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

bool waitUntil(int timeout, std::function<bool()> condition);

std::vector<uint8_t> vectorFromString(const std::string& str);

std::string stringFromVector(const std::vector<uint8_t>& vec);

std::string vectorToHexString(const std::vector<uint8_t>& vec);

std::vector<uint8_t> hexStringToVector(const std::string& str);

void printHex(const std::string& data);
