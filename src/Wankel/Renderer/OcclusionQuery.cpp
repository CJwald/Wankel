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

} // namespace Wankel
