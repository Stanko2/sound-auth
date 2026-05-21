#include "config.h"
#include "toml/toml.hpp"
#include "util.cpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <openssl/rand.h>
#include <ostream>
#include <regex>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

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

  if (seteuid(0) == -1) {
    std::cerr << "error reading credentials" << std::endl;
  }

  FILE* credFile = fopen(CREDENTIAL_FILE, "r");
  if(credFile) {
    std::cout << "credential file loaded" << std::endl;
    try {
      m_credentials = toml::parse_file(CREDENTIAL_FILE);
    } catch (const toml::parse_error &err) {
      std::cerr << "Error in credential file, deleting it" << std::endl;
      remove(CREDENTIAL_FILE);
    }

    fclose(credFile);
  }

  if (m_configFile.empty()) {
    std::cerr << "No config file found" << std::endl;
  }
  seteuid(getuid());
}

void AuthConfig::saveCredentials() {
  // if (geteuid() != 0) {
  //   std::cerr << "Setting credentials should be done as root" << std::endl;
  //   return;
  // }

  if (seteuid(0) == -1) {
    std::cerr << "error writing to credentials file" << std::endl;
    return;
  }

  if (mkdir(CREDENTIAL_FOLDER, 0700) == -1) {
    if (errno != EEXIST) {
      std::cerr << "Failed to create directory: " << CREDENTIAL_FOLDER
                << " (Error: " << strerror(errno) << ")" << std::endl;
      return;
    }
  }
  std::ofstream file(CREDENTIAL_FILE,
                     std::ios::out | std::ios::binary | std::ios::trunc);

  if (file.is_open()) {
    file << m_credentials;
    file.close();

    chmod(CREDENTIAL_FILE, S_IRUSR | S_IWUSR);
  } else {
    std::cerr << "Could not write config to " << CREDENTIAL_FILE << std::endl;
  }

  seteuid(getuid());
}

void updateAddressIfEmpty(const std::string &filePath,
                          const std::string &newAddress) {
  std::ifstream inFile(filePath);
  if (!inFile.is_open()) {
    std::cerr << "Failed to open file for reading: " << filePath << std::endl;
    return;
  }

  std::stringstream buffer;
  buffer << inFile.rdbuf();
  inFile.close();
  std::string fileContent = buffer.str();

  std::regex addressRegex(R"(^\s*address\s*=\s*(['"])(['"])\s*$)");
  std::smatch match;
  std::regex searchRegex(R"((^\s*address\s*=\s*)(['"])(['"])(\s*$))",
                         std::regex_constants::multiline);

  if (std::regex_search(fileContent, match, searchRegex)) {
    std::cout << "Empty address field found. Updating..." << std::endl;

    std::string replacement = "$1'" + newAddress + "'$4";
    std::string updatedContent =
        std::regex_replace(fileContent, searchRegex, replacement);
    std::ofstream outFile(filePath, std::ios::out | std::ios::trunc);
    if (outFile.is_open()) {
      outFile << updatedContent;
      outFile.close();
    } else {
      std::cerr << "Failed to open file for writing: " << filePath << std::endl;
    }
  }
}

const std::vector<uint8_t> AuthConfig::getAddress() {
  std::string address;
  lookupStr("address", address, "");
  if (address == "") {
    std::vector<uint8_t> b(2);
    RAND_bytes(b.data(), 2);
    address = vectorToHexString(b);
    m_config.insert_or_assign("address", address);
    updateAddressIfEmpty(m_configFile, address);
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

AuthConfig::~AuthConfig() {
  // if (m_config != NULL) {
  //     delete m_config;
  // }
}

void AuthConfig::lookupStr(const char *path, std::string &output,
                           const std::string &defaultValue) const {

  auto node = m_credentials.at_path(path);
  if (node.is_string()) {
    output = node.value_or(defaultValue);
    return;
  }
  node = m_config.at_path(path);

  output = node.value_or(defaultValue);
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
  if (!m_credentials[user].is_table()) {
    m_credentials.insert_or_assign(user, toml::table{});
  }
  m_credentials[user].as_table()->insert_or_assign("key", hexKey);
  saveCredentials();
}

void AuthConfig::setAddress(std::string user,
                            const std::vector<uint8_t> &address) {
  assert(address.size() == 2);
  std::string path = user + ".address";
  std::string hexAddr = vectorToHexString(address);

  if (!m_credentials[user].is_table()) {
    m_credentials.insert_or_assign(user, toml::table{});
  }

  m_credentials[user].as_table()->insert_or_assign("address", hexAddr);
  saveCredentials();
}

std::vector<uint8_t> AuthConfig::GetPhoneAddress(std::string user) const {
  std::string addr;
  std::string path = user + ".address";
  lookupStr(path.c_str(), addr, "");
  return hexStringToVector(addr);
}
