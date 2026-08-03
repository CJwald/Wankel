#pragma once

#include <cstdint>

namespace Wankel {

// RAII wrapper around a double-buffered pair of GL_TIME_ELAPSED query objects, for measuring the
// GPU-side cost of a render pass (e.g. the terrain draw) without introducing a CPU/GPU sync stall.
// A single query object can't safely be Begin()/End()'d every frame - its result usually isn't
// ready the same frame it was issued, so reading it back immediately would either stall or force
// skipping the read. Alternating between two query objects instead means PollElapsedMs() always
// reads a query issued a full frame ago (by which point it's essentially always ready), keeping the
// same non-stalling philosophy OcclusionQuery already established for chunk visibility testing.
class GPUTimerQuery {
public:
    GPUTimerQuery();
    ~GPUTimerQuery();

    GPUTimerQuery(const GPUTimerQuery&) = delete;
    GPUTimerQuery& operator=(const GPUTimerQuery&) = delete;

    // Wrap the pass to be measured with Begin()/End(), once per frame.
    void Begin();
    void End();

    // Non-blocking - call once per frame, after End(). Reads back the *other* slot's query (issued
    // last frame), keeping the last known value if it isn't available yet rather than stalling.
    float PollElapsedMs();

private:
    uint32_t m_IDs[2] = {0, 0};
    int m_CurrentIndex = 0;
    bool m_HasIssued[2] = {false, false};
    float m_LastElapsedMs = 0.0f;
};

} // namespace Wankel
