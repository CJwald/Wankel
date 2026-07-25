#include "wkpch.h"
#include "JobSystem.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace Wankel {

namespace {

std::vector<std::thread> s_Workers;
std::mutex s_JobMutex;
std::condition_variable s_JobCV;
bool s_Stop = false;

std::mutex s_MainThreadMutex;

// Function-local statics (not namespace-scope) so construction happens on first call, inside normal
// control flow, rather than before main() where a throwing static-init could not be caught.
std::queue<std::function<void()>>& JobQueue() {
    static std::queue<std::function<void()>> queue;
    return queue;
}

std::queue<std::function<void()>>& MainThreadQueue() {
    static std::queue<std::function<void()>> queue;
    return queue;
}

void WorkerLoop() {
    for (;;) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(s_JobMutex);
            s_JobCV.wait(lock, [] { return s_Stop || !JobQueue().empty(); });
            if (s_Stop && JobQueue().empty())
                return;
            job = std::move(JobQueue().front());
            JobQueue().pop();
        }
        job();
    }
}

} // namespace

void JobSystem::Init(unsigned int threadCount) {
    if (threadCount == 0)
        threadCount = std::max(1u, std::thread::hardware_concurrency());

    s_Stop = false;
    s_Workers.reserve(threadCount);
    for (unsigned int i = 0; i < threadCount; i++)
        s_Workers.emplace_back(WorkerLoop);

    WK_CORE_INFO("JobSystem: initialized ({0} worker threads)", threadCount);
}

void JobSystem::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(s_JobMutex);
        s_Stop = true;
    }
    s_JobCV.notify_all();
    for (std::thread& worker : s_Workers) {
        if (worker.joinable())
            worker.join();
    }
    s_Workers.clear();

    // Nothing should submit new work once shutdown begins, and running callbacks after layer/app
    // teardown started would be unsafe - drop whatever's left rather than executing it.
    std::lock_guard<std::mutex> lock(s_MainThreadMutex);
    while (!MainThreadQueue().empty())
        MainThreadQueue().pop();
}

void JobSystem::Submit(std::function<void()> fn) {
    {
        std::lock_guard<std::mutex> lock(s_JobMutex);
        JobQueue().push(std::move(fn));
    }
    s_JobCV.notify_one();
}

void JobSystem::SubmitMainThread(std::function<void()> fn) {
    std::lock_guard<std::mutex> lock(s_MainThreadMutex);
    MainThreadQueue().push(std::move(fn));
}

void JobSystem::RunMainThreadQueue(int maxJobs) {
    std::vector<std::function<void()>> toRun;
    {
        std::lock_guard<std::mutex> lock(s_MainThreadMutex);
        int n = maxJobs <= 0 ? (int)MainThreadQueue().size() : std::min(maxJobs, (int)MainThreadQueue().size());
        toRun.reserve(n);
        for (int i = 0; i < n; i++) {
            toRun.push_back(std::move(MainThreadQueue().front()));
            MainThreadQueue().pop();
        }
    }

    // Run outside the lock - a callback may itself call SubmitMainThread again (e.g. a batched
    // upload re-queuing its own continuation), which would deadlock on a lock still held here.
    for (std::function<void()>& job : toRun)
        job();
}

unsigned int JobSystem::WorkerCount() {
    return (unsigned int)s_Workers.size();
}

} // namespace Wankel
