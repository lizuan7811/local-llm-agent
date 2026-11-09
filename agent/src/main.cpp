#include "include/adapter/ModelRouter.hpp"
#include "include/manager/LogManager.hpp"
#include "service/GRPCAgentServiceImpl.hpp"
#include "include/manager/ConfigManager.hpp"
#include "manager/ControlManager.hpp"
#include "manager/PromptManager.hpp"
#include "include/register/ToolRegistry.hpp"
#include "model/OpenAISpec.hpp"
#include "service/AiAgentService.hpp"
#include "service/GeminiHttpClientService.hpp"
#include "service/OlmHttpClientService.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"  // 讓 spdlog 自行決定引入 fmt 的方式

void printConfig(const EnvironConfig& config)
{
    // 2. 把 C++ 結構體直接指定給 nlohmann::json (因為有宣告巨集，它會自動轉換)
    nlohmann::json j = config;

    // 3. 用 .dump(4) 進行 4 格縮排的 Pretty Print 輸出
    std::cout << "[config]\n" << j.dump(4) << std::endl;

    std::cout << "[Info] Starting the application ..." << std::endl;
}

void testGemini(const EnvironConfig& config)
{
    // 2. 初始化你的 Gemini 服務客戶端
    GeminiHttpClientService geminiService(config);

    // 3. 動態組裝 API Path (使用剛才討論的 fmt::runtime)
    std::string pattern = config.ai_client.gemini.api_path;
    std::string api_path = fmt::format(fmt::runtime(pattern), config.ai_client.gemini.model);
    std::cout << "Target API Path: " << api_path << std::endl;

    // 4. 準備 HTTP Headers (Gemini 需要帶 x-goog-api-key)
    httplib::Headers headers = {
        {"x-goog-api-key", geminiService.api_key},
        {"Content-Type", "application/json"}
    };

    // 5. 準備要發送的 JSON 請求內容（Google 原生 generateContent 格式）
    // 註：如果你想直接用你前面寫的 openai_spec 結構，可以透過 to_json 轉成字串
    // 直接用你前面定義好的 json 型態與大括號巢狀結構
    json req_json = {
        {
            "contents", json::array({
                {
                    {
                        "parts", json::array({
                            {{"text", "Hello! Say hi in C++ style."}}
                        })
                    }
                }
            })
        }
    };

    // 如果是用你原本自定義的 json 變數：
    // json req_json = { ... };
    std::string request_body = req_json.dump();

    // 6. 發送 POST 請求
    std::cout << "Sending request to Gemini..." << std::endl;
    httplib::Result res = geminiService.post_content(api_path, request_body, headers);

    // 7. 檢查回應結果
    if (res)
    {
        std::cout << "Status Code: " << res->status << std::endl;
        if (res->status == 200)
        {
            std::cout << "\n--- Response Body ---" << std::endl;
            std::cout << res->body << std::endl;
        }
        else
        {
            std::cerr << "API Error Response:\n" << res->body << std::endl;
        }
    }
    else
    {
        std::cerr << "HTTP request failed! Error code: " << httplib::to_string(res.error()) << std::endl;
    }
}

int main()
{
    // 3. 初始化 Config
    if (!ConfigManager::getInstance().init("resource/config.json"))
    {
        spdlog::error("讀取設定檔失敗！");
        return -1;
    }

    if (!LogManager::initLogConfig())
    {
        return -1;
    }

    // 2. 取得設定
    const EnvironConfig& config = ConfigManager::getInstance().get();

    ToolRegistry& registry = ToolRegistry::getInstance();

    // 1. 建立 PromptManager 與 ControlManager，用智能指標確保生命週期常駐
    std::unique_ptr<PromptManager> prompt_manager = std::make_unique<PromptManager>(
        config.mission.prompts_dir_path, config.mission.prompts_header_name, config.mission.prompts_content_name);
    std::unique_ptr<ControlManager> control_manager = std::make_unique<ControlManager>(
        config.control.socket_path, *prompt_manager);

    // 2. 啟動背景監聽執行緒
    if (!control_manager->start())
    {
        spdlog::error("Failed to start ControlManager.");
        return 1;
    }
    std::string headerPrompt = control_manager->get_header_prompt();
    std::string contentPrompt = control_manager->get_content_prompt();

    OlmHttpClientService olmHttpClientService(config);
    // olmHttpClientService.callOllama("Tell me difference between c++, java and C#");
    // olmHttpClientService.callOllamaToolCall(headerPrompt, contentPrompt);

    auto adapter = std::make_shared<OlmHttpClientService>(config);
    aiagent::AIAgentService service(8, adapter);
    aiagent::AgentContext agentContext;
    agentContext.setMaxTurns(config.ollama_client.max_turns);
    agentContext.setSysInput(headerPrompt);
    agentContext.setUserInput(contentPrompt);

    auto future = service.run_agent_task(agentContext, [](const openai_spec::ModelResponse& response)
    {
        // TODO getResponse name to call function
        std::cout << "收到結果: " << response.get_choices()[0].get_message().get_content() << std::endl;
    });
    // 改用 try-catch 與 .get() 來捕捉背景執行緒的真實錯誤
    try
    {
        future.get(); // 這會等待並同步拋出背景任務中未捕捉的例外
    }
    catch (const std::exception& e)
    {
        spdlog::error("Agent 執行背景任務時發生例外: {}", e.what());
    }

    spdlog::info("Agent Server is running. Control channel active.");

    // 3. 讓主執行緒進入主迴圈或等待（例如 Agent 的核心任務迴圈）
    // 假設你的主程式這裡會持續運作或等待訊號
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "[INFO] 流程執行完畢！" << std::endl;
    // printConfig(config);

    // RunServer(config);

    return 0;
}
