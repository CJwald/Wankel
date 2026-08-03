#include "wkpch.h"
#include "GPUTimerQuery.h"

#include <glad/gl.h>

namespace Wankel {

GPUTimerQuery::GPUTimerQuery() {
    glGenQueries(2, m_IDs);
}

GPUTimerQuery::~GPUTimerQuery() {
    glDeleteQueries(2, m_IDs);
}

void GPUTimerQuery::Begin() {
    glBeginQuery(GL_TIME_ELAPSED, m_IDs[m_CurrentIndex]);
}

void GPUTimerQuery::End() {
    glEndQuery(GL_TIME_ELAPSED);
    m_HasIssued[m_CurrentIndex] = true;
}

float GPUTimerQuery::PollElapsedMs() {
    int otherIndex = 1 - m_CurrentIndex;

    if (m_HasIssued[otherIndex]) {
        GLint available = 0;
        glGetQueryObjectiv(m_IDs[otherIndex], GL_QUERY_RESULT_AVAILABLE, &available);

        if (available) {
            GLuint64 elapsedNs = 0;
            glGetQueryObjectui64v(m_IDs[otherIndex], GL_QUERY_RESULT, &elapsedNs);
            m_LastElapsedMs = (float)elapsedNs / 1000000.0f;
        }
        // Not available yet (rare - a full frame has already elapsed since it was issued): keep the
        // last known value rather than stalling on it.
    }

    m_CurrentIndex = otherIndex;
    return m_LastElapsedMs;
}

} // namespace Wankel
