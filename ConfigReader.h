#ifndef CONFIG_READER
#define CONFIG_READER

#include <fstream>
#include <string>
#include <map>
using namespace std;

class ConfigReader {
    map<string, string> map;

    ConfigReader(const ConfigReader &) = delete;

    ConfigReader &operator=(const ConfigReader &) = delete;

    std::map<std::string, std::string> readConfigFile(const std::string &filename = "application.conf");

public:
    ConfigReader();

    static ConfigReader &getInstance() {
        static ConfigReader instance;
        return instance;
    }

    static string get(const string &key);
};


#endif //CONFIG_READER
