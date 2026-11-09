//
// Created by root on 2026/10/22.
//

#ifndef LIYONG_OLLAMA_AGENT_DIRECTSTRATEGY_HPP
#define LIYONG_OLLAMA_AGENT_DIRECTSTRATEGY_HPP
#include "IReasoningStrategy.hpp"
#include "../adapter/IModelAdapter.hpp"
#include "../register/ToolRegistry.hpp"


/**
 * 直接發送請求，快速取得回應。
 */

namespace aiagent {
    class DirectAnswerStrategy : public IReasoningStrategy {
    private:
        std::shared_ptr<IModelAdapter> model_adapter_;

    public:
        explicit DirectAnswerStrategy(std::shared_ptr<IModelAdapter> adapter)
            : model_adapter_(std::move(adapter)) {
        }

        openai_spec::ModelResponse execute(AgentContext &context) override {
            // 1. 組裝標準的 Request
            openai_spec::ModelRequest req;
            req.set_model(model_adapter_->get_model()); // 使用 adapter 內部的 model 名稱
            req.set_stream(false);
            // 從 Singleton 拿取所有註冊工具的 JSON 結構，並塞進 Request 裡！
            nlohmann::json tools_payload = ToolRegistry::getInstance().getToolsPayload();
            if (!tools_payload.empty()) {
                req.set_tools(tools_payload); // 假設你的 ModelRequest 有這個方法
            }
            openai_spec::Message user_message("user", context.getUserInput());
            req.add_message(user_message);

            // 2. 直接呼叫底層 chat 取得 ModelResponse
            openai_spec::ModelResponse response = model_adapter_->chat(req);

            // 3. 直接回傳 ModelResponse 給上層印出或處理
            return response;
        }
    };
}

#endif //LIYONG_OLLAMA_AGENT_DIRECTSTRATEGY_HPP
