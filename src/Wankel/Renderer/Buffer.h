#pragma once

namespace Wankel {

	class VertexBuffer {
	public:
	    VertexBuffer(float* vertices, unsigned int size);
	    ~VertexBuffer();
	
	    void Bind() const;
	private:
	    unsigned int m_ID;
	};

}
