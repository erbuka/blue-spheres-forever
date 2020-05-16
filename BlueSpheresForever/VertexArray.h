#pragma once

#include "Common.h"

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

	struct VertexAttribute
	{
		std::string Name;
		AttributeType Type;
	};


	class VertexBuffer
	{
	public:

		struct VertexAttributeDef
		{
			uint32_t Pointer;
			uint32_t Count;
			GLenum Type;
		};

		VertexBuffer(const std::initializer_list<VertexAttribute>& layout, const void * data, uint32_t count, GLenum usage = GL_STATIC_DRAW);
		~VertexBuffer();

		VertexBuffer(VertexBuffer&) = delete;
		VertexBuffer(VertexBuffer&&) = delete;
	
		void Bind();

		void SetSubData(const void* data, uint32_t offset, uint32_t count);

		uint32_t GetId() const { return m_Id; }
		uint32_t GetVertexCount() const { return m_Count; }
		uint32_t GetVertexSize() const { return m_VertexSize; }
		const std::vector<VertexAttributeDef>& GetLayout() { return m_Layout; }

	private:
		std::vector<VertexAttributeDef> m_Layout;
		uint32_t m_VertexSize;
		uint32_t m_Id, m_Count;
	};

	class VertexArray
	{
	public:
		VertexArray(uint32_t vertexCount, const std::initializer_list<Ref<VertexBuffer>>& buffers = {});
		VertexArray(uint32_t vertexCount, uint32_t vbsCount);
		~VertexArray();

		VertexArray(VertexArray&) = delete;
		VertexArray(VertexArray&&) = delete;

		uint32_t GetId() const { return m_Id; }

		void Bind();
		
		void Draw(GLenum mode);
		void Draw(GLenum mode, uint32_t count);

		void SetVertexBuffer(uint32_t index, const Ref<VertexBuffer>& buffer);
		Ref<VertexBuffer>& GetVertexBuffer(uint32_t index) { assert(index < m_Vbs.size()); return m_Vbs[index]; }

	private:

		void AddVertexBuffer(const Ref<VertexBuffer>& buffer);
		
		uint32_t m_Id;
		uint32_t m_VertexCount;
		std::vector<Ref<VertexBuffer>> m_Vbs;
	};
}

