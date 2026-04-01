#pragma once

namespace Wankel {

class VertexArray;

class Renderer {
public:
    static void Clear();
	static void Draw(VertexArray* vao, int count);
};

}
