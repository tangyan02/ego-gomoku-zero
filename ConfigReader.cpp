#include "ConfigReader.h"

#include <iostream>

ConfigReader::ConfigReader() {
    this->map = readConfigFile();
}

string ConfigReader::get(const string &key) {
    if (getInstance().map.find(key) == getInstance().map.end()) {
        cout << "Key " << key << " not found" << endl;
    }
    return getInstance().map.at(key);
}

std::map<std::string, std::string> ConfigReader::readConfigFile(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + filename);
    }

    std::map<std::string, std::string> config;
    std::string line;

    while (std::getline(file, line)) {
        // Trim leading whitespace
        line.erase(0, line.find_first_not_of(" \t"));

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            // Trim whitespace from key and value
            auto trim = [](std::string &s) {
                s.erase(0, s.find_first_not_of(" \t"));
                s.erase(s.find_last_not_of(" \t") + 1);
            };

            trim(key);
            trim(value);

            if (!key.empty()) {
                config[key] = value;
            }
        }
    }

    return config;
}
