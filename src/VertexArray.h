#pragma once

#include "Ref.h"
#include "Asset.h"

#include <string>
#include <initializer_list>

#include <glad/glad.h>

namespace bsf
{
	enum class AttributeType
	{
		Float, Float2, Float3, Float4,
		Int, Int2, Int3, Int4,
		UInt, UInt2, UInt3, UInt4,
		UShort, UShort2, UShort3, UShort4
	};

	struct VertexAttribute
	{
		std::string Name;
		AttributeType Type;
	};

	class IndexBuffer : public Asset
	{
	public:
		IndexBuffer(const void* data, AttributeType type, size_t Count);
		~IndexBuffer();

		void Bind();

		size_t GetCount() const { return m_Count; }

		AttributeType GetType() const { return m_Type; }

	private:
		size_t m_Count;
		uint32_t m_Id;
		AttributeType m_Type;
	};


	class VertexBuffer: public Asset
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

	class VertexArray : public Asset
	{
	public:
		VertexArray(uint32_t vertexCount, const std::initializer_list<Ref<VertexBuffer>>& buffers = {});
		VertexArray(uint32_t vertexCount, uint32_t vbsCount);
		~VertexArray();

		VertexArray(VertexArray&) = delete;
		VertexArray(VertexArray&&) = delete;

		uint32_t GetId() const { return m_Id; }

		void Bind();
		
		void DrawArrays(GLenum mode);
		void DrawArrays(GLenum mode, uint32_t count);
		void DrawIndexed(GLenum mode);
		void Draw(GLenum mode);

		void SetVertexBuffer(uint32_t index, const Ref<VertexBuffer>& buffer);
		Ref<VertexBuffer>& GetVertexBuffer(uint32_t index) { assert(index < m_Vbs.size()); return m_Vbs[index]; }

		void SetIndexBuffer(const Ref<IndexBuffer>& ib) { m_IndexBuffer = ib; }

		uint32_t GetVertexCount() const { return m_VertexCount; }

	private:
		
		uint32_t m_Id;
		uint32_t m_VertexCount;
		std::vector<Ref<VertexBuffer>> m_Vbs;
		Ref<IndexBuffer> m_IndexBuffer = nullptr;
	};
}

