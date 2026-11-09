//
// Created by root on 2026/10/22.
//

#ifndef LIYONG_OLLAMA_AGENT_COTSTRATEGY_HPP
#define LIYONG_OLLAMA_AGENT_COTSTRATEGY_HPP

#include <string>

#include "IReasoningStrategy.hpp"
#include "handler/AIResponseHandler.hpp"
#include "../adapter/IModelAdapter.hpp"
#include "../register/ToolRegistry.hpp"
#include "util/FileUtils.hpp"

namespace aiagent
{
    class ReActStrategy : public IReasoningStrategy
    {
    private:
        std::shared_ptr<IModelAdapter> model_adapter_;

        // 7B 模型實際上輸出的是 XML。同時送 tools 與 XML 格式會互相干擾，
        // 所以預設只用 XML。若換成工具呼叫穩定的大模型，改成 true 即可。
        static constexpr bool kUseNativeTools = false;

    public:
        explicit ReActStrategy(std::shared_ptr<IModelAdapter> adapter)
            : model_adapter_(std::move(adapter))
        {
        }

        openai_spec::ModelResponse execute(AgentContext& context) override
        {
            openai_spec::ModelResponse model_response;
            const int maxTurns = context.getMaxRetry();
            nlohmann::json tools_payload = ToolRegistry::getInstance().getToolsPayload();

            // 初始訊息只加一次
            context.addChatHistory(openai_spec::Message("system", context.getSysInput()));
            context.addChatHistory(openai_spec::Message("user", context.getUserInput()));

            std::string lastSignature;

            for (int turn = 0; turn < maxTurns; ++turn)
            {
                openai_spec::ModelRequest req;
                req.set_model(model_adapter_->get_model())
                   .set_stream(false)
                   .set_max_tokens(4096);
                // 若 ModelRequest 有 set_temperature，建議加上：
                // req.set_temperature(0.0);

                for (const auto& msg : context.getChatHistory())
                    req.add_message(msg);

                if (kUseNativeTools && !tools_payload.empty())
                    req.set_tools(tools_payload);

                try
                {
                    model_response = model_adapter_->chat(req);
                }
                catch (const std::exception& e)
                {
                    spdlog::error("LLM call failed: {}", e.what());
                    break;
                }

                const auto& choices = model_response.get_choices();
                if (choices.empty())
                {
                    spdlog::warn("LLM returned no choices");
                    break;
                }

                const openai_spec::Message& assistantMsg = choices[0].get_message();
                auto invocations = AIResponseHandler::parseToolCalls(model_response);

                if (!invocations || invocations->empty())
                {
                    context.addChatHistory(assistantMsg);
                    spdlog::info("CoT finished: no more tool calls.");
                    return model_response;
                }

                // 歷史紀錄：原生 tool_calls 保留原訊息；
                // XML 文字模式則放入「重新序列化後的乾淨格式」，避免模型模仿自己的轉義錯誤
                const auto& nativeCalls = assistantMsg.get_tool_calls();
                if (nativeCalls.has_value() && !nativeCalls->empty())
                {
                    context.addChatHistory(assistantMsg);
                }
                else
                {
                    openai_spec::Message cleanMsg;
                    cleanMsg.set_role("assistant")
                            .set_content(AIResponseHandler::serializeInvocations(*invocations));
                    context.addChatHistory(cleanMsg);
                }

                // 重複呼叫偵測：連續兩輪完全相同就中止，避免無限重試
                std::string signature;
                for (const auto& inv : *invocations)
                    signature += inv.tool + inv.arguments.dump();

                if (signature == lastSignature)
                {
                    spdlog::warn("CoT aborted: repeated identical tool calls");
                    break;
                }
                lastSignature = signature;

                for (const auto& inv : *invocations)
                {
                    std::string result = ToolRegistry::getInstance().executeTool(inv.tool, inv.arguments);
                    openai_spec::Message tool_msg;
                    tool_msg.set_role("tool").set_content(result);
                    context.addChatHistory(tool_msg);
                }
            }

            spdlog::warn("CoT stopped (max turns = {})", maxTurns);
            return model_response;
        }
    };
}
#endif //LIYONG_OLLAMA_AGENT_COTSTRATEGY_HPP