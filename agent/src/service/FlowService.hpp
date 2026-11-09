//
// Created by root on 2026/11/5.
//

#ifndef LIYONG_OLLAMA_AGENT_FLOWSERVICE_HPP
#define LIYONG_OLLAMA_AGENT_FLOWSERVICE_HPP
#include "adapter/ModelAdapter.hpp"
#include "config/EnvironConfig.hpp"
#include "manager/PromptManager.hpp"


class FlowService
{
private:
    EnvironConfig environConfig;
    PromptManager promptManager;

public:
    std::optional<std::string> routeAgentPromptAndMode(const std::string& clientInput);

    std::optional<std::string> process(ModelAdapter modelAdapter, const std::string& clientInput);
};


#endif //LIYONG_OLLAMA_AGENT_FLOWSERVICE_HPP
