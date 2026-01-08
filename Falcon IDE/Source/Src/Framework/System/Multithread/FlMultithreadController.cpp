#include "FlMultithreadController.h"

ScopedThreadPool::ScopedThreadPool(size_t threads)
{
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock lock(queueMutex);
                    condition.wait(lock, [this] {
                        return stop || !tasks.empty();
                        });
                    if (stop && tasks.empty()) return;
                    task = std::move(tasks.front());
                    tasks.pop();
                }

                try {
                    task();
                }
                catch (...) {
                    FlEditorAdministrator::Instance().GetLogger()->AddErrorLog("Throw: ScopedThreadPool task");
                }
            }
            });
    }
}


ScopedThreadPool::~ScopedThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }
}
