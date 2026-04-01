#pragma once

namespace Wankel {

class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    void Bind() const;

    void AddLayout(); // 👈 NEW

private:
    unsigned int m_ID;
};

}
