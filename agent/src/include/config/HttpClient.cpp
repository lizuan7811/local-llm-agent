//
// Created by root on 2026/10/19.
//

#include "HttpClient.hpp"

#include <stdexcept>

namespace aiagent {
    namespace {
        size_t writeCallback(char *data,
                             size_t size,
                             size_t nmemb,
                             void *userp) {
            const size_t total = size * nmemb;
            auto *response = static_cast<std::string *>(userp);
            response->append(data, total);
            return total;
        }
    }

    HttpClient::HttpClient(long connect_timeout_ms, long request_timeout_ms)
        : connect_timeout_ms_(connect_timeout_ms),
          request_timeout_ms_(request_timeout_ms) {
        curl_ = curl_easy_init();
        if (!curl_) {
            throw std::runtime_error("curl_easy_init failed");
        }
    }

    HttpClient::~HttpClient() {
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
    }

    std::string HttpClient::postJson(
        const std::string &url,
        const std::string &body) const {
        curl_easy_reset(curl_);

        std::string responseBody;

        struct curl_slist *headers = nullptr;

        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_POST, 1L);
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &responseBody);

        // 套用全域/配置傳入的逾時設定
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms_);
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, request_timeout_ms_);

        const CURLcode result = curl_easy_perform(curl_);
        long httpStatus = 0;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &httpStatus);
        curl_slist_free_all(headers);

        if (result != CURLE_OK) {
            throw std::runtime_error(
                std::string("HTTP request failed: ") + curl_easy_strerror(result));
        }

        if (httpStatus < 200 || httpStatus >= 300) {
            throw std::runtime_error(
                std::string("HTTP status code ") + std::to_string(httpStatus) + ", response: " + responseBody);
        }
        return responseBody;
    }
}
