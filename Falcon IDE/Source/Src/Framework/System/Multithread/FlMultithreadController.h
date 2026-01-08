#pragma once

/// <summary> Resource Acquisition Is Initialization </summary>
class ScopedThreadPool 
{
public:
    explicit ScopedThreadPool(size_t threads);

    ScopedThreadPool(const ScopedThreadPool&) = delete;
    ScopedThreadPool& operator=(const ScopedThreadPool&) = delete;

    ~ScopedThreadPool();

    template<class F, class... Args>
    auto Enqueue(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using return_type = std::invoke_result_t<F, Args...>;

        // Š®‘S“]‘—‚³‚ê‚½ŠÖ”‚Æˆø”‚Å packaged_task ‚ğì¬
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            [fn = std::forward<F>(f),
            ... arg = std::forward<Args>(args)]() mutable {
                return std::invoke(std::move(fn), std::move(arg)...);
            }
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");

            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }


private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop{ false };
};