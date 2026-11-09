//
// Created by root on 2026/10/22.
//

#ifndef LIYONG_OLLAMA_AGENT_AGENTCONTEXT_HPP
#define LIYONG_OLLAMA_AGENT_AGENTCONTEXT_HPP

#include <string>
#include <utility>
#include <vector>

#include "OpenAISpec.hpp"

namespace aiagent {
    class AgentContext {
    private:
        bool is_final_ = false;
        int max_turns_ = 5;
        std::string sys_input_; // sys 當前輸入
        std::string user_input_; // user 當前輸入
        std::vector<std::string> ai_resp_hist_; // 使用者當前輸入
        std::vector<openai_spec::Message> chat_history_; // 過去的對話歷史
        int available_tool_count_ = 0; // 目前能用的工具數量

    public:
        // --- Getters (加上 const 與 const 參考，避免不必要的複製) ---
        [[nodiscard]] const std::string &getUserInput() const {
            return user_input_;
        }

        [[nodiscard]] const std::vector<openai_spec::Message> &getChatHistory() const {
            return chat_history_;
        }

        [[nodiscard]] const std::string &getSysInput() const {
            return sys_input_;
        }

        [[nodiscard]] int getAvailableToolCount() const {
            return available_tool_count_;
        }

        [[nodiscard]] int getMaxRetry() const {
            return max_turns_;
        }

         void addChatHistory(const openai_spec::Message &chat) {
            chat_history_.emplace_back(chat);
        }

        void setAiRespHist(const std::string &chat) {
            ai_resp_hist_.emplace_back(chat);
        }

        // --- Setters ---
        void setMaxTurns(int maxRetry) {
            max_turns_ = maxRetry; // 使用 move 語意優效能
        }
        // --- Setters ---
        void setSysInput(std::string input) {
            sys_input_ = std::move(input); // 使用 move 語意優效能
        }

        // --- Setters ---
        void setUserInput(std::string input) {
            user_input_ = std::move(input); // 使用 move 語意優效能
        }

        void setAvailableToolCount(int count) {
            available_tool_count_ = count;
        }
    };
}

#endif //LIYONG_OLLAMA_AGENT_AGENTCONTEXT_HPP
