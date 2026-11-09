// #pragma once
// #include <string>
// #include <vector>
// #include <optional>
// #include <nlohmann/json.hpp>
//
// using json = nlohmann::json;
//
// namespace ollama_spec
// {
//     // --- 訊息類別 ---
//     class Message
//     {
//     private:
//         std::string role_;
//         std::string content_;
//         nlohmann::json tool_calls_;
//
//     public:
//         Message() = default;
//
//         Message(std::string role, std::string content)
//             : role_(std::move(role)), content_(std::move(content))
//         {
//         }
//
//         const std::string& get_role() const { return role_; }
//         const std::string& get_content() const { return content_; }
//
//         void set_role(const std::string& role) { role_ = role; }
//         void set_content(const std::string& content) { content_ = content; }
//
//         bool has_tool_calls() const
//         {
//             return !tool_calls_.is_null() && !tool_calls_.empty();
//         }
//
//         const nlohmann::json& get_tool_calls() const { return tool_calls_; }
//
//         // 允許自訂 to_json / from_json 存取私有成員
//         friend void to_json(json& j, const Message& m)
//         {
//             j = json{{"role", m.role_}, {"content", m.content_}};
//             if (m.has_tool_calls())
//             {
//                 j["tool_calls"] = m.tool_calls_;
//             }
//         }
//
//         friend void from_json(const json& j, Message& m)
//         {
//             m.role_ = j.value("role", "");
//             m.content_ = j.value("content", "");
//             if (j.contains("tool_calls"))
//             {
//                 m.tool_calls_ = j["tool_calls"];
//             }
//         }
//     };
//
//     // --- 請求規格 ---
//     class ChatCompletionRequest
//     {
//     private:
//         std::string model_;
//         std::vector<Message> messages_;
//         bool stream_ = false;
//         std::optional<double> temperature_;
//         nlohmann::json tools_;
//
//     public:
//         ChatCompletionRequest() = default;
//
//         ChatCompletionRequest& set_tools(const nlohmann::json& tools) {
//             tools_ = tools;
//             return *this;
//         }
//
//         ChatCompletionRequest& set_model(const std::string& model)
//         {
//             model_ = model;
//             return *this;
//         }
//
//         ChatCompletionRequest& set_stream(bool stream)
//         {
//             stream_ = stream;
//             return *this;
//         }
//
//         ChatCompletionRequest& add_message(const Message& msg)
//         {
//             messages_.push_back(msg);
//             return *this;
//         }
//
//         friend void to_json(json& j, const ChatCompletionRequest& req)
//         {
//             j = json{
//                 {"model", req.model_},
//                 {"messages", req.messages_},
//                 {"stream", req.stream_}
//             };
//             if (req.temperature_.has_value())
//             {
//                 j["temperature"] = req.temperature_.value();
//             }
//
//             // 【補上這一段】把 tools 序列化進 HTTP Request 中！
//             if (!req.tools_.is_null() && !req.tools_.empty())
//             {
//                 j["tools"] = req.tools_;
//             }
//         }
//     };
//
//     // --- 回應 Choice 類別 ---
//     class Choice
//     {
//     private:
//         int index_ = 0;
//         Message message_;
//         std::string finish_reason_;
//
//     public:
//         int get_index() const { return index_; }
//         const Message& get_message() const { return message_; }
//         const std::string& get_finish_reason() const { return finish_reason_; }
//
//         friend void from_json(const json& j, Choice& c)
//         {
//             c.index_ = j.value("index", 0);
//             if (j.contains("message"))
//             {
//                 c.message_ = j["message"].get<Message>();
//             }
//             c.finish_reason_ = j.value("finish_reason", "");
//         }
//     };
//
//     // --- Token 消耗統計 ---
//     class Usage
//     {
//     private:
//         int prompt_tokens_ = 0;
//         int completion_tokens_ = 0;
//         int total_tokens_ = 0;
//
//     public:
//         int get_prompt_tokens() const { return prompt_tokens_; }
//         int get_completion_tokens() const { return completion_tokens_; }
//         int get_total_tokens() const { return total_tokens_; }
//
//         friend void from_json(const json& j, Usage& u)
//         {
//             u.prompt_tokens_ = j.value("prompt_tokens", 0);
//             u.completion_tokens_ = j.value("completion_tokens", 0);
//             u.total_tokens_ = j.value("total_tokens", 0);
//         }
//     };
//
//     // --- 回應規格類別 ---
//     class ChatCompletionResponse
//     {
//     private:
//         std::string id_;
//         std::string object_;
//         long long created_ = 0;
//         std::string model_;
//         std::vector<Choice> choices_;
//         Usage usage_;
//
//     public:
//         const std::string& get_id() const { return id_; }
//         const std::string& get_model() const { return model_; }
//         const std::vector<Choice>& get_choices() const { return choices_; }
//         const Usage& get_usage() const { return usage_; }
//
//         friend void from_json(const json& j, ChatCompletionResponse& r)
//         {
//             r.id_ = j.value("id", "");
//             r.object_ = j.value("object", "");
//             r.created_ = j.value("created", 0LL);
//             r.model_ = j.value("model", "");
//             if (j.contains("choices"))
//             {
//                 r.choices_ = j["choices"].get<std::vector<Choice>>();
//             }
//             if (j.contains("usage"))
//             {
//                 r.usage_ = j["usage"].get<Usage>();
//             }
//         }
//     };
// } // namespace ollama_spec
