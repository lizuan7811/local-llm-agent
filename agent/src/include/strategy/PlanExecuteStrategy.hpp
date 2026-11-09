//
// Created by root on 2026/10/22.
//

#ifndef LIYONG_OLLAMA_AGENT_TOTSTRATEGY_HPP
#define LIYONG_OLLAMA_AGENT_TOTSTRATEGY_HPP
#include "IReasoningStrategy.hpp"
#include "../adapter/IModelAdapter.hpp"
#include "handler/AIResponseHandler.hpp"
#include "../register/ToolRegistry.hpp"
#include "util/FileUtils.hpp"

/**
 * 維護多條路徑的樹狀展開與評估邏輯。
 */

namespace aiagent
{
    class PlanExecuteStrategy : public IReasoningStrategy
    {
    private:
        std::shared_ptr<aiagent::IModelAdapter> model_adapter_;

        openai_spec::ModelResponse directCall(aiagent::AgentContext& context)
        {
            // 1. 組裝標準的 Request
            openai_spec::ModelRequest req;
            req.set_model(model_adapter_->get_model()); // 使用 adapter 內部的 model 名稱
            req.set_stream(false);

            openai_spec::Message sys_message("system", context.getSysInput());
            openai_spec::Message user_message("user", context.getUserInput());
            req.add_message(sys_message);
            req.add_message(user_message);

            // 從 Singleton 拿取所有註冊工具的 JSON 結構，並塞進 Request 裡！
            nlohmann::json tools_payload = ToolRegistry::getInstance().getToolsPayload();
            if (!tools_payload.empty())
            {
                req.set_tools(tools_payload); // 假設你的 ModelRequest 有這個方法
            }
            // 2. 直接呼叫底層 chat 取得 ModelResponse
            openai_spec::ModelResponse response = model_adapter_->chat(req);
            // 3. 直接回傳 ModelResponse 給上層印出或處理
            return response;
        }

    public:
        explicit PlanExecuteStrategy(std::shared_ptr<aiagent::IModelAdapter> adapter)
            : model_adapter_(std::move(adapter))
        {
        }

        openai_spec::ModelResponse execute(aiagent::AgentContext& context) override
        {
            int max_retry = model_adapter_->get_max_retry();
            int i = 0;

            openai_spec::ModelResponse model_response = directCall(context);

            auto invocations = AIResponseHandler::parseToolCalls(model_response);
            if (!invocations || invocations->empty()) {
                spdlog::info("CoT finished: no more tool calls.");
                return model_response;
            }

            for (const auto& inv : *invocations) {
                std::string result = ToolRegistry::getInstance().executeTool(inv.tool, inv.arguments);
                openai_spec::Message tool_msg;
                tool_msg.set_role("tool").set_content(result);
                // 若有欄位就補上：.set_tool_call_id(inv.id) / .set_name(inv.name)
                context.addChatHistory(tool_msg);
            }

            while (i < max_retry || !model_response.is_final())
            {
                model_response = directCall(context);
                ++i;
            }
            return model_response;
        }
    };
}

#endif //LIYONG_OLLAMA_AGENT_TOTSTRATEGY_HPP
