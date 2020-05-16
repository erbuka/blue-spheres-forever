#include "BsfPch.h"

#include "VertexArray.h"
#include "Log.h"

namespace bsf
{

	struct GLAttribDescriptor
	{
		GLenum Type;
		uint32_t ElementCount;
		uint32_t ElementSize;
	};

	static std::map<AttributeType, GLAttribDescriptor> s_GLAttrDescr = {
		{ AttributeType::Float,  { GL_FLOAT, 1, 4 } },
		{ AttributeType::Float2, { GL_FLOAT, 2, 4 } },
		{ AttributeType::Float3, { GL_FLOAT, 3, 4 } },
		{ AttributeType::Float4, { GL_FLOAT, 4, 4 } },
		{ AttributeType::Int,	{ GL_INT, 1, 4 } },
		{ AttributeType::Int2,	{ GL_INT, 2, 4 } },
		{ AttributeType::Int3,	{ GL_INT, 3, 4 } },
		{ AttributeType::Int4,	{ GL_INT, 4, 4 } },
		{ AttributeType::UInt,	{ GL_UNSIGNED_INT, 1, 4 } },
		{ AttributeType::UInt2,	{ GL_UNSIGNED_INT, 2, 4 } },
		{ AttributeType::UInt3,	{ GL_UNSIGNED_INT, 3, 4 } },
		{ AttributeType::UInt4,	{ GL_UNSIGNED_INT, 4, 4 } }
	};


	VertexArray::VertexArray(uint32_t vertexCount, const std::initializer_list<Ref<VertexBuffer>>& buffers) :
		m_Id(0),
		m_VertexCount(vertexCount)
	{
		BSF_GLCALL(glGenVertexArrays(1, &m_Id));

		if (buffers.size() > 0)
		{
			Bind();
			for (auto& buffer : buffers)
				AddVertexBuffer(buffer);
		}
	}

	VertexArray::VertexArray(uint32_t vertexCount, uint32_t vbsCount) : VertexArray(vertexCount)
	{
		for (uint32_t i = 0; i < vbsCount; i++)
			AddVertexBuffer(nullptr);
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

				if (attrib.Type == GL_INT || attrib.Type == GL_UNSIGNED_INT)
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

	void VertexArray::Draw(GLenum mode)
	{
		Draw(mode, m_VertexCount);
	}

	void VertexArray::Draw(GLenum mode, uint32_t count)
	{
		Bind();
		glDrawArrays(mode, 0, count);
	}

	void VertexArray::AddVertexBuffer(const Ref<VertexBuffer>& buffer)
	{
		assert(buffer->GetVertexCount() == m_VertexCount);
		m_Vbs.push_back(buffer);
	}

	void VertexArray::SetVertexBuffer(uint32_t index, const Ref<VertexBuffer>& buffer)
	{
		assert(buffer->GetVertexCount() == m_VertexCount && index < m_Vbs.size());
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
			if (s_GLAttrDescr.find(attr.Type) == s_GLAttrDescr.end())
			{
				BSF_ERROR("Vertex array attribute type is not mapped: {0}", attr.Type);
				return;
			}

			const auto& descr = s_GLAttrDescr[attr.Type];

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
}