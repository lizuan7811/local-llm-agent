//
// Created by root on 2026/10/6.
//

#include "ConfigManager.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

using json = nlohmann::json;

ConfigManager::ConfigManager() : initialized(false) {
}

ConfigManager &ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

std::string get_absolute_path(const std::string &relative_path) {
    return std::filesystem::absolute(relative_path).string();
}

void init_mission(EnvironConfig &environ_config) {
    environ_config.mission.prompts_dir_path = get_absolute_path(environ_config.mission.prompts_dir_path);
    environ_config.mission.work_dir_path = get_absolute_path(environ_config.mission.work_dir_path);
}

bool ConfigManager::init(const std::string &relative_path) {
    if (initialized) {
        return true;
    }

    // get init file absolute path and read into memory
    std::ifstream file(get_absolute_path(relative_path));

    if (!file.is_open()) {
        return false;
    }
    try {
        json j;
        file >> j;
        environ_config = j.get<EnvironConfig>();

        init_mission(environ_config);

        initialized = true;

        return true;
    } catch (const std::exception &e) {
        spdlog::error(fmt::runtime("JSON parsing error: {}"), e.what());
        return false;
    }
}

const EnvironConfig &ConfigManager::get() const {
    return environ_config;
}
