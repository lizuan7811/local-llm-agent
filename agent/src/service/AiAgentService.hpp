//
// Created by root on 2026/10/20.
//

#ifndef LIYONG_OLLAMA_AGENT_AIAGENTSERVICE_HPP
#define LIYONG_OLLAMA_AGENT_AIAGENTSERVICE_HPP

#include <iostream>
#include <vector>
#include <queue>
#include <future>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <concepts>
#include <stop_token>
#include <memory>
#include <utility>
#include <string>
#include <thread>

#include "../include/adapter/ModelRouter.hpp"

namespace aiagent {
    class AIAgentService {
    private:
        std::vector<std::jthread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        ModelRouter router_;

    public:
        explicit AIAgentService(size_t num_threads, const std::shared_ptr<IModelAdapter>  &imo) : router_(imo) {
            for (size_t i = 0; i < num_threads; ++i) {
                workers.emplace_back([this](std::stop_token stoken) {
                    while (!stoken.stop_requested()) {
                        std::function<void()> task;
                        {
                            std::unique_lock lock(queue_mutex);
                            condition.wait(lock, [this, &stoken] {
                                return !tasks.empty() || stoken.stop_requested();
                            });

                            // 如果收到停止請求且佇列已空，直接離開執行緒迴圈
                            if (stoken.stop_requested() && tasks.empty()) {
                                return;
                            }

                            if (!tasks.empty()) {
                                task = std::move(tasks.front());
                                tasks.pop();
                            }
                        }

                        // 執行任務
                        if (task) {
                            task();
                        }
                    }
                });
            }
        }

        ~AIAgentService() {
            // 發出停止請求給所有 jthread
            for (auto &worker: workers) {
                worker.request_stop();
            }
            // 喚醒所有等待中的執行緒，讓它們能夠順利退出
            condition.notify_all();

            // 確保所有執行緒都已經結束
            for (auto &worker: workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }

        // 支援任意參數與回傳值的 submit 成員函式
        template<typename F, typename... Args>
        auto submit(F &&func, Args &&... args)
            -> std::future<std::invoke_result_t<F, Args...> > {
            using return_type = std::invoke_result_t<F, Args...>;

            // 使用 shared_ptr 包裝 packaged_task，解決 std::function 需要 Copyable 的限制
            auto task_pkg = std::make_shared<std::packaged_task<return_type()> >(
                [func = std::forward<F>(func), ...args = std::forward<Args>(args)]() mutable -> return_type {
                    return std::invoke(func, args...);
                }
            );

            std::future<return_type> future = task_pkg->get_future();

            // 將任務推入執行緒池的任務佇列中
            {
                std::lock_guard lock(queue_mutex);
                tasks.emplace([task_pkg]() {
                    (*task_pkg)();
                });
            }

            condition.notify_one();
            return future;
        }

        template<typename Callback>
        auto run_agent_task(AgentContext &context, Callback &&on_complete) {
            return submit([this, &ctx = context, callback=std::forward<Callback>(on_complete)]() {
                auto strategy = router_.route(ctx);

                openai_spec::ModelResponse response = strategy->execute(ctx);

                callback(response);
            });
        }
    };
} // namespace ai_agent

#endif //LIYONG_OLLAMA_AGENT_AIAGENTSERVICE_HPP
