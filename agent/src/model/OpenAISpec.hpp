#pragma once
#include <string>
#include <vector>
#include <optional>
#include <grpcpp/support/config.h>
#include <nlohmann/json.hpp>

#include "spdlog/fmt/bundled/core.h"
#include "spdlog/fmt/bundled/ranges.h"

using json = nlohmann::json;

namespace openai_spec
{
    class FunctionCall
    {
    private:
        std::string name_;
        std::string arguments_;

    public:
        FunctionCall() = default;

        const std::string& get_name() const { return name_; }
        const std::string& get_arguments() const { return arguments_; }

        friend void to_json(json& j, const FunctionCall& fc)
        {
            j = json{
                {"name", fc.name_},
                {"arguments", fc.arguments_}
            };
        }

        friend void from_json(const json& j, FunctionCall& fc)
        {
            fc.name_ = j.value("name", "");
            if (j.contains("arguments"))
            {
                if (j["arguments"].is_string())
                {
                    fc.arguments_ = j["arguments"].get<std::string>();
                }
                else
                {
                    fc.arguments_ = j["arguments"].dump();
                }
            }
        }
    };

    class ToolCall
    {
    private:
        std::string id_;
        std::string type_ = "function";
        FunctionCall function_;

    public:
        ToolCall() = default;

        const std::string& get_id() const { return id_; }
        const std::string& get_type() const { return type_; }
        const FunctionCall& get_function() const { return function_; }

        friend void from_json(const json& j, ToolCall& t)
        {
            t.id_ = j.value("id", ""); // 修正：id 是 string
            t.type_ = j.value("type", "function");

            if (j.contains("function"))
            {
                t.function_ = j["function"].get<FunctionCall>();
            }
        }

        friend void to_json(json& j, const ToolCall& t)
        {
            j = json{
                {"id", t.get_id()},
                {"type", t.get_type()},
                {"function", t.get_function()}
            };
        }
    };

    // --- 訊息類別 ---
    class Message
    {
    private:
        std::string role_;
        std::string content_;
        std::optional<std::vector<ToolCall>> tool_calls_;
        std::optional<std::string> tool_call_id_;

    public:
        Message() = default;

        Message(std::string role, std::string content)
            : role_(std::move(role)), content_(std::move(content))
        {
        }

        const std::string& get_role() const { return role_; }
        const std::string& get_content() const { return content_; }
        const std::optional<std::vector<ToolCall>>& get_tool_calls() const { return tool_calls_; }
        const std::optional<std::string>& get_tool_call_id() const { return tool_call_id_; }

        Message& set_role(const std::string& role)
        {
            role_ = role;
            return *this;
        }

        Message& set_content(const std::string& content)
        {
            content_ = content;
            return *this;
        }

        Message& set_tool_calls(const std::vector<ToolCall>& calls)
        {
            tool_calls_ = calls; // 修正：加上底線
            return *this;
        }

        Message& set_tool_call_id(const std::string& id)
        {
            tool_call_id_ = id; // 修正：加上底線
            return *this;
        }

        friend void to_json(json& j, const Message& m)
        {
            j = json{{"role", m.role_}};
            if (!m.content_.empty())
            {
                j["content"] = m.content_;
            }
            else
            {
                j["content"] = nullptr;
            }

            if (m.tool_calls_.has_value())
            {
                j["tool_calls"] = m.tool_calls_.value();
            }
            if (m.tool_call_id_.has_value())
            {
                j["tool_call_id"] = m.tool_call_id_.value();
            }
        }

        friend void from_json(const json& j, Message& m)
        {
            m.role_ = j.value("role", "");

            if (j.contains("content") && !j["content"].is_null())
            {
                m.content_ = j["content"].get<std::string>();
            }
            else
            {
                m.content_ = "";
            }

            if (j.contains("tool_calls") && !j["tool_calls"].is_null())
            {
                m.tool_calls_ = j["tool_calls"].get<std::vector<ToolCall>>();
            }

            if (j.contains("tool_call_id") && !j["tool_call_id"].is_null())
            {
                m.tool_call_id_ = j["tool_call_id"].get<std::string>();
            }
        }
    };

    class FunctionDef
    {
    private:
        std::string name_;
        std::string description_;
        json parameters_;

    public:
        FunctionDef() = default;

        FunctionDef(std::string name, std::string desc, json params)
            : name_(std::move(name)), description_(std::move(desc)), parameters_(std::move(params))
        {
        }

        friend void to_json(json& j, const FunctionDef& f)
        {
            j = json{
                {"name", f.name_},
                {"description", f.description_},
                {"parameters", f.parameters_}
            };
        }

        friend void from_json(const json& j, FunctionDef& f)
        {
            f.name_ = j.value("name", "");
            f.description_ = j.value("description", "");
            if (j.contains("parameters"))
            {
                f.parameters_ = j["parameters"];
            }
        }
    };

    class Tool
    {
    private:
        std::string type_ = "function";
        FunctionDef function_;

    public:
        Tool() = default;

        explicit Tool(FunctionDef func) : function_(std::move(func))
        {
        }

        friend void to_json(json& j, const Tool& t)
        {
            j = json{
                {"type", t.type_},
                {"function", t.function_} // 修正：拼字錯誤與變數存取
            };
        }

        friend void from_json(const json& j, Tool& t)
        {
            t.type_ = j.value("type", "function");
            if (j.contains("function"))
            {
                t.function_ = j["function"].get<FunctionDef>();
            }
        }
    };

    // --- 請求規格 ---
    class ModelRequest
    {
    private:
        std::string model_;
        std::vector<Message> messages_;
        bool stream_ = false;
        std::optional<double> temperature_;
        std::vector<Tool> tools_;
        std::optional<std::string> tool_choice_;
        int max_tokens_=4096;

    public:
        ModelRequest() = default;

        ModelRequest& set_model(const std::string& model)
        {
            model_ = model;
            return *this;
        }

        ModelRequest& set_stream(bool stream)
        {
            stream_ = stream;
            return *this;
        }

        ModelRequest& set_temperature(double temperature)
        {
            temperature_ = temperature;
            return *this;
        }

        ModelRequest& set_max_tokens(int max_tokens)
        {
            max_tokens_ = max_tokens;
            return *this;
        }

        ModelRequest& add_message(const Message& msg)
        {
            messages_.push_back(msg);
            return *this;
        }

        ModelRequest& set_tools(const nlohmann::json& tools)
        {
            tools_ = std::move(tools);
            return *this;
        }

        ModelRequest& add_tool(const Tool& tool)
        {
            tools_.push_back(tool);
            return *this;
        }

        ModelRequest& set_tool_choice(const std::string& choice)
        {
            tool_choice_ = choice;
            return *this;
        }

        friend void to_json(json& j, const ModelRequest& req)
        {
            j = json{
                {"model", req.model_},
                {"messages", req.messages_},
                {"stream", req.stream_},
                {"max_tokens", req.max_tokens_}
            };
            if (req.temperature_.has_value())
            {
                j["temperature"] = req.temperature_.value();
            }
            if (!req.tools_.empty())
            {
                j["tools"] = req.tools_; // 修正：改回複數 tools
            }
            if (req.tool_choice_.has_value())
            {
                j["tool_choice"] = req.tool_choice_.value();
            }
        }
    };

    // --- 回應 Choice 類別 ---
    class Choice
    {
    private:
        int index_ = 0;
        Message message_;
        std::string finish_reason_;

    public:
        Choice() = default;

        int get_index() const { return index_; }
        const Message& get_message() const { return message_; }
        const std::string& get_finish_reason() const { return finish_reason_; }

        friend void from_json(const json& j, Choice& c)
        {
            c.index_ = j.value("index", 0);
            if (j.contains("message"))
            {
                c.message_ = j["message"].get<Message>();
            }
            c.finish_reason_ = j.value("finish_reason", "");
        }
    };

    // --- Token 消耗統計 ---
    class Usage
    {
    private:
        int prompt_tokens_ = 0;
        int completion_tokens_ = 0;
        int total_tokens_ = 0;

    public:
        Usage() = default;

        int get_prompt_tokens() const { return prompt_tokens_; }
        int get_completion_tokens() const { return completion_tokens_; }
        int get_total_tokens() const { return total_tokens_; }

        friend void from_json(const json& j, Usage& u)
        {
            u.prompt_tokens_ = j.value("prompt_tokens", 0);
            u.completion_tokens_ = j.value("completion_tokens", 0);
            u.total_tokens_ = j.value("total_tokens", 0);
        }
    };

    // --- 回應規格類別 ---
    class ModelResponse
    {
    private:
        std::string id_;
        std::string object_;
        long long created_ = 0;
        std::string model_;
        bool is_final_ = false;
        std::vector<Choice> choices_;
        Usage usage_;

    public:
        ModelResponse() = default;

        const std::string& get_id() const { return id_; }
        const std::string& get_model() const { return model_; }
        const std::vector<Choice>& get_choices() const { return choices_; }
        const Usage& get_usage() const { return usage_; }
        bool is_final() const { return is_final_; }

        friend void from_json(const json& j, ModelResponse& r)
        {
            r.id_ = j.value("id", "");
            r.object_ = j.value("object", "");
            r.created_ = j.value("created", 0LL);
            r.model_ = j.value("model", "");
            r.is_final_ = j.value("is_final", false);
            if (j.contains("choices"))
            {
                r.choices_ = j["choices"].get<std::vector<Choice>>();
            }
            if (j.contains("usage"))
            {
                r.usage_ = j["usage"].get<Usage>();
            }
        }
    };
} // namespace openai_spec
