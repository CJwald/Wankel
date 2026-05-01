#pragma once
#include <cstdint>

namespace Wankel {

class IndexBuffer {
public:
    IndexBuffer(const uint32_t* indices, uint32_t count);
    ~IndexBuffer();

    void Bind() const;
	unsigned int GetID() const { return m_ID; }

    uint32_t GetCount() const { return m_Count; }

private:
    uint32_t m_ID;
    uint32_t m_Count;
};

}
