//
// build_and_run 工具：把 BuildRunner 包成 vTool 以便註冊到 ToolRegistry
//
#ifndef LIYONG_OLLAMA_AGENT_BUILDANDRUNTOOL_HPP
#define LIYONG_OLLAMA_AGENT_BUILDANDRUNTOOL_HPP

#include <filesystem>
#include <string>

#include "manager/ToolRegistry.hpp"
#include "tool/BuildRunner.hpp"   // 依你的專案目錄調整

class BuildAndRunTool : public vTool
{
private:
    aiagent::BuildRunner runner_;

public:
    // workspace 必須與 create_file 寫入的目錄相同
    explicit BuildAndRunTool(std::filesystem::path workspace,
                             int compileTimeoutSec = 60,
                             int runTimeoutSec = 10)
        : runner_(std::move(workspace), compileTimeoutSec, runTimeoutSec)
    {
    }

    [[nodiscard]] std::string getType() const override { return "function"; }
    [[nodiscard]] std::string getName() const override { return "build_and_run"; }

    [[nodiscard]] std::string getDescription() const override
    {
        return aiagent::BuildRunner::toolSchema()["function"]["description"].get<std::string>();
    }

    [[nodiscard]] nlohmann::json getParameters() const override
    {
        return aiagent::BuildRunner::toolSchema()["function"]["parameters"];
    }

    // 不使用標籤內文：<response><tool>build_and_run</tool></response> 即可
    [[nodiscard]] std::string getBodyParam() const override { return ""; }

    std::string execute(const nlohmann::json& args) const override
    {
        return runner_.execute(args);
    }
};

#endif //LIYONG_OLLAMA_AGENT_BUILDANDRUNTOOL_HPP