//
// Created by root on 2026/10/8.
//

#ifndef LIYONG_OLLAMA_AGENT_OLMHTTPCLIENTSERVICE_HPP
#define LIYONG_OLLAMA_AGENT_OLMHTTPCLIENTSERVICE_HPP

#include "../include/adapter/IModelAdapter.hpp"
#include "config/EnvironConfig.hpp"
#include "config/HttpClient.hpp"
#include "model/OllamaSpec.hpp" // 1. 引入你的 Spec
#include "model/OpenAISpec.hpp"

class OlmHttpClientService : public aiagent::IModelAdapter {
public:
    std::string model;
    std::string url;
    bool stream;
    int max_retries;
    std::string native_api_path;
    std::string compatibility_api_path;

    // 使用建構子初始化清單 (Member Initializer List) - 完全保持不變！
    OlmHttpClientService(const EnvironConfig &environ_config)
        : client_(environ_config.ollama_client.connect_timeout_ms, environ_config.ollama_client.request_timeout_ms),
          url(environ_config.ollama_client.url),
          stream(environ_config.ollama_client.stream),
          model(environ_config.ollama_client.model),
          native_api_path(environ_config.ollama_client.api_path.native),
          max_retries(environ_config.ollama_client.max_turns),
          compatibility_api_path(environ_config.ollama_client.api_path.compatibility) {
    }

    // 2. 將回傳型別改為 ChatCompletionResponse
    openai_spec::ModelResponse callOllama(const std::string &content);

    // 2. 將回傳型別改為 ChatCompletionResponse
    openai_spec::ModelResponse callOllamaToolCall(const std::string &headerPrompt, const std::string &contentPrompt);

    openai_spec::ModelResponse chat(const openai_spec::ModelRequest &request) override;

    [[nodiscard]] const std::string &get_model() const override {
        return this->model;
    }

    [[nodiscard]] const int &get_max_retry() const override {
        return this->max_retries;
    }

private:
    aiagent::HttpClient client_;
};

#endif //LIYONG_OLLAMA_AGENT_OLMHTTPCLIENTSERVICE_HPP
