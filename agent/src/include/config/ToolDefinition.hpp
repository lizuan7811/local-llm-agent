// //
// // Created by root on 2026/10/20.
// //
//
// #ifndef LIYONG_OLLAMA_AGENT_TOOLDEFINITION_HPP
// #define LIYONG_OLLAMA_AGENT_TOOLDEFINITION_HPP
// #include <string>
//
// #include "nlohmann/json.hpp"
// #include "nlohmann/json_fwd.hpp"
//
//
// class ToolDefinition {
//     std::string type_ = "function";
//     std::string name_;
//     std::string description_;
//     nlohmann::json parameters_;
//
// public:
//     const std::string get_name() const {
//         return name_;
//     }
//
//     void set_name(const std::string &name) {
//         name_ = name;
//     }
//
//     const std::string get_description() const {
//         return description_;
//     }
//
//     void set_description(const std::string &description) {
//         description_ = description;
//     }
//
//     const std::string get_parameters() const {
//         return parameters_;
//     }
//
//     void set_parameters(const std::string &parameters) {
//         parameters_ = parameters;
//     }
//
//     [[nodiscard]] nlohmann::json toJson() const {
//         return {
//             {"type", type_},
//             {
//                 "function", {
//                     {"name", name_},
//                     {"description", description_},
//                     {"parameters", parameters_}
//                 }
//             }
//         };
//     }
// };
//
//
// #endif //LIYONG_OLLAMA_AGENT_TOOLDEFINITION_HPP
