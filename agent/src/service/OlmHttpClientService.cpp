#include <iostream>
#include <string>
#include "OlmHttpClientService.hpp"

#include "../include/manager/ConfigManager.hpp"
#include "handler/AIResponseHandler.hpp"
#include "../include/register/ToolRegistry.hpp"

#include "model/OpenAISpec.hpp"

class AICreateResponseSpec;
/**
 *  Ollama call llm client
 * @param request
 * @return
 *
 */
openai_spec::ModelResponse OlmHttpClientService::chat(
    const openai_spec::ModelRequest &request) {
    std::string request_body = nlohmann::json(request).dump();
    spdlog::debug("ollama request: {}", request_body);
    openai_spec::ModelResponse response_data;
    try {
        std::string resBody = client_.postJson(this->url + this->compatibility_api_path, request_body);

        if (!resBody.empty()) {
            try {
                nlohmann::json res_json = nlohmann::json::parse(resBody);
                response_data = res_json.get<openai_spec::ModelResponse>();
                spdlog::debug("OLLAMA response: {}", resBody);
            } catch (std::exception &e) {
                spdlog::error("OLLAMA response: {}", e.what());
            }
        } else {
            spdlog::error("[HTTP Error]: response body is empty.");
        }
    } catch (const std::exception &e) {
        spdlog::error("OLLAMA Send Error: {}", e.what());
    }
    return response_data;
}

#include "spdlog/spdlog.h"

openai_spec::ModelResponse OlmHttpClientService::callOllama(const std::string &content) {
    // 1. 使用類別內部的成員變數 model 與 stream 組裝 Request
    openai_spec::ModelRequest req;
    req.set_model(this->model); // 讀取初始化時載入的 model
    req.set_stream(this->stream); // 讀取初始化時載入的 stream

    // 設定對話訊息
    openai_spec::Message user_message;
    user_message.set_role("user");
    user_message.set_content(content);
    req.add_message(user_message);

    // 2. 轉為 JSON 發送
    nlohmann::json req_json = req;
    std::string request_body = req_json.dump();

    return chat(req);
}

// 簡化方法 2：Tool Call 複雜邏輯
openai_spec::ModelResponse OlmHttpClientService::callOllamaToolCall(const std::string &headerPrompt,
                                                                    const std::string &contentPrompt) {
    openai_spec::ModelRequest req;
    req.set_model(this->model);
    req.set_stream(this->stream);

    const Mission &mission = ConfigManager::getInstance().get().mission;

    openai_spec::Message sys_message(
        "system", headerPrompt);
    openai_spec::Message user_message("user", contentPrompt);

    req.add_message(sys_message);
    req.add_message(user_message);

    // 建構 Tool Def
    nlohmann::json parameters = {
        {"type", "object"},
        {
            "properties", {
                {
                    "file_path", {
                        {"type", "string"},
                        {"description", "The target relative or absolute file path, for example: ./test/test.cpp"}
                    }
                },
                {
                    "content", {
                        {"type", "string"},
                        {
                            "description",
                            "Pure C++ source code content. STRICTLY NO MARKDOWN, NO ```cpp tags, and no conversational text."
                        }
                    }
                }
            }
        },
        {"required", {"file_path", "content"}}
    };

    openai_spec::FunctionDef function_def("create_file", "Create or overwrite a C++ source code file.", parameters);
    req.add_tool(openai_spec::Tool(function_def));

    // 發送請求
    openai_spec::ModelResponse response_data = chat(req);

    // 處理 Tool Call / AI 回應的落地邏輯 (保持你原本的檔案寫入行為)
    try {
        // 1. 透過 AIResponseHandler 統一解析 ModelResponse
        auto tool_invocations_opt = AIResponseHandler::parseToolCalls(response_data);

        if (tool_invocations_opt.has_value() && !tool_invocations_opt->empty()) {
            // 2. 有工具呼叫：逐一透過 ToolRegistry 派發執行
            for (const auto &inv : tool_invocations_opt.value()) {
                spdlog::info("Executing tool: {}", inv.tool);

                // 這裡自動對應到你的 CreateFileTool 或未來擴充的其他工具
                std::string result = ToolRegistry::getInstance().executeTool(inv.tool, inv.arguments);

                spdlog::info("Tool execution result [{}]: {}", inv.tool, result);
            }
        }
        else {
            // 3. 沒有工具呼叫：這是一般的文字回答
            const auto &choices = response_data.get_choices();
            if (!choices.empty()) {
                std::string raw_content = choices[0].get_message().get_content();
                spdlog::info("Model responded with text content: {}", raw_content);
            }
        }
    } catch (const std::exception &e) {
        spdlog::error("[Tool Call Processing Error]: {}", e.what());
    }

    return response_data;
}
