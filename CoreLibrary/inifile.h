#ifndef INIFILE_H
#define INIFILE_H

#include <unordered_map>

class IniFile
{
public:
    bool readFile(std::string filename);
    bool hasString(const std::string group, const std::string name);

    std::string getString(const std::string group, const std::string name);

    bool hasInt(const std::string group, const std::string name);

    int getInt(const std::string group, const std::string name);

    bool hasDouble(const std::string group, const std::string name);

    int getDouble(const std::string group, const std::string name);

private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_values;
};


#endif // INIFILE_H
