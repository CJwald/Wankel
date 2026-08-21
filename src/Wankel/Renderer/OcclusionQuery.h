#pragma once

#include <cstdint>

namespace Wankel {

// RAII wrapper around a GL_ANY_SAMPLES_PASSED query object - see Renderer::BeginOcclusionQuery/
// BeginConditionalRender. One instance is meant to persist across many frames (reused every frame
// it's tested), not created fresh per draw - GL query results only become meaningful a frame or
// more after being issued, so a query needs continuity across frames to be useful at all.
class OcclusionQuery {
public:
    OcclusionQuery();
    ~OcclusionQuery();

    OcclusionQuery(const OcclusionQuery&) = delete;
    OcclusionQuery& operator=(const OcclusionQuery&) = delete;

    uint32_t GetID() const { return m_ID; }

    // The frame number this query was last issued (BeginOcclusionQuery/EndOcclusionQuery) on - 0 means
    // never. A conditional render should only ever consume a query issued on the *immediately
    // preceding* frame (see the caller's own frame-number comparison), not merely "at some point in
    // the past": a chunk that drops out of the frustum for a while (e.g. turning away, then back) stops
    // having its query reissued every frame, so an old "ever issued" flag alone would let a stale
    // result - reflecting visibility from a since-changed camera angle - wrongly gate a much later
    // frame's render, hiding a chunk that's actually visible again. Comparing exact frame numbers
    // instead makes staleness self-correcting: skip the conditional render and fall back to
    // unconditional (same as a brand-new query) whenever there's a gap.
    uint64_t GetLastIssuedFrame() const { return m_LastIssuedFrame; }
    void MarkIssued(uint64_t frameNumber) { m_LastIssuedFrame = frameNumber; }

    // Non-blocking poll of this query's last-issued result (GL_QUERY_RESULT_AVAILABLE-gated, never
    // stalls) - true + outVisible set if a result was ready, false (outVisible untouched) otherwise,
    // same "keep going without it" philosophy as GPUTimerQuery's own poll. outVisible reflects
    // GL_ANY_SAMPLES_PASSED: true means at least one sample passed the depth test the last time
    // this query ran.
    bool PollVisible(bool& outVisible) const;

private:
    uint32_t m_ID = 0;
    uint64_t m_LastIssuedFrame = 0;
};

} // namespace Wankel
