//
// Created by root on 2026/10/27.
//

#ifndef LIYONG_OLLAMA_AGENT_STRINGUTILS_H
#define LIYONG_OLLAMA_AGENT_STRINGUTILS_H
#include <string>


class StringUtils
{
public:
    static std::string trim(const std::string& s)
    {
        auto b = s.find_first_not_of(" \t\r\n\"'`.");
        if (b == std::string::npos) return "";
        auto e = s.find_last_not_of(" \t\r\n\"'`.");
        return s.substr(b, e - b + 1);
    }
};


#endif //LIYONG_OLLAMA_AGENT_STRINGUTILS_H
