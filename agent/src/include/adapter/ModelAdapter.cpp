//
// Created by root on 2026/10/19.
//
/**
 * Thought（思考）：大模型分析現在該做什麼。
 * Action（行動）：大模型決定要呼叫哪個工具，或者覺得資料夠了、決定直接輸出最終答案。
 */
#include "ModelAdapter.hpp"

ModelAdapter& ModelAdapter::getInstance()
{
    // Meyers' Singleton: C++11 起區域靜態變數是執行緒安全的 (Thread-safe)
    static ModelAdapter instance;
    return instance;
}

void ModelAdapter::addProperty(const ModelAdapterProperty& property)
{
    // 以 agentCode 作為 key 寫入 unordered_map
    properties[property.getAgentCode()] = property;
}

bool ModelAdapter::getProperty(const std::string& agentCode, ModelAdapterProperty& outProperty) const
{
    auto it = properties.find(agentCode);
    if (it != properties.end()) {
        outProperty = it->second;
        return true;
    }
    return false;
}