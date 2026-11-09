//
// Created by root on 2026/10/19.
//

#ifndef LIYONG_OLLAMA_AGENT_OLLAMAADAPTER_HPP
#define LIYONG_OLLAMA_AGENT_OLLAMAADAPTER_HPP

#include <string>
#include <unordered_map>
#include "nlohmann/json.hpp"
#include "service/AiAgentService.hpp"

// AI 模型/Agent 屬性配置檔
class ModelAdapterProperty
{
private:
    std::string agentCode;
    std::string version;
    std::string taskMode;
    std::string syspromptContent;
    std::string userpromptContent;

public:
    ModelAdapterProperty() = default;

    // 提供建構子方便初始化
    ModelAdapterProperty(std::string code, std::string ver, std::string mode,
                         std::string sysPrompt, std::string userPrompt)
        : agentCode(std::move(code)), version(std::move(ver)), taskMode(std::move(mode)),
          syspromptContent(std::move(sysPrompt)), userpromptContent(std::move(userPrompt)) {}

    // Getters (常數引用以提升效能)
    const std::string& getAgentCode() const { return agentCode; }
    const std::string& getVersion() const { return version; }
    const std::string& getTaskMode() const { return taskMode; }
    const std::string& getSyspromptContent() const { return syspromptContent; }
    const std::string& getUserpromptContent() const { return userpromptContent; }

    // Setters
    void setAgentCode(const std::string& code) { agentCode = code; }
    void setVersion(const std::string& ver) { version = ver; }
    void setTaskMode(const std::string& mode) { taskMode = mode; }
    void setSyspromptContent(const std::string& sys) { syspromptContent = sys; }
    void setUserpromptContent(const std::string& user) { userpromptContent = user; }
};

// 全域模型適配單例 (Meyers' Singleton)
class ModelAdapter
{
private:
    // 預設建構與解構子必須可存取 (私有即可)，不能 = delete
    ModelAdapter() = default;
    ~ModelAdapter() = default;

    // 禁止 Copy / Move
    ModelAdapter(const ModelAdapter&) = delete;
    ModelAdapter& operator=(const ModelAdapter&) = delete;

    std::unordered_map<std::string, ModelAdapterProperty> properties;

public:
    static ModelAdapter& getInstance();

    // 建議將 Property 傳參改為 const reference，避免不必要的深拷貝
    void addProperty(const ModelAdapterProperty& property);

    // 根據 agentCode 獲取對應配置
    bool getProperty(const std::string& agentCode, ModelAdapterProperty& outProperty) const;
};

#endif //LIYONG_OLLAMA_AGENT_OLLAMAADAPTER_HPP