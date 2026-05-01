#pragma once

#include <cstdint>

namespace Geometry {

inline constexpr float CubeVertices[] = {
	
	 0.5f,  0.5f,  0.5f,  1.0f,0.0f,0.0f,1.0f, // 1
	-0.5f,  0.5f,  0.5f,  0.0f,1.0f,0.0f,1.0f, // 2
	-0.5f, -0.5f,  0.5f,  0.0f,0.0f,1.0f,1.0f, // 3
	 0.5f, -0.5f,  0.5f,  1.0f,1.0f,0.0f,1.0f, // 4

	 0.5f,  0.5f, -0.5f,  0.0f,1.0f,1.0f,1.0f, // 5
	-0.5f,  0.5f, -0.5f,  1.0f,0.0f,1.0f,1.0f, // 6
	-0.5f, -0.5f, -0.5f,  1.0f,1.0f,1.0f,1.0f, // 7
	 0.5f, -0.5f, -0.5f,  0.0f,0.0f,0.0f,1.0f, // 8
    
};

inline constexpr uint32_t CubeIndices[] = {

    0,1,2, 0,2,3, 
    4,6,5, 4,7,6, 
    0,4,5, 0,5,1, 
    7,2,6, 7,3,2, 
    1,5,6, 1,6,2, 
    0,3,7, 0,7,4  
};

}
