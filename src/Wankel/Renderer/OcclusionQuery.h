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

private:
    uint32_t m_ID = 0;
};

} // namespace Wankel
