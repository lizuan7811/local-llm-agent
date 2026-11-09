//
// Created by root on 2026/10/12.
//

#include "PromptManager.hpp"

#include <filesystem>
#include <fstream>
#include <mutex>

#include "spdlog/spdlog.h"
#include "util/FileUtils.hpp"

PromptManager::PromptManager(const std::string &prompt_path,
                             const std::string &prompt_header_name,
                             const std::string &prompt_content_name) {
    this->prompt_header_path_ = prompt_path + prompt_header_name;
    this->prompt_content_path_ = prompt_path + prompt_content_name;
    reload();
}

void PromptManager::reload() {
    try {

        std::string new_header_prompt = aiagent::FileUtils::readFileToString(prompt_header_path_);
        std::string new_content_prompt = aiagent::FileUtils::readFileToString(prompt_content_path_);

        std::unique_lock<std::shared_mutex> lock(mutex_);

        cached_header_ = std::move(new_header_prompt);
        cached_content_ = std::move(new_content_prompt);

        spdlog::info("[PromptManager] - Successfully load/reloaded header prompt from : {}, content prompt from : {}, content size: {} bytes.",
                     prompt_header_path_,
                     prompt_content_path_,
                     cached_content_.size());
    } catch (std::exception &e) {
        spdlog::error("[PromptManager] - {}", e.what());
    }
}

std::string PromptManager::get_header_prompt() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return cached_header_;
}

std::string PromptManager::get_content_prompt() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return cached_content_;
}

std::string PromptManager::get_header_promt_path() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return prompt_header_path_;
}

std::string PromptManager::get_content_promt_path() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return prompt_content_path_;
}
//
// std::string PromptManager::read_file_to_string(const std::string &filepath) {
//     if (!std::filesystem::exists(filepath)) {
//         spdlog::error("prompt: filepath {} does not exist", filepath);
//         throw std::runtime_error("prompt: filepath does not exist, filePath: " + filepath);
//     }
//     std::ifstream file(filepath, std::ios::in | std::ios::binary);
//     if (!file.is_open()) {
//         spdlog::error("Failed to open prompt file: {}", filepath);
//         throw std::runtime_error("Failed to open prompt file: " + filepath);
//     }
//
//     std::ostringstream ss;
//     ss << file.rdbuf();
//     return ss.str();
// }
