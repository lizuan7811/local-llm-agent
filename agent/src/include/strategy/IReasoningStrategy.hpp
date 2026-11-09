//
// Created by root on 2026/10/22.
//

#ifndef LIYONG_OLLAMA_AGENT_IREASONINGSTRATEGY_HPP
#define LIYONG_OLLAMA_AGENT_IREASONINGSTRATEGY_HPP
#include "model/AgentContext.hpp"
#include "model/OpenAISpec.hpp"

class IReasoningStrategy {
public:
    virtual ~IReasoningStrategy() = default;

    virtual openai_spec::ModelResponse execute(aiagent::AgentContext &context) = 0;
};
#endif //LIYONG_OLLAMA_AGENT_IREASONINGSTRATEGY_HPP
