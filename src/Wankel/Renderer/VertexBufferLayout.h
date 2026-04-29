#pragma once
#include <vector>
#include <string>
#include <glad/gl.h>

namespace Wankel {

	struct BufferElement {
		std::string Name;
		GLenum Type;
		uint32_t Count;
		uint32_t Offset;
		bool Normalized;

		BufferElement(std::string name, GLenum type, uint32_t count, uint32_t offset, bool normalized)
			: Name(std::move(name)),
			  Type(type),
			  Count(count),
			  Offset(offset),
			  Normalized(normalized)
		{}

		static uint32_t GetSizeOfType(GLenum type) {
			switch (type) {
				case GL_FLOAT: return 4;
				case GL_UNSIGNED_INT: return 4;
				case GL_UNSIGNED_BYTE: return 1;
			}
			return 0;
		}
	};

	class VertexBufferLayout {
	public:
		VertexBufferLayout() : m_Stride(0) {}

		void PushFloat(uint32_t count, const std::string& name, bool normalized = false) {
			m_Elements.emplace_back(name, GL_FLOAT, count, m_Stride, normalized);
			m_Stride += count * BufferElement::GetSizeOfType(GL_FLOAT);
		}

		void PushUInt(uint32_t count, const std::string& name) {
			m_Elements.emplace_back(name, GL_UNSIGNED_INT, count, m_Stride, false);
			m_Stride += count * BufferElement::GetSizeOfType(GL_UNSIGNED_INT);
		}

		void PushUChar(uint32_t count, const std::string& name, bool normalized = true) {
			m_Elements.emplace_back(name, GL_UNSIGNED_BYTE, count, m_Stride, normalized);
			m_Stride += count * BufferElement::GetSizeOfType(GL_UNSIGNED_BYTE);
		}

		inline const std::vector<BufferElement>& GetElements() const { return m_Elements; }
		inline uint32_t GetStride() const { return m_Stride; }

	private:
		std::vector<BufferElement> m_Elements;
		uint32_t m_Stride;
	};

}
