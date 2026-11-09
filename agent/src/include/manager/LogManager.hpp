// LogManager.hpp
#ifndef LIYONG_OLLAMA_AGENT_LOGMANAGER_HPP
#define LIYONG_OLLAMA_AGENT_LOGMANAGER_HPP

#pragma once

class LogManager {
private:
    // 如果完全不需要建立實例，甚至可以把建構子直接 delete 掉，或維持 private
    LogManager() = delete;
    ~LogManager() = delete;

public:
    // 直接提供靜態初始化函數
    static bool initLogConfig();
};

#endif //LIYONG_OLLAMA_AGENT_LOGMANAGER_HPP