#include "wkpch.h"
#include "JobSystem.h"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>

namespace Wankel {

namespace {

std::vector<std::thread> s_Workers;
std::mutex s_JobMutex;
std::condition_variable s_JobCV;
bool s_Stop = false;
uint64_t s_NextSequence = 0; // guarded by s_JobMutex, same as the queue it orders

std::mutex s_MainThreadMutex;

// Priority + FIFO-among-ties ordering for the worker job queue - see docs/VoxelJobSystemTODO.md step 4.
// Sequence is a monotonic submission counter so jobs of equal priority run in the order they were
// submitted, rather than being reordered arbitrarily by the heap.
struct JobEntry {
    int Priority = 0;
    uint64_t Sequence = 0;
    std::function<void()> Fn;
};

// std::priority_queue's top() is the "greatest" element per this comparator - higher Priority wins;
// among equal priority, the EARLIER (smaller) Sequence should win, so it must compare as "greater".
struct JobEntryCompare {
    bool operator()(const JobEntry& a, const JobEntry& b) const {
        if (a.Priority != b.Priority)
            return a.Priority < b.Priority;
        return a.Sequence > b.Sequence;
    }
};

// Function-local statics (not namespace-scope) so construction happens on first call, inside normal
// control flow, rather than before main() where a throwing static-init could not be caught.
std::priority_queue<JobEntry, std::vector<JobEntry>, JobEntryCompare>& JobQueue() {
    static std::priority_queue<JobEntry, std::vector<JobEntry>, JobEntryCompare> queue;
    return queue;
}

std::queue<std::function<void()>>& MainThreadQueue() {
    static std::queue<std::function<void()>> queue;
    return queue;
}

void WorkerLoop() {
    for (;;) {
        JobEntry entry;
        {
            std::unique_lock<std::mutex> lock(s_JobMutex);
            s_JobCV.wait(lock, [] { return s_Stop || !JobQueue().empty(); });
            if (s_Stop && JobQueue().empty())
                return;
            // priority_queue::top() only exposes a const reference (its container invariants forbid
            // mutating an element in place), but it's about to be pop()'d under this same lock, so
            // moving out of it first is safe - standard extract-then-pop idiom for a movable-only T.
            entry = std::move(const_cast<JobEntry&>(JobQueue().top()));
            JobQueue().pop();
        }
        entry.Fn();
    }
}

} // namespace

void JobSystem::Init(unsigned int threadCount) {
    if (threadCount == 0) {
        // Reserve one core for the main thread - see docs/VoxelJobSystemTODO.md step 5. Guards the
        // 0-or-1-core case explicitly rather than letting `hw - 1` underflow as unsigned.
        unsigned int hw = std::thread::hardware_concurrency();
        threadCount = hw > 1 ? hw - 1 : 1;
    }

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

void JobSystem::Submit(std::function<void()> fn, int priority) {
    {
        std::lock_guard<std::mutex> lock(s_JobMutex);
        JobQueue().push(JobEntry {priority, s_NextSequence++, std::move(fn)});
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
