//
// Created by root on 2026/10/19.
//

#pragma once
#ifndef LIYONG_OLLAMA_AGENT_HTTPCLIENT_HPP
#define LIYONG_OLLAMA_AGENT_HTTPCLIENT_HPP
#include <mutex>
#include <string>
#include <curl/curl.h>

namespace aiagent {
    class HttpClient {
    public:
        // 建構子接收連線與請求逾時設定 (毫秒)
        HttpClient(long connect_timeout_ms = 5000, long request_timeout_ms = 120000);

        ~HttpClient();

        HttpClient(const HttpClient &) = delete;

        HttpClient &operator=(const HttpClient &) = delete;

        std::string postJson(const std::string &url, const std::string &body) const;

    private:
        CURL *curl_;
        long connect_timeout_ms_;
        long request_timeout_ms_;
        std::mutex mutex_;
    };
} // namespace aiagent

#endif // LIYONG_OLLAMA_AGENT_HTTPCLIENT_HPP