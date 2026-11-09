//
// Created by root on 2026/10/20.
//

#ifndef LIYONG_OLLAMA_AGENT_TOOLREGISTRY_HPP
#define LIYONG_OLLAMA_AGENT_TOOLREGISTRY_HPP

#include <exception>
#include <map>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

// ---------------------------------------------------------------------------
// 工具基底類別
// ---------------------------------------------------------------------------
class vTool {
public:
    virtual ~vTool() = default;

    [[nodiscard]] virtual std::string getType() const = 0;

    [[nodiscard]] virtual std::string getName() const = 0;

    [[nodiscard]] virtual std::string getDescription() const = 0;

    [[nodiscard]] virtual nlohmann::json getParameters() const = 0;

    // XML fallback 用：標籤內文要對應到哪個參數名；空字串代表此工具不使用內文
    [[nodiscard]] virtual std::string getBodyParam() const { return ""; }

    virtual std::string execute(const nlohmann::json &args) const = 0;

    // 只輸出 OpenAI 規格欄位（bodyParam 是內部用，不送給模型）
    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"type", getType()},
            {
                "function", {
                    {"name", getName()},
                    {"description", getDescription()},
                    {"parameters", getParameters()}
                }
            }
        };
    }
};

// ---------------------------------------------------------------------------
// 工具註冊中心（Meyers singleton）
// 注意：Registry 不擁有 vTool，工具生命週期由各自的全域/靜態物件管理
// ---------------------------------------------------------------------------
class ToolRegistry {
private:
    ToolRegistry() = default;

    ~ToolRegistry() = default;

    ToolRegistry(const ToolRegistry &) = delete;

    ToolRegistry &operator=(const ToolRegistry &) = delete;

    // 用 std::map 讓遍歷順序固定，送給模型的 tools 陣列才穩定（有利於 prompt cache）
    std::map<std::string, vTool *> tools_;

public:
    static ToolRegistry &getInstance() {
        static ToolRegistry instance;
        return instance;
    }

    void registerTool(vTool *tool) {
        if (tool) {
            tools_[tool->getName()] = tool;
        }
    }

    [[nodiscard]] vTool *getTool(const std::string &name) {
        auto it = tools_.find(name);
        return it != tools_.end() ? it->second : nullptr;
    }

    [[nodiscard]] const vTool *getTool(const std::string &name) const {
        auto it = tools_.find(name);
        return it != tools_.end() ? it->second : nullptr;
    }

    // 組出送給 Ollama / OpenAI 的 tools 陣列
    [[nodiscard]] nlohmann::json getToolsPayload() const {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto &[name, tool]: tools_) {
            arr.push_back(tool->toJson());
        }
        return arr;
    }

    // 回傳所有已註冊工具名稱（給 XML fallback 掃描用）
    [[nodiscard]] std::vector<std::string> getToolNames() const {
        std::vector<std::string> names;
        names.reserve(tools_.size());
        for (const auto &[name, tool]: tools_) {
            names.push_back(name);
        }
        return names;
    }

    // 查詢某工具的 body 參數名；找不到工具回傳空字串
    [[nodiscard]] std::string getBodyParam(const std::string &name) const {
        const vTool *tool = getTool(name);
        return tool ? tool->getBodyParam() : std::string();
    }

    // 執行工具；所有錯誤都轉成字串回傳給模型，讓它能自行修正
    std::string executeTool(const std::string &name, const nlohmann::json &args) {
        vTool *tool = getTool(name);
        if (!tool) {
            std::string available;
            for (const auto &n: getToolNames()) {
                if (!available.empty()) available += ", ";
                available += n;
            }
            return "Error: unknown tool '" + name + "'. Available tools: " + available +
                   ". Do not call any other tool. If all work is finished, reply with the single word: DONE";
        }
        try {
            return tool->execute(args);
        } catch (const std::exception &e) {
            return "Error executing tool '" + name + "': " + e.what();
        }
    }
};

#endif //LIYONG_OLLAMA_AGENT_TOOLREGISTRY_HPP