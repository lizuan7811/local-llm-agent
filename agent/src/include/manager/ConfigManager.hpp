//
// Created by root on 2026/10/6.
//

#ifndef LIYONG_OLLAMA_AGENT_CONFIGMANAGER_HPP
#define LIYONG_OLLAMA_AGENT_CONFIGMANAGER_HPP

#pragma once
#include "../config/EnvironConfig.hpp"

class ConfigManager {
private:
    EnvironConfig environ_config;

    bool initialized;

    ConfigManager();

    ~ConfigManager() = default;

    ConfigManager(const ConfigManager &) = delete;

    ConfigManager &operator=(const ConfigManager &) = delete;

public:
    static ConfigManager &getInstance();

    bool init(const std::string &relative_path);

    const EnvironConfig &get() const;
};


#endif //LIYONG_OLLAMA_AGENT_CONFIGMANAGER_HPP
