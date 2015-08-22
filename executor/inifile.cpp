#include "inifile.h"

#include <ctype.h>     // for isspace
#include <stddef.h>    // for size_t
#include <wordexp.h>   // for wordexp, wordexp_t
#include <algorithm>   // for find_if
#include <fstream>     // for ifstream, basic_istream
#include <functional>  // for unary_negate, pointer_to_unary_function, not1, etc
#include <stdexcept>   // for invalid_argument
#include <string>      // for basic_string, string, operator==, stod, stoi, etc

#include "CoreLibrary/debug.h"     // for DebugStream, debug

static inline std::string trim(std::string string)
{
    string.erase(string.begin(), std::find_if(string.begin(), string.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    string.erase(std::find_if(string.rbegin(), string.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), string.end());
    return string;
}

static inline std::string string_replace(std::string haystack, std::string needle, std::string new_value)
{
    for (size_t index = 0;; index += new_value.length()) {
        index = haystack.find(needle, index);

        if (index == std::string::npos) {
            break;
        }

        haystack.erase(index, needle.length());
        haystack.insert(index, new_value);
    }

    return haystack;
}

namespace utils
{

bool IniFile::readFile(std::string filename)
{
    std::ifstream file;
    wordexp_t exp_res;
    wordexp(filename.c_str(), &exp_res, 0);
    file.open(exp_res.we_wordv[0]);

    if (!file.is_open()) {
        debug("inifile") << "unable to open file" << filename;
        return 1;
    }

    std::string line;
    std::string group;
    size_t linenum = 0;

    while (std::getline(file, line)) {
        linenum++;
        size_t commentStart = line.find("//");

        if (commentStart != std::string::npos) {
            line = trim(line.substr(0, commentStart));
        }

        if (line.length() == 0 || line.front() == '#') {
            continue;
        }

        if (line[0] == '[') {
            size_t end = line.find(']');

            if (end == std::string::npos) {
                debug("inifile") << "invalid file, unclosed group at line " << linenum;
                return false;
            }

            group = trim(line.substr(1, end - 1));
            continue;
        }

        size_t nameEnd = line.find('=');

        if (nameEnd == std::string::npos) {
            debug("inifile") << "invalid file, missing = at line " << linenum;
            return false;
        }

        std::string name = trim(string_replace(line.substr(0, nameEnd), "%20", " "));
        std::string value = trim(line.substr(nameEnd + 1));

        if (name == "" || value == "") {
            continue;
        }

        m_values[group][name] = value;
    }

    return true;
}

std::string IniFile::getString(const std::string group, const std::string name, std::string defaultVal)
{
    if (!hasString(group, name)) {
        return defaultVal;
    }

    return m_values[group][name];
}

uint64_t IniFile::getInt(const std::string group, const std::string name, uint64_t defaultVal)
{
    if (!hasInt(group, name)) {
        return defaultVal;
    }

    return std::stoi(m_values[group][name]);
}

double IniFile::getDouble(const std::string group, const std::string name, double defaultVal)
{
    if (!hasDouble(group, name)) {
        return defaultVal;
    }

    return std::stod(m_values[group][name]);
}

int IniFile::getBool(const std::string group, const std::string name, bool defaultVal)
{
    if (!hasBool(group, name)) {
        return defaultVal;
    }

    const std::string val = m_values[group][name];
    return val == "true" || val == "yes";
}

bool IniFile::hasString(const std::string group, const std::string name)
{
    if (m_values.find(group) == m_values.end()) {
        return false;
    }

    return (m_values[group].find(name) != m_values[group].end());
}

bool IniFile::hasDouble(const std::string group, const std::string name)
{
    if (!hasString(group, name)) {
        return false;
    }

    size_t pos;

    try {
        std::stod(m_values[group][name], &pos);
    } catch (std::invalid_argument&) {
        return false;
    }

    return (pos > 0);
}

bool IniFile::hasBool(const std::string group, const std::string name)
{
    if (!hasString(group, name)) {
        return false;
    }

    const std::string val = m_values[group][name];
    return (val == "true") || (val == "yes") || (val == "false") || (val == "no");
}

bool IniFile::hasInt(const std::string group, const std::string name)
{
    if (!hasString(group, name)) {
        return false;
    }

    size_t pos;

    try {
        std::stoi(m_values[group][name], &pos);
    } catch (std::invalid_argument&) {
        return false;
    }

    return (pos > 0);
}

} // namespace utils
