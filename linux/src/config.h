#pragma once

#include "ggwave/ggwave.h"
#include <cstdint>
#include <string>
#include <vector>
#include <libconfig.h++>
#include "toml/toml.hpp"

#define CONFIG_NAME "sound-auth.cfg"
#define CREDENTIAL_FOLDER "/var/lib/sound-auth"
#define CREDENTIAL_FILE "/var/lib/sound-auth/credentials"

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
    void saveCredentials();
    std::string getPlaybackDeviceName() const;
    std::string getCaptureDeviceName() const;
    std::vector<uint8_t> getSecretKey(std::string user) const;
    void setSecretKey(std::string user, const std::vector<uint8_t>& key);
    void setAddress(std::string user, const std::vector<uint8_t>& address);
    std::vector<uint8_t> GetPhoneAddress(std::string user) const;
    const std::vector<uint8_t> getAddress();
    toml::table getConfig() {
      return m_config;
    }
private:
    std::string m_configFile;
    toml::table m_config;
    toml::table m_credentials;
    int deviceNameToId(const std::string& name, const bool isCapture) const;
    void lookupStr(const char* path, std::string& output, const std::string& defaultValue) const;
};
