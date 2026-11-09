#include "LogManager.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <vector>

#include "ConfigManager.hpp"
#include "../config/EnvironConfig.hpp"

namespace {
    std::string ToLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return s;
    }
}

bool LogManager::initLogConfig() {
    try {
        const EnvironConfig &cfg = ConfigManager::getInstance().get();

        const std::string logLevel = cfg.global.log_level;
        const std::string logPath = cfg.global.log_path; // 例如 "logs/agent.log"
        const std::string logPattern = cfg.global.log_pattern; // 例如 "logs/agent.log"

        auto lvl = spdlog::level::from_str(ToLower(logLevel));
        if (lvl == spdlog::level::off && ToLower(logLevel) != "off") {
            spdlog::warn("未知的 log_level '{}',改用 info", logLevel);
            lvl = spdlog::level::info;
        }

        // 先建立資料夾
        auto parent = std::filesystem::path(logPath).parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logPath, 1024 * 1024 * 5, 3);

        std::vector<spdlog::sink_ptr> sinks{console_sink, rotating_sink};
        auto logger = std::make_shared<spdlog::logger>(
            "agent_logger", sinks.begin(), sinks.end());

        logger->set_level(lvl); // 由 logger 統一控制等級
        logger->set_pattern(logPattern);
        logger->flush_on(lvl); // warn 以上立即寫檔

        spdlog::set_default_logger(logger);

        spdlog::info("日誌系統初始化成功,等級={},輸出至螢幕與 {}", logLevel, logPath);
        return true;
    } catch (const std::exception &ex) {
        std::printf("Log 初始化失敗: %s\n", ex.what());
        return false;
    }
}
