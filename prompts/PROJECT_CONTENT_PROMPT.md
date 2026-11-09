# Task: C++20 generic thread pool ("AI Agent Service")

Create exactly ONE file: `main.cpp` (everything in a single file).
It must compile with: `g++ -std=c++20 -pthread main.cpp -o main`

## Requirements
1. `class AgentService` with constructor `explicit AgentService(size_t threads)` (pool size configurable).
2. Generic submit:
   `template <typename F, typename... Args> auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>`
   - Use `std::packaged_task`, `std::forward` and perfect forwarding.
   - Accept functions, lambdas and functors.
3. Workers are `std::jthread`; each worker loop uses a `std::stop_token`.
4. Task queue (`std::queue<std::function<void()>>`) protected by `std::mutex` + `std::condition_variable_any`.
   Use `cv.wait(lock, stoken, predicate)` (plain `std::condition_variable` does not accept a stop_token).
5. Destructor (RAII): request stop, let workers finish all queued tasks, then join.
6. Exceptions thrown inside a task must be rethrown by `future.get()`.
7. `main()` must:
   - submit 3 or more tasks with DIFFERENT return types: `std::string`, a user-defined struct, and `int`;
   - submit one task that throws, and catch the exception around its `get()`;
   - print every result.

## Pitfalls to avoid
- `std::packaged_task` and `std::future` are move-only. Store the task in a `std::shared_ptr<std::packaged_task<R()>>` and capture it in a lambda that is stored as `std::function<void()>`. Return the future obtained from `get_future()` by value (move), never copy it.
- Call `future.get()` only ONCE per future.
- Do NOT run the task inside `submit()`; only enqueue it.
- Do NOT use `std::bind` with the move-only task; use a lambda or `std::apply` to capture arguments.
- Include `<thread>`, `<stop_token>`, `<future>`, `<functional>`, `<queue>`, `<mutex>`, `<condition_variable>`, `<string>`, `<iostream>`, `<vector>`, `<memory>`, `<stdexcept>`, `<type_traits>`.