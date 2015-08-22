#ifndef INIFILE_H
#define INIFILE_H

#include <stdint.h>       // for uint64_t
#include <string>         // for string
#include <unordered_map>  // for unordered_map

namespace utils
{

class IniFile
{
public:
    bool readFile(std::string filename);


    std::string getString(const std::string group, const std::string name, std::string defaultVal);
    uint64_t getInt(const std::string group, const std::string name, uint64_t defaultVal);
    double getDouble(const std::string group, const std::string name, double defaultVal);
    int getBool(const std::string group, const std::string name, bool defaultVal);

private:
    bool hasInt(const std::string group, const std::string name);
    bool hasDouble(const std::string group, const std::string name);
    bool hasBool(const std::string group, const std::string name);
    bool hasString(const std::string group, const std::string name);

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_values;
};

} //namespace utils

#endif // INIFILE_H
