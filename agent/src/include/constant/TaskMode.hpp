//
// Created by root on 2026/10/22.
//

#ifndef LIYONG_OLLAMA_AGENT_TASKCOMPLEXITY_HPP
#define LIYONG_OLLAMA_AGENT_TASKCOMPLEXITY_HPP
#include "strategy/IReasoningStrategy.hpp"

namespace airoute
{
    enum class TaskMode
    {
        DIRECT_ANSWER,
        PLAN_AND_EXECUTE,
        REACT,
        STRUCTURED_OUTPUT
    };

    class AgentContext
    {
    private:
        std::unique_ptr<IReasoningStrategy> ireasoningStrategy_;
        std::string agentPrompt_;

    public:
        AgentContext(std::unique_ptr<IReasoningStrategy> ireasoningStrategy, std::string agentPrompt);
        TaskMode getTaskComplexity();
        std::string getTaskPrompt();

        void setTaskComplexity(std::unique_ptr<IReasoningStrategy> ireasoningStrategy)
        {
            this->ireasoningStrategy_ = std::move(ireasoningStrategy);
        }

        void setTaskPrompt(const std::string& agentPrompt)
        {
            this->agentPrompt_ = agentPrompt;
        }
    };
}
#endif //LIYONG_OLLAMA_AGENT_TASKCOMPLEXITY_HPP
