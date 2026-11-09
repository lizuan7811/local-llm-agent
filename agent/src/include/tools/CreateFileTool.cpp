//
// Created by root on 2026/10/21.
//

#include <fstream>

#include "config/EnvironConfig.hpp"
#include "manager/ConfigManager.hpp"
#include "../register/ToolRegistry.hpp"
#include "util/FileUtils.hpp"

class CreateFileTool : public vTool
{
public:
    CreateFileTool()
    {
        // 關鍵：在建構子執行時，把自己自動註冊到單例 Registry 中！
        ToolRegistry::getInstance().registerTool(this);
    }

    // 補上這行，實作 vTool 的純虛擬函式
    [[nodiscard]] std::string getType() const override
    {
        return "function";
    }

    [[nodiscard]] std::string getName() const override
    {
        return "create_file";
    }

    [[nodiscard]] std::string getDescription() const override
    {
        return "Create or overwrite a file with given content.";
    }

    [[nodiscard]] nlohmann::json getParameters() const override
    {
        return {
            {"type", "object"},
            {
                "properties", {
                    {"file_path", {{"type", "string"}, {"description", "Target file path."}}},
                    {"content", {{"type", "string"}, {"description", "File content."}}}
                }
            },
            {"required", {"file_path", "content"}}
        };
    }

    [[nodiscard]] std::string getBodyParam() const override { return "content"; }

    // 修正點：移除 static，加上 const 和 override
    [[nodiscard]] std::string execute(const nlohmann::json& args) const override
    {
        const EnvironConfig& environ_config = ConfigManager::getInstance().get();

        std::string path = args.at("file_path");
        std::string content = args.at("content");

        std::optional<fs::path> realPath = aiagent::FileUtils::resolveSafePath(
            environ_config.mission.work_dir_path, path);

        if (realPath.has_value())
        {
            // const fs::path& realPath_ = realPath.value();

            const fs::path& realPath_(realPath.value());
            if (realPath_.has_parent_path())
            {
                fs::create_directories(realPath_.parent_path());
            }

            // 1. 如果你在專案中有用 spdlog，可以直接印出絕對路徑
            spdlog::info("Creating file folder and file at real path: {}", realPath_.string());

            std::ofstream outfile(realPath_);
            if (!outfile)
            {
                // 失敗時也可以把 realPath_ 印出幫助 Debug
                return "Failed to open file: " + path + " (resolved path: " + realPath_.string() + ")";
            }

            outfile << content;

            // 2. 成功時把 realPath_.string() 回傳包含在訊息中
            return "Successfully created file: " + path + " (real path: " + realPath_.string() + ")";
        }
        return "Failed to resolve safe path for: " + path;
    }
};

// 宣告一個全域靜態變數。
// 這會在程式啟動（進入 main 之前）自動呼叫 CreateFileTool 的建構子，
// 進而自動完成註冊，完全不需要在 main 裡面手動呼叫！
static CreateFileTool global_create_file_tool;
