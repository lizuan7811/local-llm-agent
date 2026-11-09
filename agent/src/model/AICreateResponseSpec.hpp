//
// Created by root on 2026/10/14.
//

#ifndef LIYONG_OLLAMA_AGENT_AIRESCREATEFILESPEC_HPP
#define LIYONG_OLLAMA_AGENT_AIRESCREATEFILESPEC_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class AiCreateFileSpec {
    std::string path_;
    std::string content_;

public:
    AiCreateFileSpec() = default;

    AiCreateFileSpec(std::string path, std::string content)
        : path_(std::move(path)), content_(std::move(content)) {
    }

    const std::string &get_path() const { return path_; }
    const std::string &get_content() const { return content_; }

    // 【關鍵】這裡必須加上這行，讓 nlohmann/json 知道怎麼序列化 AiCreateFileSpec
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AiCreateFileSpec, path_, content_)
};

class AICreateResponseSpec {
    std::vector<AiCreateFileSpec> createFileSpecs_;

public:
    AICreateResponseSpec() = default;

    explicit AICreateResponseSpec(std::vector<AiCreateFileSpec> specs)
        : createFileSpecs_(std::move(specs)) {
    }

    // 提供 Getter 讓外部（如檔案寫入模組）能夠遍歷
    const std::vector<AiCreateFileSpec> &get_create_file_specs() const {
        return createFileSpecs_;
    }

    // 讓 nlohmann/json 能夠序列化 AIResponseSpec
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AICreateResponseSpec, createFileSpecs_)
};

#endif //LIYONG_OLLAMA_AGENT_AIRESCREATEFILESPEC_HPP
