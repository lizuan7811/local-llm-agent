//
// Created by root on 2026/10/22.
//

#ifndef LIYONG_OLLAMA_AGENT_MODELROUTER_HPP
#define LIYONG_OLLAMA_AGENT_MODELROUTER_HPP

#include <memory>
#include <string>

#include "constant/TaskMode.hpp"
#include "model/OpenAISpec.hpp"
#include "IModelAdapter.hpp"
#include "strategy/ReActStrategy.hpp"
#include "strategy/DirectAnswerStrategy.hpp"
#include "strategy/IReasoningStrategy.hpp"
#include "strategy/PlanExecuteStrategy.hpp"
#include "util/StringUtils.hpp"

namespace aiagent
{
    class ModelRouter
    {
    private:
        std::shared_ptr<IModelAdapter> model_adapter_; // 持有模型轉接器

    public:
        // 透過建構子注入 ModelAdapter
        explicit ModelRouter(std::shared_ptr<IModelAdapter> adapter)
            : model_adapter_(std::move(adapter))
        {
        }

        std::unique_ptr<IReasoningStrategy> route(const aiagent::AgentContext& agentContext)
        {
            // call llm get which prompt and mode it use
            airoute::AgentContext airouteAgent = routeAgentModeAndPrompt(agentContext);

            // route AgentMode
            airoute::TaskMode complexity = airouteAgent.getTaskComplexity();
        }

    private:
        std::unique_ptr<IReasoningStrategy> mapToTaskComplexity(const std::string& label)
        {
            airoute::TaskMode taskMode = airoute::TaskMode::REACT;

            if (label == "DIRECT_ANSWER") taskMode = airoute::TaskMode::DIRECT_ANSWER;
            if (label == "PLAN_AND_EXECUTE") taskMode = airoute::TaskMode::PLAN_AND_EXECUTE;
            if (label == "STRUCTURED_OUTPUT") taskMode = airoute::TaskMode::STRUCTURED_OUTPUT;

            switch (taskMode)
            {
            case airoute::TaskMode::DIRECT_ANSWER:
                return std::make_unique<DirectAnswerStrategy>(model_adapter_);
            case airoute::TaskMode::PLAN_AND_EXECUTE:
                return std::make_unique<ReActStrategy>(model_adapter_);
            case airoute::TaskMode::STRUCTURED_OUTPUT:
                return std::make_unique<PlanExecuteStrategy>(model_adapter_);
            case airoute::TaskMode::REACT:
                return std::make_unique<ReActStrategy>(model_adapter_);
            }
            return std::make_unique<ReActStrategy>(model_adapter_);
        }

        airoute::AgentContext routeAgentModeAndPrompt(const AgentContext& agentContext)
        {
            if (!model_adapter_)
            {
                return {
                    mapToTaskComplexity(""), ""
                }; // 預設保護
            }

            // 1. 建立給路由分類專用的 Request 結構
            openai_spec::ModelRequest req;

            req.set_model(model_adapter_.get()->get_model()); // 讀取初始化時載入的 model
            req.set_stream(false); // 分類不需要串流，一次拿結果
            req.set_temperature(0.0);
            req.set_max_tokens(8); // 只需要一個單字，限制輸出長度

            // 加入嚴格約束的 System Prompt
            const std::string kRouterPrompt = R"(你是任務路由分類器。依下列標準分類使用者輸入：
Simple：單一步驟即可回答，不需推理或工具。
  例：問候、查單一事實、簡單翻譯、單句改寫。
Complex：需要多步驟推理、整理多項資訊，或撰寫一般程式碼。
  例：比較兩種方案優缺點、寫一個函式、摘要長文。
Expert：需要深度專業知識、系統設計、除錯複雜問題或大量規劃。
  例：設計分散式架構、分析 race condition、撰寫完整模組。

只輸出 Simple、Complex、Expert 其中一個單字，不要加標點、解釋或其他文字。

範例：
輸入：你好 → Simple
輸入：幫我寫一個 C++ 的 quick sort → Complex
輸入：幫我設計支援百萬連線的 gateway 架構 → Expert)";
            openai_spec::Message sys_message(
                "system",
                kRouterPrompt
            );

            openai_spec::Message user_message("user", agentContext.getUserInput());

            req.add_message(sys_message);
            req.add_message(user_message);

            // 2. 呼叫底層 OlmHttpClientService 的 chat 方法
            openai_spec::ModelResponse response = model_adapter_->chat(req);

            // 3. 從 Response 中萃取文字回應
            std::string model_reply;
            try
            {
                const auto& choices = response.get_choices();
                if (!choices.empty())
                {
                    model_reply = choices[0].get_message().get_content();
                }
            }
            catch (std::exception& e)
            {
                model_reply = "";
                spdlog::error(e.what());
            }

            std::string label = StringUtils::trim(model_reply);
            std::unique_ptr<IReasoningStrategy> ireasonStrategy = mapToTaskComplexity(label);

            // 格式不符：記 log 方便調 prompt，然後走保守路徑
            spdlog::warn("router unexpected reply: {}", model_reply);
            return {(std::move(ireasonStrategy)), ""};
        }
    };
}

#endif //LIYONG_OLLAMA_AGENT_MODELROUTER_HPP
