//
// Created by root on 2026/10/12.
//

#include "ControlManager.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

#include "spdlog/spdlog.h" // 修正 4：改用標準 spdlog 標頭檔

// 修正 1 & 2：補上類別名稱與命名空間，且參照不能 std::move
ControlManager::ControlManager(std::string socket_path, PromptManager &prompt_manager)
    : socket_path_(std::move(socket_path)),
      prompt_manager_(prompt_manager),
      server_fd_(-1) {
}

ControlManager::~ControlManager() {
    stop();
}

bool ControlManager::start() {
    unlink(socket_path_.c_str());

    // 修正 3：修正 AF_UNIC -> AF_UNIX, SOCKET_STREAM -> SOCK_STREAM
    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        spdlog::error("[ControlManager] Failed to open socket.");
        return false;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd_, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        spdlog::error("[ControlManager] Failed to bind socket.");
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    if (listen(server_fd_, 5) < 0) {
        spdlog::error("[ControlManager] Failed to listen socket.");
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    is_running_ = true;

    listener_thread_ = std::thread(&ControlManager::listen_loop, this);

    spdlog::info("[ControlManager] listening for connections on: {}", socket_path_);

    return true;
}


void ControlManager::stop() {
    is_running_ = false;
    if (server_fd_ != -1) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }

    if (listener_thread_.joinable()) {
        listener_thread_.join();
    }

    unlink(socket_path_.c_str());
    spdlog::info("[ControlManager] Control channel Stopped.");
}

void ControlManager::listen_loop() {
    while (is_running_) {
        int client_fd = accept(server_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            if (!is_running_) {
                break;
            }
            continue;
        }
        char buffer[1024] = {0};

        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

        if (bytes_read > 0) {
            std::string cmd(buffer, bytes_read);

            auto last_pos = cmd.find_last_not_of("\n\r");
            if (last_pos != std::string::npos) {
                cmd.erase(last_pos + 1);
            } else {
                cmd.clear();
            }

            const char *resp = nullptr;
            if (cmd == "reload") {
                spdlog::info("[ControlManager] Reloading configuration file/prompt.");
                prompt_manager_.reload();
                resp = "OK: Prompt reloaded successfully.\n";
            } else {
                spdlog::warn("[ControlManager] Received unknown command: '{}'", cmd);
                resp = "UNKNOWN COMMAND. Available: reload.\n";
            }

            write(client_fd, resp, std::strlen(resp));
        }
        close(client_fd);
    }
}

std::string ControlManager::get_header_prompt() const {
    return prompt_manager_.get_header_prompt();
}


std::string ControlManager::get_content_prompt() const {
    return prompt_manager_.get_content_prompt();
}
