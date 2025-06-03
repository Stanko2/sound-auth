#pragma once

#include "ggwave/ggwave.h"
#include <cstdint>
#include <string>
#include <vector>
#include <libconfig.h++>

#define CONFIG_NAME "sound-auth.cfg"

static const std::string CONFIG_PATHS[] = {
    ".",
    "/etc",
    "/usr/local/share/sound-auth",
    "/usr/share/sound-auth",
};



using namespace libconfig;

class AuthConfig {
public:
    static AuthConfig& instance()
    {
        static AuthConfig INSTANCE;
        return INSTANCE;
    }
    AuthConfig();
    ~AuthConfig();
    ggwave_ProtocolId getProtocol() const;
    void saveConfig();
    int getPlaybackDeviceId() const;
    int getCaptureDeviceId() const;
    std::vector<uint8_t> getSecretKey(std::string user) const;
    void setSecretKey(std::string user, const std::vector<uint8_t>& key) const;
    void setAddress(std::string user, const std::vector<uint8_t>& address) const;
    void setSetting(const char* path, const std::string value) const;
    std::vector<uint8_t> GetPhoneAddress(std::string user) const;
    const std::vector<uint8_t> getAddress();
private:
    std::string m_configFile;
    Config* m_config;
    void lookupStr(const char* path, std::string& output, const std::string& defaultValue) const;
    int deviceNameToId(const std::string& name, const bool isCapture) const;
};
