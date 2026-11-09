cmake_minimum_required(VERSION 3.24)
project(liyong_ollama_agent CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 有安裝 ccache 就自動使用,重編譯會快很多
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(CURL REQUIRED)

set(SRC_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROTO_DIR  "${SRC_DIR}/proto")
set(PROTO_FILE "${PROTO_DIR}/agent.proto")

# ---- 1. 生成檔(放在 source 的 proto 資料夾,維持原本 include 路徑不變) ----
set(GENERATED_PROTO_H  "${PROTO_DIR}/agent.pb.h")
set(GENERATED_PROTO_CC "${PROTO_DIR}/agent.pb.cc")
set(GENERATED_GRPC_H   "${PROTO_DIR}/agent.grpc.pb.h")
set(GENERATED_GRPC_CC  "${PROTO_DIR}/agent.grpc.pb.cc"
        src/service/FlowService.cpp
        src/service/FlowService.hpp)
set(GENERATED_FILES
        ${GENERATED_PROTO_H} ${GENERATED_PROTO_CC}
        ${GENERATED_GRPC_H}  ${GENERATED_GRPC_CC})

if(TARGET protobuf::protoc)
    set(PROTOC_CMD $<TARGET_FILE:protobuf::protoc>)
elseif(PROTOBUF_PROTOC_EXECUTABLE)
    set(PROTOC_CMD ${PROTOBUF_PROTOC_EXECUTABLE})
else()
    find_program(PROTOC_CMD protoc REQUIRED)
endif()

if(TARGET gRPC::grpc_cpp_plugin)
    set(GRPC_PLUGIN_CMD $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
else()
    find_program(GRPC_PLUGIN_CMD grpc_cpp_plugin REQUIRED)
endif()

# ---- 2. 只有 agent.proto 變動時才重新產生 ----
# (原本的 add_custom_target 每次建構都會刪除並重生,導致 pb.cc 每次都重編)
add_custom_command(
        OUTPUT ${GENERATED_FILES}
        COMMAND ${PROTOC_CMD}
        --cpp_out=${SRC_DIR}
        --grpc_out=${SRC_DIR}
        --plugin=protoc-gen-grpc=${GRPC_PLUGIN_CMD}
        -I ${SRC_DIR}
        ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating protobuf/gRPC sources from agent.proto"
        VERBATIM
)

# 給 Makefile 的 `make proto` 使用: 只有 agent.proto 比生成檔新才會真的重生
add_custom_target(generate_proto DEPENDS ${GENERATED_FILES})

# ---- 3. 第三方套件 ----
# 下載內容放在專案目錄的 .deps,不會因為刪除 build 目錄或
# CLion 的 "Reset Cache and Reload" 而被清掉 (記得把 .deps 加進 .gitignore)
# 用一般變數(不是 CACHE),並且在 include(FetchContent) 之前設定,
# 這樣就算舊的 CMakeCache.txt 裡已經記了 _deps 路徑也會被蓋過去
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
message(STATUS "FetchContent base dir: ${FETCHCONTENT_BASE_DIR}")

include(FetchContent)

# 已下載過就不要每次 configure 都連 GitHub 檢查更新
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)

# 避免 spdlog 建成 shared lib
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)

# 先找系統已安裝的版本 (apt install nlohmann-json3-dev libspdlog-dev),
# 找不到才從 GitHub 下載 (淺層 clone,只抓該 tag)。
# 這裡不用 FetchContent 的 FIND_PACKAGE_ARGS: 它會以依賴名稱 "json" 當 package 名稱,
# 跟 Debian/Ubuntu 的 nlohmann_jsonConfig.cmake 名稱對不上,會跳警告並誤判找不到。
set(FETCH_LIST "")

find_package(nlohmann_json 3.10 QUIET)
if(nlohmann_json_FOUND)
    message(STATUS "Using system nlohmann_json ${nlohmann_json_VERSION}")
else()
    message(STATUS "System nlohmann_json not found, will fetch from GitHub")
    FetchContent_Declare(
            json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG        v3.11.3
            GIT_SHALLOW    TRUE
            GIT_PROGRESS   TRUE
    )
    list(APPEND FETCH_LIST json)
endif()

find_package(spdlog 1.9 QUIET)
if(spdlog_FOUND)
    message(STATUS "Using system spdlog ${spdlog_VERSION}")
else()
    message(STATUS "System spdlog not found, will fetch from GitHub")
    FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG        v1.12.0
            GIT_SHALLOW    TRUE
            GIT_PROGRESS   TRUE
    )
    list(APPEND FETCH_LIST spdlog)
endif()

# cpp-httplib 一律下載 (固定版本)
FetchContent_Declare(
        cpp-httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG        v0.15.3
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
)
list(APPEND FETCH_LIST cpp-httplib)

FetchContent_MakeAvailable(${FETCH_LIST})

# ---- 4. 執行檔 ----
add_executable(agent_server
        src/main.cpp
        src/service/GRPCAgentServiceImpl.cpp
        src/service/OlmHttpClientService.cpp
        src/include/manager/ConfigManager.cpp
        src/include/manager/LogManager.cpp
        src/service/GeminiHttpClientService.cpp
        src/include/manager/PromptManager.cpp
        src/include/manager/ControlManager.cpp
        src/include/handler/AIResponseHandler.cpp
        src/service/WorkspaceSandbox.cpp
        src/include/config/HttpClient.cpp
        src/include/register/ToolRegistry.cpp
        src/service/AiAgentService.cpp
#        src/include/config/ToolDefinition.cpp
        src/service/ReActAgent.cpp
        src/include/adapter/ModelAdapter.cpp
        src/include/adapter/OllamaAdapter.cpp
        src/service/ReActAgent.cpp
        src/include/strategy/DirectAnswerStrategy.cpp
        src/include/strategy/ReActStrategy.cpp
        src/include/strategy/PlanExecuteStrategy.cpp
        src/model/AgentContext.cpp
        src/include/tools/CreateFileTool.cpp
        ${GENERATED_PROTO_CC}
        ${GENERATED_GRPC_CC}
        src/include/register/PromptRegistry.cpp
        src/include/register/PromptRegistry.hpp
        src/include/strategy/StructuredOutputStrategy.cpp
        src/include/strategy/StructuredOutputStrategy.hpp
)

target_link_libraries(agent_server PRIVATE
        protobuf::libprotobuf
        gRPC::grpc++
        httplib::httplib
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        CURL::libcurl
)

target_include_directories(agent_server PRIVATE
        ${SRC_DIR}
        ${SRC_DIR}/include
)

# ---- 5. 複製 resource 到 build 輸出目錄(資料夾存在才複製) ----
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resource")
    # 當 agent_server 編譯完成後（POST_BUILD），自動建立符號連結
    add_custom_command(TARGET agent_server POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_CURRENT_SOURCE_DIR}/resource"
            "$<TARGET_FILE_DIR:agent_server>/resource"
            COMMENT "Symlinking resource directory to build output directory"
    )
else()
    message(WARNING "resource directory not found, skip copying")
endif()

# ==========================================
# 自動將根目錄的 prompt 與 workspace 連結到編譯輸出目錄
# ==========================================

# 1. 確保輸出目錄（通常是 bin 或根建置目錄）存在

# 2. 建立到上一層 prompt 的符號連結
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../prompts")
    # 當 agent_server 編譯完成後（POST_BUILD），自動建立符號連結
    add_custom_command(TARGET agent_server POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_CURRENT_SOURCE_DIR}/../prompts"
            "$<TARGET_FILE_DIR:agent_server>/prompts"
            COMMENT "Symlinking prompt directory to build output directory"
    )
else()
    message(WARNING "prompt directory not found, skip copying")
endif()

# 3. 建立到上一層 workspace 的符號連結
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../workspace")
    # 當 agent_server 編譯完成後（POST_BUILD），自動建立符號連結
    add_custom_command(TARGET agent_server POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_CURRENT_SOURCE_DIR}/../workspace"
            "$<TARGET_FILE_DIR:agent_server>/workspace"
            COMMENT "Symlinking workspace directory to build output directory"
    )
else()
    message(WARNING "workspace directory not found, skip copying")
endif()