#pragma once

namespace Wankel {

	class VertexArray {
	public:
	    VertexArray();
	    ~VertexArray();
	
	    void Bind() const;
	
	    void AddLayout();
	
	private:
	    unsigned int m_ID;
	};

}
