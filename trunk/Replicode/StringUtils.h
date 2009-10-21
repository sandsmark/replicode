// StringUtils.h

#if !defined(STRING_UTILS__INCLUDED_)
#define STRING_UTILS__INCLUDED_

#include <string>

namespace StringUtils
{

int StartsWith(const std::string &s, const std::string &str);

int EndsWith(const std::string &s, const std::string &str);

void MakeUpper(std::string &str);

void MakeLower(std::string &str);

void Trim(std::string& str, const char* chars2remove = " ");

void TrimLeft(std::string& str, const char* chars2remove = " ");

void TrimRight(std::string& str, const char* chars2remove = " ");

void ReplaceLeading(std::string& str, const char* chars2replace, char c);

std::string Int2String(int i);
}

#endif
