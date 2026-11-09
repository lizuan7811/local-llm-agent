//
// Created by root on 2026/10/16.
//

#include "WorkspaceSandbox.hpp"
#include <fstream>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

WorkspaceSandbox::WorkspaceSandbox(const fs::path &root) {
    fs::create_directories(root);
    root_ = fs::weakly_canonical(fs::absolute(root));
}

std::optional<std::filesystem::path> WorkspaceSandbox::resolve(const std::string &relative) const {
    if (relative.empty()) {
        return std::nullopt;
    }
    fs::path rel(relative);
    if (rel.is_absolute()) {
        return std::nullopt; // 拒絕絕對路徑
    }

    fs::path full = (root_ / rel).lexically_normal();
    fs::path check = full.lexically_relative(root_);

    // 如果 check 為空、或是等於 "."，代表就是 root_ 本身或內部
    // 如果 check 的開頭是 ".."，代表它試圖逃出 root_ 目錄（發生 Path Traversal）
    if (!check.empty() && *check.begin() == "..") {
        return std::nullopt; // ❌ 違規，拒絕存取！
    }

    // 額外保險：確保路徑真的在 root_ 之下（防止其他極端狀況）
    // （如果 lexically_relative 回傳了絕對路徑，通常代表跨了硬碟代號等）
    if (check.is_absolute()) {
        return std::nullopt;
    }

    return full; // ✅ 安全，回傳完整路徑
}

bool WorkspaceSandbox::write_file(const std::string &relative, const std::string &content, std::string &error) const {
    std::optional<std::filesystem::path> full = resolve(relative);
    if (!full) {
        error = "path not allowed (must be relative and inside workspace):" + relative;
        return false;
    }
    std::error_code ec;
    fs::create_directories(full->parent_path(), ec);
    if (ec) {
        error = "cannot create directory: " + ec.message();
        return false;
    }
    std::ofstream out(*full, std::ios::binary);
    if (!out) {
        error = "cannot open file for writing: " + full->string();
        return false;
    }
    out << content;
    spdlog::info("[Sandbox] wrote {} ({} bytes).", full->string(), content.size());
    return true;
}
