#include <algorithm>
#include <cctype>

#include "string_utils.h"


namespace	string_utils{

int StartsWith(const std::string &s, const std::string &str) {
	std::string::size_type pos = s.find_first_of(str);
	if (pos == 0)
		return 0;
	else
		return -1;
}

int EndsWith(const std::string &s, const std::string &str) {
	std::string::size_type pos = s.find_last_of(str);
	if (pos == s.size()-str.size())
		return pos;
	else
		return -1;
}

void MakeUpper(std::string &str)
{
	std::transform(str.begin(),str.end(),str.begin(),toupper);
}

void MakeLower(std::string &str)
{
	std::transform(str.begin(),str.end(),str.begin(),tolower);
}

void Trim(std::string& str, const char* chars2remove)
{
	TrimLeft(str, chars2remove);
	TrimRight(str, chars2remove);
}

void TrimLeft(std::string& str, const char* chars2remove)
{
	if (!str.empty())
	{
		std::string::size_type pos = str.find_first_not_of(chars2remove);

		if (pos != std::string::npos)
			str.erase(0,pos);
		else
			str.erase( str.begin() , str.end() ); // make empty
	}
}

void TrimRight(std::string& str, const char* chars2remove)
{
	if (!str.empty())
	{
		std::string::size_type pos = str.find_last_not_of(chars2remove);

		if (pos != std::string::npos)
			str.erase(pos+1);
		else
			str.erase( str.begin() , str.end() ); // make empty
	}
}


void ReplaceLeading(std::string& str, const char* chars2replace, char c)
{
	if (!str.empty())
	{
		std::string::size_type pos = str.find_first_not_of(chars2replace);

		if (pos != std::string::npos)
			str.replace(0,pos,pos,c);
		else
		{
			int n = str.size();
			str.replace(str.begin(),str.end()-1,n-1,c);
		}
	}
}

std::string Int2String(int i) {
	char buffer[1024];
	sprintf(buffer,"%d",i);
	return std::string(buffer);
}
	
}