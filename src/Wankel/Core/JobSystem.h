#pragma once

#include <functional>

namespace Wankel {

// Fire-and-forget worker pool plus a main-thread completion queue. Workers run arbitrary CPU-only work
// submitted via Submit(); anything touching OpenGL, the ECS registry, or other main-thread-only state
// must instead go through SubmitMainThread(), drained once per frame by Application::Run().
class JobSystem {
public:
    // threadCount == 0 picks hardware_concurrency() (clamped >= 1) additional worker threads.
    static void Init(unsigned int threadCount = 0);
    static void Shutdown();

    // Runs fn on a worker thread at some future point, exactly once. fn must not touch OpenGL,
    // Scene/registry, or any other main-thread-only state - only pure CPU work / privately-owned data.
    static void Submit(std::function<void()> fn);

    // Runs fn exactly once, only when RunMainThreadQueue() next drains it on the main thread.
    static void SubmitMainThread(std::function<void()> fn);

    // Call once per frame from the main thread; drains up to maxJobs pending callbacks (<=0 = drain all).
    static void RunMainThreadQueue(int maxJobs = 64);

    static unsigned int WorkerCount();
};

} // namespace Wankel
