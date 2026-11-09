//
// build_and_run 工具：白名單編譯與執行
//
// 設計重點：
//  1. 不經過 shell（fork + execv），模型無法注入任何指令
//  2. 編譯器與參數固定：g++ -std=c++20 -Wall -Wextra -pthread，模型只能決定「編譯哪些 .cpp」
//  3. 來源檔必須是 workspace 內的相對路徑、副檔名白名單、不可為符號連結逃出 workspace
//  4. 編譯與執行各有逾時，逾時會 kill 整個 process group
//  5. 輸出大小有上限；執行階段另外限制 CPU 時間、位址空間、檔案大小、core dump
//
// 注意：這不是沙箱。生成的程式仍以你的使用者身分執行，可存取網路與檔案系統。
//       要真正隔離，請把 run 階段改用 bwrap / nsjail / docker run --network none。
//
#ifndef LIYONG_OLLAMA_AGENT_BUILDRUNNER_HPP
#define LIYONG_OLLAMA_AGENT_BUILDRUNNER_HPP

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include <nlohmann/json.hpp>

namespace aiagent
{
    class BuildRunner
    {
    public:
        struct ExecResult
        {
            bool started = false;
            bool timedOut = false;
            bool truncated = false;
            int exitCode = -1;
            int termSignal = 0;
            std::string output;
        };

        explicit BuildRunner(std::filesystem::path workspace,
                             int compileTimeoutSec = 60,
                             int runTimeoutSec = 10,
                             size_t maxOutputBytes = 8192)
            : workspace_(std::filesystem::weakly_canonical(std::filesystem::absolute(std::move(workspace)))),
              compileTimeoutSec_(compileTimeoutSec),
              runTimeoutSec_(runTimeoutSec),
              maxOutputBytes_(maxOutputBytes)
        {
        }

        // OpenAI 風格的工具描述（原生 tools 模式使用；XML 模式不需要）
        static nlohmann::json toolSchema()
        {
            return {
                {"type", "function"},
                {
                    "function", {
                        {"name", "build_and_run"},
                        {
                            "description",
                            "Compile the C++ sources in the workspace with g++ -std=c++20 -pthread and run the "
                            "resulting program. Returns compiler errors or program output."
                        },
                        {
                            "parameters", {
                                {"type", "object"},
                                {
                                    "properties", {
                                        {
                                            "sources", {
                                                {"type", "array"},
                                                {"items", {{"type", "string"}}},
                                                {
                                                    "description",
                                                    "Optional list of relative .cpp paths. Default: all .cpp files in the workspace."
                                                }
                                            }
                                        },
                                        {
                                            "run", {
                                                {"type", "boolean"},
                                                {"description", "Run the program after a successful build (default true)."}
                                            }
                                        }
                                    }
                                },
                                {"required", nlohmann::json::array()}
                            }
                        }
                    }
                }
            };
        }

        // 工具入口：永遠回傳字串（成功或失敗都是給模型看的文字）
        std::string execute(const nlohmann::json& args) const
        {
            namespace fs = std::filesystem;

            std::string err;
            auto sources = resolveSources(args, err);
            if (!sources)
            {
                return "Error: " + err;
            }

            const bool doRun = args.is_object() && args.contains("run") && args["run"].is_boolean()
                                   ? args["run"].get<bool>()
                                   : true;

            // 刪掉舊的執行檔，避免編譯失敗卻執行到舊版本
            std::error_code ec;
            fs::remove(workspace_ / "main", ec);

            std::vector<std::string> argv = {
                "g++", "-std=c++20", "-Wall", "-Wextra", "-pthread",
                "-I.", "-Iinclude", "-Isrc"
            };
            for (const auto& s : *sources) argv.push_back(s);
            argv.push_back("-o");
            argv.push_back("main");

            Limits compileLimits;
            compileLimits.cpuSeconds = compileTimeoutSec_ + 5;
            ExecResult build = runProcess(argv, compileTimeoutSec_, compileLimits);

            std::string report;
            if (!build.started)
            {
                return "Error: failed to start compiler (is g++ installed?)";
            }
            if (build.timedOut)
            {
                return "BUILD FAILED: compiler timed out after " + std::to_string(compileTimeoutSec_) + "s.\n" +
                       build.output;
            }
            if (build.exitCode != 0)
            {
                return "BUILD FAILED (exit code " + std::to_string(build.exitCode) + ").\n"
                       "--- compiler output ---\n" + build.output + (build.truncated ? "\n...[truncated]" : "") +
                       "\n--- end ---\n"
                       "Fix the errors by re-creating the affected file(s) with create_file, then call build_and_run again.";
            }

            report = "BUILD OK.\n";
            if (!build.output.empty())
            {
                report += "--- compiler warnings ---\n" + build.output + (build.truncated ? "\n...[truncated]" : "") +
                          "\n--- end ---\n";
            }

            if (!doRun) return report;

            Limits runLimits;
            runLimits.cpuSeconds = runTimeoutSec_ + 1;
            runLimits.addressSpaceBytes = 2ULL * 1024 * 1024 * 1024;
            runLimits.fileSizeBytes = 10ULL * 1024 * 1024;
            ExecResult run = runProcess({(workspace_ / "main").string()}, runTimeoutSec_, runLimits);

            if (!run.started)
            {
                return report + "Error: failed to start the program.";
            }
            if (run.timedOut)
            {
                report += "RUN FAILED: timed out after " + std::to_string(runTimeoutSec_) +
                          "s (deadlock or infinite loop?).\n";
            }
            else if (run.termSignal != 0)
            {
                report += "RUN FAILED: killed by signal " + std::to_string(run.termSignal) +
                          " (" + signalName(run.termSignal) + ").\n";
            }
            else
            {
                report += "RUN finished with exit code " + std::to_string(run.exitCode) + ".\n";
            }
            report += "--- program output ---\n" + run.output + (run.truncated ? "\n...[truncated]" : "") +
                      "\n--- end ---\n";

            const bool ok = !run.timedOut && run.termSignal == 0 && run.exitCode == 0;
            report += ok
                          ? "If the output matches the task requirements, reply with the single word: DONE"
                          : "Fix the problem by re-creating the affected file(s) with create_file, then call build_and_run again.";
            return report;
        }

    private:
        struct Limits
        {
            rlim_t cpuSeconds = 0;
            rlim_t addressSpaceBytes = 0; // 0 = 不限制
            rlim_t fileSizeBytes = 0; // 0 = 不限制
        };

        std::filesystem::path workspace_;
        int compileTimeoutSec_;
        int runTimeoutSec_;
        size_t maxOutputBytes_;

        static std::string signalName(int sig)
        {
            switch (sig)
            {
            case SIGSEGV: return "SIGSEGV";
            case SIGABRT: return "SIGABRT";
            case SIGFPE: return "SIGFPE";
            case SIGKILL: return "SIGKILL";
            case SIGXCPU: return "SIGXCPU";
            case SIGBUS: return "SIGBUS";
            case SIGTERM: return "SIGTERM";
            default: return "signal";
            }
        }

        static bool hasAllowedExt(const std::filesystem::path& p)
        {
            const std::string e = p.extension().string();
            return e == ".cpp" || e == ".cc" || e == ".cxx";
        }

        static bool isInside(const std::filesystem::path& base, const std::filesystem::path& p)
        {
            auto rel = p.lexically_relative(base);
            if (rel.empty()) return false;
            return *rel.begin() != "..";
        }

        // 取得要編譯的來源清單（相對 workspace 的路徑）
        std::optional<std::vector<std::string>> resolveSources(const nlohmann::json& args, std::string& err) const
        {
            namespace fs = std::filesystem;
            std::vector<std::string> out;

            if (args.is_object() && args.contains("sources"))
            {
                if (!args["sources"].is_array())
                {
                    err = "'sources' must be an array of relative paths.";
                    return std::nullopt;
                }
                for (const auto& item : args["sources"])
                {
                    if (!item.is_string())
                    {
                        err = "'sources' must contain strings only.";
                        return std::nullopt;
                    }
                    std::string s = item.get<std::string>();
                    fs::path rel(s);
                    if (s.empty() || s[0] == '-' || rel.is_absolute() || !hasAllowedExt(rel))
                    {
                        err = "invalid source path: '" + s + "' (must be a relative .cpp/.cc/.cxx path).";
                        return std::nullopt;
                    }
                    std::error_code ec;
                    fs::path full = fs::weakly_canonical(workspace_ / rel, ec);
                    if (ec || !isInside(workspace_, full) || !fs::is_regular_file(full, ec))
                    {
                        err = "source not found inside workspace: '" + s + "'.";
                        return std::nullopt;
                    }
                    out.push_back(full.lexically_relative(workspace_).string());
                }
            }
            else
            {
                std::error_code ec;
                fs::recursive_directory_iterator it(workspace_, fs::directory_options::skip_permission_denied, ec);
                for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
                {
                    const auto& entry = *it;
                    std::error_code ec2;
                    if (entry.is_symlink(ec2) || !entry.is_regular_file(ec2)) continue;
                    if (!hasAllowedExt(entry.path())) continue;
                    out.push_back(entry.path().lexically_relative(workspace_).string());
                    if (out.size() > 50) break;
                }
                std::sort(out.begin(), out.end());
            }

            if (out.empty())
            {
                err = "no .cpp source files found in workspace. Create them with create_file first.";
                return std::nullopt;
            }
            if (out.size() > 50)
            {
                err = "too many source files (limit 50).";
                return std::nullopt;
            }
            return out;
        }

        // 在 PATH 中解析可執行檔（在 fork 之前做，子行程只能呼叫 async-signal-safe 函式）
        static std::string resolveExecutable(const std::string& name)
        {
            if (name.find('/') != std::string::npos) return name;
            const char* pathEnv = std::getenv("PATH");
            std::string paths = pathEnv ? pathEnv : "/usr/local/bin:/usr/bin:/bin";
            size_t start = 0;
            while (start <= paths.size())
            {
                size_t end = paths.find(':', start);
                if (end == std::string::npos) end = paths.size();
                std::string dir = paths.substr(start, end - start);
                if (dir.empty()) dir = ".";
                std::string cand = dir + "/" + name;
                if (::access(cand.c_str(), X_OK) == 0) return cand;
                start = end + 1;
            }
            return "";
        }

        ExecResult runProcess(const std::vector<std::string>& argvStr, int timeoutSec, const Limits& lim) const
        {
            ExecResult res;

            std::string exe = resolveExecutable(argvStr.at(0));
            if (exe.empty()) return res;

            // 所有需要配置記憶體的準備工作都在 fork 之前完成
            std::vector<char*> argv;
            for (const auto& a : argvStr) argv.push_back(const_cast<char*>(a.c_str()));
            argv.push_back(nullptr);
            const std::string cwd = workspace_.string();

            int pfd[2];
            if (::pipe2(pfd, O_CLOEXEC) != 0) return res;

            pid_t pid = ::fork();
            if (pid < 0)
            {
                ::close(pfd[0]);
                ::close(pfd[1]);
                return res;
            }

            if (pid == 0)
            {
                // ---- 子行程：只使用 async-signal-safe 函式 ----
                ::setpgid(0, 0);
                ::dup2(pfd[1], STDOUT_FILENO);
                ::dup2(pfd[1], STDERR_FILENO);
                int devnull = ::open("/dev/null", O_RDONLY);
                if (devnull >= 0) ::dup2(devnull, STDIN_FILENO);

                if (::chdir(cwd.c_str()) != 0) ::_exit(126);

                rlimit core{0, 0};
                ::setrlimit(RLIMIT_CORE, &core);
                if (lim.cpuSeconds > 0)
                {
                    rlimit r{lim.cpuSeconds, lim.cpuSeconds};
                    ::setrlimit(RLIMIT_CPU, &r);
                }
                if (lim.addressSpaceBytes > 0)
                {
                    rlimit r{lim.addressSpaceBytes, lim.addressSpaceBytes};
                    ::setrlimit(RLIMIT_AS, &r);
                }
                if (lim.fileSizeBytes > 0)
                {
                    rlimit r{lim.fileSizeBytes, lim.fileSizeBytes};
                    ::setrlimit(RLIMIT_FSIZE, &r);
                }

                ::execv(exe.c_str(), argv.data());
                ::_exit(127);
            }

            // ---- 父行程 ----
            ::setpgid(pid, pid); // 與子行程的 setpgid 競爭無妨，兩邊結果相同
            ::close(pfd[1]);
            res.started = true;

            const int fd = pfd[0];
            ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);

            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeoutSec);
            int status = 0;
            bool reaped = false;
            char buf[4096];

            auto drain = [&]()
            {
                while (true)
                {
                    ssize_t n = ::read(fd, buf, sizeof(buf));
                    if (n > 0)
                    {
                        if (res.output.size() < maxOutputBytes_)
                        {
                            size_t room = maxOutputBytes_ - res.output.size();
                            res.output.append(buf, std::min(static_cast<size_t>(n), room));
                            if (static_cast<size_t>(n) > room) res.truncated = true;
                        }
                        else
                        {
                            res.truncated = true;
                        }
                        continue;
                    }
                    return n == 0; // true = EOF
                }
            };

            while (true)
            {
                pollfd p{fd, POLLIN, 0};
                ::poll(&p, 1, 100);
                bool eof = drain();

                if (!reaped)
                {
                    pid_t w = ::waitpid(pid, &status, WNOHANG);
                    if (w == pid) reaped = true;
                }
                if (reaped || eof) break;

                if (std::chrono::steady_clock::now() >= deadline)
                {
                    res.timedOut = true;
                    break;
                }
            }

            if (res.timedOut || !reaped)
            {
                ::kill(-pid, SIGKILL);
                ::kill(pid, SIGKILL);
            }
            if (!reaped)
            {
                ::waitpid(pid, &status, 0);
            }
            else
            {
                ::kill(-pid, SIGKILL); // 清掉可能殘留的子孫行程
            }
            drain();
            ::close(fd);

            if (WIFEXITED(status)) res.exitCode = WEXITSTATUS(status);
            else if (WIFSIGNALED(status)) res.termSignal = WTERMSIG(status);
            return res;
        }
    };
}

#endif //LIYONG_OLLAMA_AGENT_BUILDRUNNER_HPP