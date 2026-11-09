//
// Created by root on 2026/11/5.
//

#ifndef LIYONG_OLLAMA_AGENT_PROMPTREGISTRY_HPP
#define LIYONG_OLLAMA_AGENT_PROMPTREGISTRY_HPP
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

namespace globalprompt
{
    class PromptPropery
    {
    private:
        std::string prompt_code_;
        std::string sys_prompt_content_;
        std::string task_prompt_content;
        std::string sys_prompt_path_;
        std::string task_prompt_path_;

    public:
        PromptPropery(const std::string& code, const std::string& sys_prompt_path,
                      const std::string& task_prompt_path) : prompt_code_(code),
                                                             sys_prompt_path_(sys_prompt_path),
                                                             task_prompt_path_(task_prompt_path)
        {
        }

        const std::string& getSystemPrompt()
        {
            return sys_prompt_path_;
        }
        
        const std::string& getSystemPrompt()
        {
            return sys_prompt_path_;
        }

        void setSystemPrompt(const std::string& prompt)
        {
            this->sys_prompt_path_ = prompt;
        }

        const std::string& getTaskPrompt()
        {
            return task_prompt_path_;
        }

        void setTaskPrompt(const std::string& prompt)
        {
            this->task_prompt_path_ = prompt;
        };
    };

    class PromptRegistry;

    class PromptRegistry
    {
    private:
        std::unordered_map<std::string, std::string> prompts;

    public:
        PromptRegistry();
    };
};

#endif //LIYONG_OLLAMA_AGENT_PROMPTREGISTRY_HPP
