//
// Created by root on 2026/10/12.
//

#ifndef LIYONG_OLLAMA_AGENT_PROMPTMANAGER_HPP
#define LIYONG_OLLAMA_AGENT_PROMPTMANAGER_HPP

#include <string>
#include <shared_mutex> // 記得引入這個

class PromptManager {
public:
    explicit PromptManager(const std::string &prompt_path,
                             const std::string &prompt_header_name,
                             const std::string &prompt_content_name);

    PromptManager(const PromptManager &) = delete;

    PromptManager &operator=(const PromptManager &) = delete;

    void reload();

    std::string get_header_prompt() const; // 記得補上這個剛才有提到的取得方法

    std::string get_content_prompt() const; // 記得補上這個剛才有提到的取得方法

    std::string get_header_promt_path() const;

    std::string get_content_promt_path() const;

private:
    std::string prompt_header_path_;
    std::string prompt_content_path_;

    // Avoid thread race conditions.
    mutable std::shared_mutex mutex_; // 修正：拿掉多出來的 t

    std::string cached_header_;
    std::string cached_content_;
};


#endif //LIYONG_OLLAMA_AGENT_PROMPTMANAGER_HPP
