#include "config.h"
#include "ggwave/ggwave.h"
#include <SDL2/SDL_audio.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>
#include <libconfig.h++>
#include <openssl/rand.h>
#include <ostream>
#include <sstream>
#include <string>
#include "util.cpp"

AuthConfig::AuthConfig() {
    m_config = new Config();
    m_configFile = "";
    for (int i = 0; i < 4; i++) {
        std::string path = CONFIG_PATHS[i] + "/" + CONFIG_NAME;
        FILE* file = fopen(path.c_str(), "r");
        if (file) {
            m_configFile = path.c_str();
            m_config->read(file);
            fclose(file);
            break;
        }
    }

    if (m_configFile.empty()) {
        std::cerr << "No config file found" << std::endl;
        m_config = nullptr;
    }
}

void AuthConfig::saveConfig() {
    if (!m_configFile.empty()) {
        m_config->writeFile(m_configFile);
    }
}

const std::vector<uint8_t> AuthConfig::getAddress() {
    std::string address;
    lookupStr("address", address, "");
    if (address == "") {
        std::vector<uint8_t> b(2);
        RAND_bytes(b.data(), 2);
        address = vectorToHexString(b);
        setSetting("address", address);
    }

    assert(address.length() == 4);
    return hexStringToVector(address);
}

int AuthConfig::getPlaybackDeviceId() const {
    std::string device;
    lookupStr("devices.playback", device, "auto");
    if (device == "auto") {
        return -1;
    }
    return deviceNameToId(device, false);
}

int AuthConfig::deviceNameToId(const std::string& name, const bool isCapture) const {
    int numDevice = SDL_GetNumAudioDevices(isCapture);
    if (numDevice <= 0) {
        std::cerr << "Cannot find any audio devices" << SDL_GetError() << std::endl;
        return -1;
    }

    for (int i = 0; i < numDevice; ++i) {
        std::string deviceName = SDL_GetAudioDeviceName(i, isCapture);
        if (deviceName == name) {
            return i;
        }
    }
    std::cerr << "Cannot find audio device '" << name << "'" << std::endl;
    return -1;
}


int AuthConfig::getCaptureDeviceId() const {
    std::string device;
    lookupStr("devices.capture", device, "auto");
    if (device == "auto") {
        return -1;
    }
    return deviceNameToId(device, true);
}

ggwave_ProtocolId AuthConfig::getProtocol() const {
    std::string protocol;
    lookupStr("protocol", protocol, "ultrasound_fastest");

    if (protocol == "ultrasound_fastest") {
        return GGWAVE_PROTOCOL_ULTRASOUND_FASTEST;
    } else if (protocol == "ultrasound_fast") {
        return GGWAVE_PROTOCOL_ULTRASOUND_FAST;
    } else if (protocol == "ultrasound_normal") {
        return GGWAVE_PROTOCOL_ULTRASOUND_NORMAL;
    } else if (protocol == "audible_normal") {
        return GGWAVE_PROTOCOL_AUDIBLE_NORMAL;
    } else if (protocol == "audible_fast") {
        return GGWAVE_PROTOCOL_AUDIBLE_FAST;
    } else if (protocol == "audible_fastest") {
        return GGWAVE_PROTOCOL_AUDIBLE_FASTEST;
    }

    std::cerr << "Invalid protocol: " << protocol << std::endl;
    return GGWAVE_PROTOCOL_ULTRASOUND_FASTEST;
}

AuthConfig::~AuthConfig() {
    // if (m_config != NULL) {
    //     delete m_config;
    // }
}

void AuthConfig::lookupStr(const char* path, std::string& output, const std::string& defaultValue) const {
    if (!m_config) {
        output = defaultValue;
        return;
    }
    if (m_config->lookupValue(path, output)) {
        return;
    } else {
        output = defaultValue;
    }
}

void AuthConfig::setSetting(const char* path, const std::string value) const {
    if (!m_config) {
        return;
    }
    Setting& setting = m_config->getRoot();
    Setting* current = &setting;
    std::stringstream ss(path);
    std::vector<std::string> segments;
    std::string s;
    while(std::getline(ss, s, '.')) {
        segments.push_back(s);
    }
    for (int i = 0; i < segments.size(); ++i) {
        const std::string& segment = segments[i];
        if (i == segments.size() - 1) {
            if (current->exists(segment)) {
                (*current)[segment] = value;
            } else {
                current->add(segment, Setting::TypeString) = value;
            }
        }
        else {
            if (!current->exists(segment)) {
                current = &current->add(segment, Setting::TypeGroup);
            } else {
                current = &(*current)[segment];
            }
        }
    }

    m_config->writeFile(m_configFile);
}

std::vector<uint8_t> AuthConfig::getSecretKey(std::string user) const {
    std::string key;
    std::string path = user + ".key";
    lookupStr(path.c_str(), key, "");
    return hexStringToVector(key);
}

void AuthConfig::setSecretKey(std::string user, const std::vector<uint8_t>& key) const {
    std::string path = user + ".key";
    std::string hexKey = vectorToHexString(key);
    setSetting(path.c_str(), hexKey);
}

void AuthConfig::setAddress(std::string user, const std::vector<uint8_t>& address) const {
    assert(address.size() == 2);
    std::string path = user + ".address";
    std::string hexAddr = vectorToHexString(address);
    setSetting(path.c_str(), hexAddr);
}

std::vector<uint8_t> AuthConfig::GetPhoneAddress(std::string user) const {
    std::string addr;
    std::string path = user + ".address";
    lookupStr(path.c_str(), addr, "");
    return hexStringToVector(addr);
}
