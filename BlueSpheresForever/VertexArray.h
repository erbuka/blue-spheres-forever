#pragma once

#include <string>
#include <initializer_list>

#include <glad/glad.h>

namespace bsf
{
	enum class AttributeType
	{
		Float, Float2, Float3, Float4,
		Int, Int2, Int3, Int4,
		UInt, UInt2, UInt3, UInt4
	};

	struct VertexArrayAttribute
	{
		std::string Name;
		AttributeType Type;
	};

	class VertexArray
	{
	public:
		VertexArray(const std::initializer_list<VertexArrayAttribute>& layout);
		~VertexArray();

		uint32_t GetId() const { return m_Id; }
		uint32_t GetCount() const { return m_Count; }

		void SetData(const void* data, uint32_t vertexCount, GLenum usage);
		void SetSubData(const void* data, uint32_t offset, uint32_t vertexCount);

		void Draw(GLenum mode);
		void Draw(GLenum mode, uint32_t count);

	private:
		uint32_t m_VertexSize;
		uint32_t m_Id, m_Vb, m_Count;
	};
}

