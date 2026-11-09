//
// Created by root on 2026/10/12.
//

#ifndef LIYONG_OLLAMA_AGENT_CONTROLMANAGER_HPP
#define LIYONG_OLLAMA_AGENT_CONTROLMANAGER_HPP

#include <string>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include "PromptManager.hpp" // 需要操作 PromptManager

class ControlManager {
public:
    // 建構子：帶入 socket 路徑，並可選擇性綁定 PromptManager
    explicit ControlManager(std::string socket_path, PromptManager &prompt_manager);

    // 預設解構子：確保背景執行緒能安全被 join 或釋放
    ~ControlManager();

    ControlManager(const ControlManager &) = delete;

    ControlManager &operator=(const ControlManager &) = delete;

    // 啟動控制通道的背景監聽執行緒
    bool start();

    // 停止監聽與清理 Socket 檔案
    void stop();

    std::string get_header_prompt() const;

    std::string get_content_prompt() const;

private:
    int server_fd_;
    std::string socket_path_;
    PromptManager &prompt_manager_; // 參照：收到指令時用來重載 Prompt

    std::atomic<bool> is_running_{false};
    std::thread listener_thread_;

    mutable std::shared_mutex mutex_;

    // 背景執行緒的迴圈函式
    void listen_loop();
};

#endif //LIYONG_OLLAMA_AGENT_CONTROLMANAGER_HPP
