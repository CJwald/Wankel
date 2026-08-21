#include "wkpch.h"
#include "OcclusionQuery.h"

#include <glad/gl.h>

namespace Wankel {

OcclusionQuery::OcclusionQuery() {
    glGenQueries(1, &m_ID);
}

OcclusionQuery::~OcclusionQuery() {
    glDeleteQueries(1, &m_ID);
}

bool OcclusionQuery::PollVisible(bool& outVisible) const {
    GLint available = 0;
    glGetQueryObjectiv(m_ID, GL_QUERY_RESULT_AVAILABLE, &available);
    if (!available)
        return false;

    GLuint result = 0;
    glGetQueryObjectuiv(m_ID, GL_QUERY_RESULT, &result);
    outVisible = result != 0;
    return true;
}

} // namespace Wankel
