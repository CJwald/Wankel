#pragma once
#include <cstdint>

namespace Wankel {

class IndexBuffer {
public:
    IndexBuffer(const uint32_t* indices, uint32_t count);
    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;

    void Bind() const;
    uint32_t GetCount() const;
    uint32_t GetID() const;

    // Respecifies this buffer's storage in place (same GL object ID) and updates GetCount() to match.
    void SetData(const uint32_t* indices, uint32_t count);

private:
    uint32_t m_Count;
    uint32_t m_ID;
};

} // namespace Wankel
