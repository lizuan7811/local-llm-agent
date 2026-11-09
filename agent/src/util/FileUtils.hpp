//
// Created by root on 2026/10/22.
//

#ifndef LIYONG_OLLAMA_AGENT_FILEUTILS_HPP
#define LIYONG_OLLAMA_AGENT_FILEUTILS_HPP
#include <filesystem>
#include <fstream>
#include <string>
#include <optional>

#include "spdlog/spdlog.h"
namespace fs = std::filesystem;

namespace aiagent
{
    class FileUtils
    {
    public:
        // 將指定路徑的檔案讀取並轉為 std::string
        static std::string readFileToString(const std::string& filepath)
        {
            if (!std::filesystem::exists(filepath))
            {
                spdlog::error("File path does not exist: {}", filepath);
                throw std::runtime_error("File path does not exist, filePath: " + filepath);
            }

            std::ifstream file(filepath, std::ios::in | std::ios::binary);
            if (!file.is_open())
            {
                spdlog::error("Failed to open file: {}", filepath);
                throw std::runtime_error("Failed to open file: " + filepath);
            }

            std::ostringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }

        // ==========================================
        // 2. 實用安全工具：路徑越界防護
        // ==========================================
        static std::optional<fs::path> resolveSafePath(const fs::path& basePath, const std::string& relativePath)
        {
            fs::path rel(relativePath);

            if (rel.empty() || rel.is_absolute())
            {
                return std::nullopt;
            }

            fs::path base = fs::absolute(basePath).lexically_normal();
            fs::path full = fs::absolute(base / rel).lexically_normal();

            fs::path checkRel = full.lexically_relative(base);
            if (checkRel.empty())
            {
                return std::nullopt;
            }

            // 第一段是 ".." 才算越界
            if (*checkRel.begin() == "..")
            {
                return std::nullopt;
            }

            return full;
        }

        // 去掉開頭的 "./" 或 ".\"
        static std::string stripDotSlash(std::string p)
        {
            if (p.rfind("./", 0) == 0 || p.rfind(".\\", 0) == 0)
            {
                p = p.substr(2);
            }
            return p;
        }

        // 洗鍊 Ollama 回應的髒格式
        static std::string sanitizeLlgyXmlResponse(std::string rawResponse)
        {
            // 1. 如果開頭有帶 ```xml 或 ```，把它們拔掉
            if (rawResponse.rfind("```xml", 0) == 0)
            {
                rawResponse = rawResponse.substr(6);
            }
            else if (rawResponse.rfind("```json", 0) == 0)
            {
                rawResponse = rawResponse.substr(7);
            }
            else if (rawResponse.rfind("```", 0) == 0)
            {
                rawResponse = rawResponse.substr(3);
            }

            // 2. 如果結尾有帶 ```，把它拔掉
            if (rawResponse.size() >= 3 && rawResponse.substr(rawResponse.size() - 3) == "```")
            {
                rawResponse = rawResponse.substr(0, rawResponse.size() - 3);
            }

            return rawResponse;
        }
    };
} // namespace aiagent
#endif //LIYONG_OLLAMA_AGENT_FILEUTILS_HPP
