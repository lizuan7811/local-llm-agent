#ifndef LIYONG_OLLAMA_AGENT_GLOBALCONFIG_HPP
#define LIYONG_OLLAMA_AGENT_GLOBALCONFIG_HPP

#include <string>
#include <nlohmann/json.hpp>  // 記得引入 json 庫

class GlobalConfig {
public:
    std::string log_level;
    std::string log_path;
    std::string time_format;
    std::string log_pattern;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GlobalConfig, log_level, log_path, time_format, log_pattern)


class GrpcKeepaliveConfig {
public:
    bool enabled;
    int time_ms;
    int timeout_ms;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GrpcKeepaliveConfig, enabled, time_ms, timeout_ms)


class GrpcTlsConfig {
public:
    bool enabled;
    std::string cert_file;
    std::string key_file;
    std::string ca_file;
    bool verify_client;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GrpcTlsConfig, enabled, cert_file, key_file, ca_file, verify_client)


class GrpcConfig {
public:
    std::string host;
    int port;
    int worker_threads;
    int max_message_size;

    GrpcKeepaliveConfig keepalive;
    GrpcTlsConfig tls;
};

// 巢狀結構也可以直接放進去，nlohmann/json 會自動去對應裡面的屬性
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GrpcConfig, host, port, worker_threads, max_message_size, keepalive, tls)


class ApiPath {
public:
    std::string native;
    std::string compatibility;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ApiPath, native, compatibility)


class OllamaTlsConfig {
public:
    bool enabled;
    bool verify_certificate;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OllamaTlsConfig, enabled, verify_certificate)


class OllamaClientConfig {
public:
    std::string url;
    std::string model;

    int connect_timeout_ms;
    int request_timeout_ms;

    int max_turns;
    int tern_delay_ms;

    std::string keep_alive;
    bool stream;

    OllamaTlsConfig tls;
    ApiPath api_path;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OllamaClientConfig, url, model, connect_timeout_ms, request_timeout_ms, max_turns,
                                   tern_delay_ms, keep_alive, stream, tls, api_path)


class AgentPermissionConfig {
public:
    bool shell;
    bool file_read;
    bool file_write;
    bool file_delete;
    bool git;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AgentPermissionConfig, shell, file_read, file_write, file_delete, git)


class AgentConfig {
public:
    bool enabled;

    std::string workspace;

    int max_iterations;
    int command_timeout_ms;

    AgentPermissionConfig permissions;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AgentConfig, enabled, workspace, max_iterations, command_timeout_ms, permissions)


class AIClient {
public:
    bool enable;
    std::string model;
    std::string base_url;
    std::string api_path;
    std::string api_key;

    int connect_timeout_ms;
    int request_timeout_ms;

    int max_retries;
    int retry_delay_ms;

    std::string keep_alive;
    bool stream;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AIClient, enable, model, base_url, api_path, api_key, connect_timeout_ms,
                                   request_timeout_ms,
                                   max_retries, retry_delay_ms, keep_alive, stream)

class AIClientConfig {
public:
    AIClient gemini;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AIClientConfig, gemini)

class Mission {
public:
    std::string prompts_dir_path;
    std::string prompts_header_name;
    std::string prompts_content_name;
    std::string work_dir_path;
    std::string work_dir_name;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mission, prompts_dir_path, prompts_header_name, prompts_content_name, work_dir_path,
                                   work_dir_name)

class Control {
public:
    std::string socket_path;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Control, socket_path)


class EnvironConfig {
public:
    GlobalConfig global;
    GrpcConfig grpc;
    OllamaClientConfig ollama_client;
    AgentConfig agent;
    AIClientConfig ai_client;
    Mission mission;
    Control control;
};

// 最外層的 EnvironConfig
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EnvironConfig, global, grpc, ollama_client, agent, ai_client, mission, control)

#endif // LIYONG_OLLAMA_AGENT_GLOBALCONFIG_HPP
