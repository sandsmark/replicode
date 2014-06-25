#include "inifile.h"

#include <fstream>
#include <algorithm>
#include <iostream>

static inline std::string trim(std::string string)
{
    string.erase(string.begin(), std::find_if(string.begin(), string.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    string.erase(std::find_if(string.rbegin(), string.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), string.end());
    return string;
}

bool IniFile::readFile(std::string filename)
{
    std::ifstream file;
    file.open(filename);

    if (!file.is_open()) {
        std::cout << "unable to open file\n";
        return 1;
    }

    std::string line;
    std::string group;
    size_t linenum = 0;
    while (std::getline(file, line)) {
        linenum++;
        if (line.length() == 0) continue;

        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end == std::string::npos) {
                std::cout << "invalid file, unclosed group at line " << linenum << std::endl;
                return false;
            }
            group = trim(line.substr(1, end - 1));
            continue;
        }

        size_t nameEnd = line.find('=');
        if (nameEnd == std::string::npos) {
            std::cerr << "invalid file, missing = at line " << linenum << std::endl;
            return false;
        }
        std::string name = trim(line.substr(0, nameEnd));
        std::string value = trim(line.substr(nameEnd + 1));
        m_values[group][name] = value;
    }

    return true;
}

bool IniFile::hasString(const std::string group, const std::string name)
{
    if (m_values.find(group) == m_values.end()) return false;
    return (m_values[group].find(name) != m_values[group].end());
}

std::string IniFile::getString(const std::string group, const std::string name)
{
    return m_values[group][name];
}

bool IniFile::hasInt(const std::string group, const std::string name)
{
    if (!hasString(group, name)) return false;
    size_t pos;
    try {
        std::stoi(getString(group, name), &pos);
    } catch (std::invalid_argument&) {
        return false;
    }
    return (pos > 0);
}

int IniFile::getInt(const std::string group, const std::string name)
{
    if (!hasInt(group, name)) return 0;
    return std::stoi(getString(group, name));
}

bool IniFile::hasDouble(const std::string group, const std::string name)
{
    if (!hasDouble(group, name)) return false;
    size_t pos;
    try {
        std::stod(getString(group, name), &pos);
    } catch (std::invalid_argument&) {
        return false;
    }
    return (pos > 0);
}

int IniFile::getDouble(const std::string group, const std::string name)
{
    if (!hasDouble(group, name)) return 0;
    return std::stod(getString(group, name));
}
