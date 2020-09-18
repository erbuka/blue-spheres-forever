#include "BsfPch.h"

#include "VertexArray.h"
#include "Log.h"
#include "Table.h"

namespace bsf
{

	struct GLAttribDescriptor
	{
		GLenum Type;
		uint32_t ElementCount;
		uint32_t ElementSize;
	};

	static constexpr Table<16, AttributeType, GLAttribDescriptor> s_GLAttrDescr = {
		std::make_tuple(AttributeType::Float,		GLAttribDescriptor{ GL_FLOAT, 1, 4 }),
		std::make_tuple(AttributeType::Float2,		GLAttribDescriptor{ GL_FLOAT, 2, 4 }),
		std::make_tuple(AttributeType::Float3,		GLAttribDescriptor{ GL_FLOAT, 3, 4 }),
		std::make_tuple(AttributeType::Float4,		GLAttribDescriptor{ GL_FLOAT, 4, 4 }),
		std::make_tuple(AttributeType::Int,			GLAttribDescriptor{ GL_INT, 1, 4 }),
		std::make_tuple(AttributeType::Int2,		GLAttribDescriptor{ GL_INT, 2, 4 }),
		std::make_tuple(AttributeType::Int3,		GLAttribDescriptor{ GL_INT, 3, 4 }),
		std::make_tuple(AttributeType::Int4,		GLAttribDescriptor{ GL_INT, 4, 4 }),
		std::make_tuple(AttributeType::UInt,		GLAttribDescriptor{ GL_UNSIGNED_INT, 1, 4 }),
		std::make_tuple(AttributeType::UInt2,		GLAttribDescriptor{ GL_UNSIGNED_INT, 2, 4 }),
		std::make_tuple(AttributeType::UInt3,		GLAttribDescriptor{ GL_UNSIGNED_INT, 3, 4 }),
		std::make_tuple(AttributeType::UInt4,		GLAttribDescriptor{ GL_UNSIGNED_INT, 4, 4 }),
		std::make_tuple(AttributeType::UShort,		GLAttribDescriptor{ GL_UNSIGNED_SHORT, 1, 2 }),
		std::make_tuple(AttributeType::UShort2,		GLAttribDescriptor{ GL_UNSIGNED_SHORT, 2, 2 }),
		std::make_tuple(AttributeType::UShort3,		GLAttribDescriptor{ GL_UNSIGNED_SHORT, 3, 2 }),
		std::make_tuple(AttributeType::UShort4,		GLAttribDescriptor{ GL_UNSIGNED_SHORT, 4, 2 }),
	};


	VertexArray::VertexArray(uint32_t vertexCount, const std::initializer_list<Ref<VertexBuffer>>& buffers) :
		VertexArray(vertexCount, buffers.size())
	{

		uint32_t index = 0;

		for (auto& buffer : buffers)
		{
			SetVertexBuffer(index, buffer);
			index++;
		}

	}

	VertexArray::VertexArray(uint32_t vertexCount, uint32_t vbsCount) :
		m_VertexCount(vertexCount)
	{
		BSF_GLCALL(glGenVertexArrays(1, &m_Id));

		m_Vbs.resize(vbsCount);

		std::fill(m_Vbs.begin(), m_Vbs.end(), nullptr);

	}

	VertexArray::~VertexArray()
	{
		glDeleteVertexArrays(1, &m_Id);
	}
	
	void VertexArray::Bind()
	{
		BSF_GLCALL(glBindVertexArray(m_Id));

		uint32_t index = 0;

		for (auto& vb : m_Vbs)
		{

			assert(vb != nullptr);

			vb->Bind();

			for (auto& attrib : vb->GetLayout())
			{

				uint32_t vertexSize = vb->GetVertexSize();

				glEnableVertexAttribArray(index);

				if (attrib.Type == GL_INT || attrib.Type == GL_UNSIGNED_INT || attrib.Type == GL_UNSIGNED_SHORT)
				{
					BSF_GLCALL(glVertexAttribIPointer(index, attrib.Count, attrib.Type, vertexSize, (const void*)attrib.Pointer));
				}
				else
				{
					BSF_GLCALL(glVertexAttribPointer(index, attrib.Count, attrib.Type, GL_FALSE, vertexSize, (const void*)attrib.Pointer));
				}

				index++;
			}

		}

	}

	void VertexArray::DrawArrays(GLenum mode)
	{
		DrawArrays(mode, m_VertexCount);
	}

	void VertexArray::DrawArrays(GLenum mode, uint32_t count)
	{
		Bind();
		glDrawArrays(mode, 0, count);
	}

	void VertexArray::DrawIndexed(GLenum mode)
	{
		assert(m_IndexBuffer != nullptr);
		Bind();
		m_IndexBuffer->Bind();
		glDrawElements(mode, m_IndexBuffer->GetCount(), s_GLAttrDescr.Get<0, 1>(m_IndexBuffer->GetType()).Type, 0);
	}

	void VertexArray::Draw(GLenum mode)
	{
		m_IndexBuffer ? DrawIndexed(mode) : DrawArrays(mode);
	}

	void VertexArray::SetVertexBuffer(uint32_t index, const Ref<VertexBuffer>& buffer)
	{
		assert(buffer != nullptr && buffer->GetVertexCount() == m_VertexCount && index < m_Vbs.size());
		m_Vbs[index] = buffer;
	}
	
	VertexBuffer::VertexBuffer(const std::initializer_list<VertexAttribute>& layout, const void* data, uint32_t count, GLenum usage) : 
		m_Id(0),
		m_Count(count),
		m_VertexSize(0)
	{

		uint32_t index = 0;
		uint32_t pointer = 0;

		for (const auto& attr : layout)
		{

			const auto& descr = s_GLAttrDescr.Get<0, 1>(attr.Type);

			m_VertexSize += descr.ElementSize * descr.ElementCount;

			m_Layout.push_back({ pointer, descr.ElementCount, descr.Type });

			pointer += descr.ElementCount * descr.ElementSize;
			index++;
		}

		BSF_GLCALL(glGenBuffers(1, &m_Id));
		Bind();
		glBufferData(GL_ARRAY_BUFFER, m_VertexSize * count, data, usage);

	}
	VertexBuffer::~VertexBuffer()
	{
		if (m_Id == 0)
		{
			BSF_GLCALL(glDeleteBuffers(1, &m_Id));
			m_Id = 0;
		}
	}


	void VertexBuffer::Bind()
	{
		BSF_GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_Id));
	}

	void VertexBuffer::SetSubData(const void* data, uint32_t offset, uint32_t count)
	{
		Bind();
		glBufferSubData(GL_ARRAY_BUFFER, offset * m_VertexSize, count * m_VertexSize, data);
	}

	IndexBuffer::IndexBuffer(const void* data, AttributeType type, size_t count) : m_Count(count)
	{

		assert(type == AttributeType::UInt || type == AttributeType::UShort);

		m_Type = type;

		BSF_GLCALL(glGenBuffers(1, &m_Id));
		BSF_GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id));
		BSF_GLCALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * s_GLAttrDescr.Get<0, 1>(type).ElementSize, data, GL_STATIC_DRAW));
	}

	IndexBuffer::~IndexBuffer()
	{
		BSF_GLCALL(glDeleteBuffers(1, &m_Id));
	}
	void IndexBuffer::Bind()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
	}
}