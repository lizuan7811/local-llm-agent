//
// Created by root on 2026/10/16.
//

#pragma once
#ifndef LIYONG_IMODEL_ADAPTER_HPP
#define LIYONG_IMODEL_ADAPTER_HPP

#include "model/OpenAISpec.hpp"

namespace aiagent {
    class IModelAdapter {
    public:
        virtual ~IModelAdapter() = default;

        virtual const std::string &get_model() const = 0; // 或加上 const

        virtual const int &get_max_retry() const = 0; // 或加上 const

        // 統一採用 OpenAI 規範的 ModelRequest 作為標準輸入
        virtual openai_spec::ModelResponse chat(const openai_spec::ModelRequest &request) = 0;
    };
}

#endif //LIYONG_IMODEL_ADAPTER_HPP
