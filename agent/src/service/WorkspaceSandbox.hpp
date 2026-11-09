//
// Created by root on 2026/10/16.
//

#ifndef LIYONG_OLLAMA_AGENT_WORKSPACESANDBNOX_HPP
#define LIYONG_OLLAMA_AGENT_WORKSPACESANDBNOX_HPP
#pragma once
#include <filesystem>
#include <optional>

class WorkspaceSandbox {
public:
    explicit WorkspaceSandbox(const std::filesystem::path &root);

    std::optional<std::filesystem::path> resolve(const std::string &relative) const;

    bool write_file(const std::string &relative, const std::string &content, std::string &error) const;

    const std::filesystem::path &root() const;

private:
    std::filesystem::path root_;
};

#endif //LIYONG_OLLAMA_AGENT_WORKSPACESANDBNOX_HPP
