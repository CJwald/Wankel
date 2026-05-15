#pragma once
#include <cstdint>

namespace Wankel {

	class IndexBuffer {
	public:
	    IndexBuffer(const uint32_t* indices, uint32_t count);
	    ~IndexBuffer();
	
	    void Bind() const;
	    uint32_t GetCount() const;
		uint32_t GetID() const;
	
	private:
	    uint32_t m_Count;
	    uint32_t m_ID;
	};

}
