#ifndef LIYONG_OLLAMA_AGENT_GEMINIHTTPCLIENTSERVICE_HPP
#define LIYONG_OLLAMA_AGENT_GEMINIHTTPCLIENTSERVICE_HPP

#include <format>
#include <string>
#include "httplib.h"
#include "config/EnvironConfig.hpp"

class GeminiHttpClientService {
private:
    // 1. 將 client_ 宣告移到最上方，確保初始化順序正確
    httplib::Client client_;

public:
    std::string model;
    std::string api_key;
    std::string api_path;
    std::string base_url; // 建議改名為 base_url 比較清楚
    bool stream;
    int max_retries;

    GeminiHttpClientService(const EnvironConfig &environConfig)
        : client_(environConfig.ai_client.gemini.base_url),
          // 注意：這裡應該只要主機根目錄 (例如 https://generativelanguage.googleapis.com)
          model(environConfig.ai_client.gemini.model),
          stream(environConfig.ai_client.gemini.stream),
          api_key(environConfig.ai_client.gemini.api_key),
          base_url(environConfig.ai_client.gemini.base_url) {
        // 如果你的環境支援 HTTPS (SSL)，記得要啟用 cpp-httplib 的 SSL 支援
        // #define CPPHTTPLIB_OPENSSL_SUPPORT

        api_path = environConfig.ai_client.gemini.api_path;

        // 設定 Connection & Request Timeout (單位：秒, 微秒)
        int connect_sec = environConfig.ai_client.gemini.connect_timeout_ms / 1000;
        int request_sec = environConfig.ai_client.gemini.request_timeout_ms / 1000;

        client_.set_connection_timeout(connect_sec, 0);
        client_.set_read_timeout(request_sec, 0);
        client_.set_write_timeout(request_sec, 0);
        client_.set_keep_alive(true); // 啟用 HTTP Keep-Alive
    }

    // 提供一個方便發送請求的方法範例
    // 路徑會像是： /v1beta/models/gemini-2.0-flash:generateContent
    httplib::Result post_content(const std::string &path, const std::string &body, const httplib::Headers &headers) {
        return client_.Post(path, headers, body, "application/json");
    }
};

#endif //LIYONG_OLLAMA_AGENT_GEMINIHTTPCLIENTSERVICE_HPP
