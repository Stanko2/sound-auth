#include "config.h"
#include "ggwave/ggwave.h"
#include "toml/toml.hpp"
#include "util.cpp"
#include <SDL2/SDL_audio.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <openssl/rand.h>
#include <ostream>
#include <string>
#include <vector>
#include <regex>

AuthConfig::AuthConfig() {
  m_configFile = "";
  for (int i = 0; i < 4; i++) {
    std::string path = CONFIG_PATHS[i] + "/" + CONFIG_NAME;
    FILE *file = fopen(path.c_str(), "r");
    try {
      if (file) {
        m_configFile = path.c_str();
        m_config = toml::parse_file(m_configFile);
        fclose(file);
        break;
      }

    } catch (const toml::parse_error &err) {
      std::cerr << "Config parse error: " << err << std::endl;
      m_configFile = "";
      fclose(file);
    }
  }

  if (m_configFile.empty()) {
    std::cerr << "No config file found" << std::endl;
  }
}

void AuthConfig::saveConfig() {
  std::ofstream file(m_configFile,
                     std::ios::out | std::ios::binary | std::ios::trunc);

  if (file.is_open()) {
    file << m_config;
    file.close();
  } else {
    std::cerr << "Could not write config: " << std::endl;
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

std::string AuthConfig::getPlaybackDeviceName() const {
  std::string device;
  lookupStr("devices.playback", device, "auto");
  return device;
}

std::string AuthConfig::getCaptureDeviceName() const {
  std::string device;
  lookupStr("devices.capture", device, "auto");
  return device;
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

void AuthConfig::lookupStr(const char *path, std::string &output,
                           const std::string &defaultValue) const {
  auto node = m_config.at_path(path);

  output = node.value_or(defaultValue);
}

void AuthConfig::setSetting(const char *path, const std::string &value) {
  std::ifstream inFile(m_configFile);
  if (!inFile.is_open())
    return;

  std::stringstream buffer;
  buffer << inFile.rdbuf();
  std::string content = buffer.str();
  inFile.close();

  // Split path
  std::string pathStr(path);
  std::vector<std::string> parts;

  size_t start = 0;
  size_t end;

  while ((end = pathStr.find('.', start)) != std::string::npos) {
    parts.push_back(pathStr.substr(start, end - start));
    start = end + 1;
  }

  parts.push_back(pathStr.substr(start));

  // Build section name
  std::string section;
  for (size_t i = 0; i + 1 < parts.size(); ++i) {
    if (!section.empty())
      section += ".";

    section += parts[i];
  }

  const std::string key = parts.back();

  std::stringstream input(content);
  std::stringstream output;

  std::string line;

  bool inTargetSection = section.empty();
  bool sectionFound = section.empty();
  bool keyUpdated = false;

  std::regex sectionRegex(R"(^\s*\[(.+)\]\s*$)");
  std::regex keyRegex("^\\s*" + key + R"(\\s*=\\s*.*$)");

  while (std::getline(input, line)) {
    std::smatch match;

    // Detect section
    if (std::regex_match(line, match, sectionRegex)) {
      std::string currentSection = match[1];

      // Leaving target section without finding key
      if (inTargetSection && !keyUpdated) {
        output << key << " = \"" << value << "\"\n";
        keyUpdated = true;
      }

      inTargetSection = (currentSection == section);

      if (inTargetSection)
        sectionFound = true;
    }

    // Replace key inside target section
    if (inTargetSection && std::regex_search(line, keyRegex)) {
      // Preserve inline comment
      std::string comment;

      auto commentPos = line.find('#');
      if (commentPos != std::string::npos)
        comment = " " + line.substr(commentPos);

      output << key << " = \"" << value << "\"" << comment << "\n";

      keyUpdated = true;
    } else {
      output << line << "\n";
    }
  }

  // Append key if section exists but key missing
  if (sectionFound && !keyUpdated) {
    output << key << " = \"" << value << "\"\n";
  }

  // Append section + key if section missing
  if (!sectionFound) {
    output << "\n[" << section << "]\n";
    output << key << " = \"" << value << "\"\n";
  }

  std::ofstream outFile(m_configFile, std::ios::out | std::ios::trunc);

  if (outFile.is_open()) {
    outFile << output.str();
  }
}

std::vector<uint8_t> AuthConfig::getSecretKey(std::string user) const {
  std::string key;
  std::string path = user + ".key";
  lookupStr(path.c_str(), key, "");
  return hexStringToVector(key);
}

void AuthConfig::setSecretKey(std::string user,
                              const std::vector<uint8_t> &key) {
  std::string path = user + ".key";
  std::string hexKey = vectorToHexString(key);
  setSetting(path.c_str(), hexKey);
}

void AuthConfig::setAddress(std::string user,
                            const std::vector<uint8_t> &address) {
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
