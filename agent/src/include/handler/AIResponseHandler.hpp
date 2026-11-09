//
// Created by root on 2026/10/14.
//

#ifndef LIYONG_OLLAMA_AGENT_AIRESPONSEHANDLER_HPP
#define LIYONG_OLLAMA_AGENT_AIRESPONSEHANDLER_HPP

#include <string>
#include <filesystem>
#include <optional>
#include <vector>

#include "config/EnvironConfig.hpp"
#include "model/OpenAISpec.hpp"
#include "spdlog/spdlog.h"
#include <nlohmann/json.hpp>

#include "../register/ToolRegistry.hpp"
#include "parser/GenericXmlParser.hpp"

namespace fs = std::filesystem;

// 通用的工具呼叫結構
struct ToolCallInvocation
{
    std::string tool;
    nlohmann::json arguments;
};

class AIResponseHandler
{
public:
    // ==========================================
    // 1. 核心方法：解析 ToolCalls
    //    優先序：原生 tool_calls -> XML <response> -> JSON（最後才用，避免 C++ 大括號誤判）
    // ==========================================
    static std::optional<std::vector<ToolCallInvocation>> parseToolCalls(const openai_spec::ModelResponse& response)
    {
        std::vector<ToolCallInvocation> invocations;

        const auto& choices = response.get_choices();
        if (choices.empty())
        {
            return std::nullopt;
        }

        const auto& message = choices[0].get_message();
        const auto& tool_calls_opt = message.get_tool_calls();

        if (tool_calls_opt.has_value() && !tool_calls_opt->empty())
        {
            for (const auto& tool_call : tool_calls_opt.value())
            {
                const auto& func = tool_call.get_function();
                std::string name = func.get_name();
                std::string args_str = func.get_arguments();

                try
                {
                    nlohmann::json args = nlohmann::json::parse(args_str);
                    invocations.push_back({name, args});
                    spdlog::info("Successfully parsed ToolCall from Spec: {}", name);
                }
                catch (const std::exception& e)
                {
                    spdlog::error("Failed to parse tool arguments JSON: {}", e.what());
                }
            }
        }

        if (!invocations.empty())
        {
            return invocations;
        }

        // 備用方案：模型把 tool call 寫在 content 裡
        std::string content = message.get_content();
        if (!content.empty())
        {
            auto xml_invocations = parseToolCallsFromXml(content);
            if (xml_invocations.has_value() && !xml_invocations->empty())
            {
                return xml_invocations.value();
            }

            auto json_invocations = parseToolCallsFromJson(content);
            if (json_invocations.has_value() && !json_invocations->empty())
            {
                return json_invocations.value();
            }
        }

        return std::nullopt;
    }

    // 將解析後的呼叫重新序列化成乾淨的 XML（放進對話歷史，避免模型模仿自己的錯誤格式）
    static std::string serializeInvocations(const std::vector<ToolCallInvocation>& invs)
    {
        std::string out;
        for (const auto& inv : invs)
        {
            out += "<response>\n";
            out += "<tool>" + inv.tool + "</tool>\n";
            if (inv.tool == "create_file")
            {
                out += "<path>" + inv.arguments.value("file_path", std::string()) + "</path>\n";
                out += "<content><![CDATA[\n" +
                    escapeCdata(inv.arguments.value("content", std::string())) +
                    "]]></content>\n";
            }
            else
            {
                const std::string body = ToolRegistry::getInstance().getBodyParam(inv.tool);
                if (!body.empty())
                {
                    out += "<content><![CDATA[" +
                        escapeCdata(inv.arguments.value(body, std::string())) +
                        "]]></content>\n";
                }
            }
            out += "</response>\n";
        }
        return out;
    }

    // 萃取 Markdown code block
    static std::string extract_codeblock(const std::string& rawResponse)
    {
        std::string blocks = extract_all_code_blocks(rawResponse);
        return blocks.empty() ? rawResponse : blocks;
    }

private:
    // ---------- 字串工具 ----------
    static std::string trim(const std::string& s)
    {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return "";
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    }

    static std::string xmlUnescape(std::string s)
    {
        auto rep = [&](const std::string& from, const std::string& to)
        {
            size_t p = 0;
            while ((p = s.find(from, p)) != std::string::npos)
            {
                s.replace(p, from.size(), to);
                p += to.size();
            }
        };
        rep("&lt;", "<");
        rep("&gt;", ">");
        rep("&quot;", "\"");
        rep("&apos;", "'");
        rep("&amp;", "&"); // 必須最後處理
        return s;
    }

    // CDATA 內若含 "]]>" 需要拆開
    static std::string escapeCdata(const std::string& s)
    {
        std::string out;
        size_t pos = 0, p;
        while ((p = s.find("]]>", pos)) != std::string::npos)
        {
            out.append(s, pos, p - pos);
            out += "]]]]><![CDATA[>";
            pos = p + 3;
        }
        out.append(s, pos, std::string::npos);
        return out;
    }

    // 解碼 <content> 內容：CDATA > 轉義內容 > 原始內容
    static std::string decodeContent(std::string c)
    {
        size_t cd = c.find("<![CDATA[");
        if (cd != std::string::npos)
        {
            size_t st = cd + 9;
            size_t en = c.rfind("]]>");
            if (en != std::string::npos && en >= st)
            {
                std::string inner = c.substr(st, en - st);
                if (!inner.empty() && inner.front() == '\n') inner.erase(0, 1);
                return inner;
            }
        }
        if (!c.empty() && c.front() == '\n') c.erase(0, 1);
        return xmlUnescape(c);
    }

    static std::string tagText(const std::string& b, const std::string& tag)
    {
        const std::string o = "<" + tag + ">", c = "</" + tag + ">";
        size_t a = b.find(o);
        if (a == std::string::npos) return "";
        a += o.size();
        size_t e = b.find(c, a);
        return (e == std::string::npos) ? "" : b.substr(a, e - a);
    }

    // ---------- Markdown code block ----------
    static std::string extract_all_code_blocks(const std::string& s)
    {
        std::string out;
        size_t pos = 0;
        while (true)
        {
            size_t open = s.find("```", pos);
            if (open == std::string::npos) break;

            size_t lineEnd = s.find('\n', open);
            if (lineEnd == std::string::npos) break;

            size_t close = s.find("```", lineEnd + 1);
            if (close == std::string::npos) break;

            out.append(s, lineEnd + 1, close - (lineEnd + 1));
            out += '\n';
            pos = close + 3;
        }
        return out;
    }

    // ---------- JSON 備用解析 ----------
    static std::optional<std::vector<ToolCallInvocation>> parseToolCallsFromJson(const std::string& aiResponse)
    {
        std::string target_str = extract_all_code_blocks(aiResponse);
        if (target_str.empty())
        {
            target_str = aiResponse;
        }

        std::vector<ToolCallInvocation> invocations;
        size_t pos = 0;

        while ((pos = target_str.find('{', pos)) != std::string::npos)
        {
            size_t start_pos = pos;
            int brace_count = 0;
            bool inside_string = false;
            bool escape = false;
            size_t end_pos = std::string::npos;

            for (size_t i = start_pos; i < target_str.size(); ++i)
            {
                char c = target_str[i];
                if (escape)
                {
                    escape = false;
                    continue;
                }
                if (c == '\\')
                {
                    escape = true;
                    continue;
                }
                if (c == '"') { inside_string = !inside_string; }
                if (!inside_string)
                {
                    if (c == '{')
                    {
                        brace_count++;
                    }
                    else if (c == '}')
                    {
                        brace_count--;
                        if (brace_count == 0)
                        {
                            end_pos = i;
                            break;
                        }
                    }
                }
            }

            if (end_pos == std::string::npos) break;

            std::string single_json_str = target_str.substr(start_pos, (end_pos - start_pos) + 1);
            pos = end_pos + 1;

            try
            {
                nlohmann::json j = nlohmann::json::parse(single_json_str);

                std::string tool_name;
                nlohmann::json args;

                if (j.is_object() && j.contains("name"))
                {
                    tool_name = j["name"].get<std::string>();
                    args = j.contains("arguments") ? j["arguments"] : j;
                }
                else if (j.is_object() && j.contains("function"))
                {
                    tool_name = j["function"].value("name", "");
                    args = j["function"].contains("arguments") ? j["function"]["arguments"] : j;
                }

                if (args.is_string())
                {
                    args = nlohmann::json::parse(args.get<std::string>());
                }

                if (!tool_name.empty())
                {
                    invocations.push_back({tool_name, args});
                    spdlog::info("Parsed Text-fallback Tool Call: {}", tool_name);
                }
            }
            catch (const std::exception& e)
            {
                spdlog::debug("Skip unparsable JSON fragment: {}", e.what());
                continue;
            }
        }

        return invocations.empty() ? std::nullopt : std::optional(invocations);
    }

    // ---------- XML 解析（不遞迴，不依賴 GenericXmlParser）----------
    // 格式：
    //   <response><tool>create_file</tool><path>a.cpp</path><content>...</content></response>
    // content 取區塊內「最後一個」</content>，所以程式碼裡有任何 < > 都不會壞
    static std::optional<std::vector<ToolCallInvocation>> parseToolCallsFromXml(const std::string& s)
    {
        std::vector<ToolCallInvocation> invocations;
        size_t pos = 0;

        std::vector<XmlNode> rootNodes = GenericXmlParser::parseAllRootNodes(s, "response", {"content"});


        for (const XmlNode& rootNode : rootNodes)
        {
            ToolCallInvocation inv;

            if (rootNode.getAttributes().count("tool"))
            {
                inv.tool = rootNode.getAttributes().at("tool");
            }
            else
            {
                inv.tool = rootNode.getChildText("tool");
            }

            nlohmann::json args = nlohmann::json::object();

            for (const auto& [attrKey, attrVal] : rootNode.getAttributes())
            {
                if (attrKey != "tool")
                {
                    args[attrKey] = attrVal;
                }
            }

            std::string path = rootNode.getChildText("path");
            std::string content = rootNode.getChildText("content");

            if (!path.empty()) args["file_path"] = path;
            if (!content.empty()) args["content"] = content;

            if (!inv.tool.empty())
            {
                inv.arguments = args;
                invocations.push_back(inv);
            }
        }

        return invocations.empty() ? std::nullopt : std::optional(invocations);


        // while (true)
        // {
        //     size_t open = s.find("<response>", pos);
        //     if (open == std::string::npos) break;
        //
        //     const size_t bodyStart = open + 10;
        //     size_t nextOpen = s.find("<response>", bodyStart);
        //     // nextOpen 為 npos 時 rfind 會搜尋全文
        //     size_t close = s.rfind("</response>", nextOpen);
        //     if (close == std::string::npos || close < bodyStart)
        //     {
        //         pos = bodyStart;
        //         continue;
        //     }
        //
        //     std::string block = s.substr(bodyStart, close - bodyStart);
        //     pos = close + 11;
        //
        //     // 先切出 <content>，其餘部分才找 tool/path，
        //     // 避免程式碼內出現 "<path>" 之類字串造成誤判
        //     std::string content;
        //     std::string head = block;
        //     size_t cs = block.find("<content>");
        //     size_t ce = block.rfind("</content>");
        //     if (cs != std::string::npos && ce != std::string::npos && ce > cs)
        //     {
        //         content = decodeContent(block.substr(cs + 9, ce - cs - 9));
        //         head = block.substr(0, cs) + block.substr(ce + 10);
        //     }
        //
        //     std::string tool = trim(tagText(head, "tool"));
        //     if (tool.empty()) continue;
        //
        //     std::string path = trim(tagText(head, "path"));
        //
        //     nlohmann::json args = nlohmann::json::object();
        //     if (tool == "create_file")
        //     {
        //         if (path.empty())
        //         {
        //             spdlog::warn("create_file without <path>, skipped");
        //             continue;
        //         }
        //         args["file_path"] = path;
        //         args["content"] = content;
        //     }
        //     else
        //     {
        //         // 標籤內文要放進哪個參數，由工具自己宣告（getBodyParam）；空字串代表不使用內文
        //         const std::string body = ToolRegistry::getInstance().getBodyParam(tool);
        //         if (!body.empty()) args[body] = trim(content);
        //         if (!path.empty()) args["file_path"] = path;
        //     }
        //
        //     spdlog::info("Parsed XML Tool Call: {}", tool);
        //     out.push_back({tool, args});
        // }
        //
        // return out.empty() ? std::nullopt : std::optional(out);
    }
};

#endif //LIYONG_OLLAMA_AGENT_AIRESPONSEHANDLER_HPP
