[GitHub Repo Description]
A high-performance C++20 AI Agent framework featuring local LLM integration (Ollama/Gemini), ReAct reasoning loops, dynamic model routing, and sandboxed tool execution.

[README.md Content]
# C++20 Local AI Agent Framework

A high-performance, modular C++20 AI Agent and LLM execution framework designed for local intelligence, secure tool invocation, and multi-step reasoning workflows (powered by Ollama / Gemini).

---

## 🚀 Key Features

* **Modern C++20 Architecture**: Built with clean separation of concerns, leveraging modern C++ standards, smart pointers, and concurrency primitives.
* **Secure Workspace Sandbox (`WorkspaceSandbox`)**: Strict path resolution (`resolveSafePath`) ensuring all file read/write operations remain sandboxed within permitted boundaries.
* **Modular Tool Registry (`ToolRegistry`)**: Dynamic tool registration (File I/O, Directory listing, Command execution) governed by configurable permissions and timeouts.
* **ReAct & Multi-Step Reasoning Loop**: Supports autonomous tool-calling loops (`Agent::run`) with iteration limits, fallback parsers, and result feedback.
* **Flexible Model Router & Adapters**: Decoupled design supporting multiple LLM backends (Ollama, Gemini) with clean strategy pattern abstractions (`ModelRouter`, `ModelAdapter`).
* **Concurrency & Task Management**: Thread-safe task queuing and execution handled via `ControlManager` and worker threads.

---

## 🛠️ System Architecture

### Core Components
```main
├─ ConfigManager ─ EnvironConfig
├─ LogManager
├─ PromptManager ──────────► System Prompt (Dynamic / Reloadable)
├─ ControlManager ─ run/status/cancel ─► TaskQueue
│                                         │
│                           ┌─────────────▼────────────┐
│                           │  AgentRunner (Worker Thread)│
│                           └─────────────┬────────────┘
│                                         ▼
│                                   Agent::run(task)  ← Loop
│                                    │            │
│                              ILLMClient     ToolRegistry
│                           ┌────┴────┐      ┌───┬───┬────┬──────┐
│                         Olm     Gemini   list read write run_cmd
│                                                  └ WorkspaceSandbox
└─ AIResponseHandler ─► FallbackParser (Handles non-tool outputs)
```

### Execution Flow & Strategy
ModelRouter -> call AI get Strategy -> AI Strategy -> ModelAdapter -> Service implement to call Model for result

* **ModelRouter**: Analyzes incoming requests to decide the optimal execution strategy (e.g., Direct Answer, CoT, ReAct).
* **AI Strategy**: Drives multi-step reasoning and execution loops.
* **ModelAdapter**: Manages underlying HTTP communications, payload formatting, and API integrations with LLM providers.

---

## 📋 Agent Roles & Task Modes

The framework supports extensible agent specialization and execution paradigms:

* **Agent Roles (`AgentCode`)**:
    * `CODE_AGENT`: Specialized in C++/Java code reading, compilation, and refactoring (`write_file`, `exec_cmd`).
    * `FINANCE_AGENT`: Data access and stock quote analysis.
    * `SYS_MONITOR_AGENT`: Log analysis and system diagnostics.
    * `GENERAL_AGENT`: General knowledge and conversational tasks.

* **Task Execution Modes (`TaskMode`)**:
    * `REACT`: Dynamic Thought-Action-Observation loop for complex, unpredictable tool usage.
    * `PLAN_AND_EXECUTE`: Global planning followed by sequential execution for multi-step tasks.
    * `DIRECT_ANSWER`: Pure text generation without tool overhead.
    * `STRUCTURED_OUTPUT`: Enforces strict JSON Schema compliance.

---

## ⚙️ Environment Prerequisites

Make sure you have the required development libraries installed (Protobuf, gRPC, CMake, Ninja, etc.):

sudo apt update
sudo apt install -y \
libprotobuf-dev protobuf-compiler \
libgrpc-dev libgrpc++-dev protobuf-compiler-grpc \
libcurl4-openssl-dev ninja-build build-essential cmake

---

## 📦 Compile and Build

Clone the repository and build the project using CMake and Ninja:

# Configure with Ninja
cmake -S . -B cmake-build-debug -G Ninja

# Build the project
cmake --build cmake-build-debug

*(Alternative using Unix Makefiles)*
cmake -S . -B cmake-build-debug -G "Unix Makefiles"
make -C cmake-build-debug

---

## 🗺️ Roadmap & Development Phases

- [x] **P0 & P1 (Core Abstractions)**: `ILLMClient`, `WorkspaceSandbox`, `ToolRegistry`, and secure path resolution.
- [x] **P2 (Tool Integration)**: `list_dir`, `read_file`, `write_file`, and permission-controlled `run_command`.
- [x] **P3 (ReAct Loop)**: Max iteration safeguards, tool feedback loop, and fallback mechanism.
- [ ] **P4 & P5 (Control & Robustness)**: Async task queuing, retry mechanisms, history truncation, and trace logging.
- [ ] **P6 (Advanced)**: Streaming responses, Human-in-the-loop, and gRPC task distribution.

---

## 📄 License
[MIT License](LICENSE)
