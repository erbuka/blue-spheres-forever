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


	VertexArray::VertexArray(const std::initializer_list<VertexArrayAttribute>& layout) :
		m_Id(0),
		m_Vb(0),
		m_Count(0),
		m_VertexSize(0)
	{
		for (const auto& attr : layout)
		{
			if (s_GLAttrDescr.find(attr.Type) == s_GLAttrDescr.end())
				BSF_ERROR("Vertex array attribute type is not mapped: {0}", attr.Type);

			const auto& descr = s_GLAttrDescr[attr.Type];
			m_VertexSize += descr.ElementSize * descr.ElementCount;
		}


		BSF_GLCALL(glGenVertexArrays(1, &m_Id));
		BSF_GLCALL(glBindVertexArray(m_Id));

		BSF_GLCALL(glGenBuffers(1, &m_Vb));
		BSF_GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_Vb));

		uint32_t index = 0;
		uint32_t pointer = 0;

		for (const auto& attr : layout)
		{
			const auto& descr = s_GLAttrDescr[attr.Type];

			BSF_GLCALL(glEnableVertexAttribArray(index));

			if (descr.Type == GL_INT || descr.Type == GL_UNSIGNED_INT)
			{
				BSF_GLCALL(glVertexAttribIPointer(index, descr.ElementCount, descr.Type, m_VertexSize, (const void*)pointer));
			}
			else
			{
				BSF_GLCALL(glVertexAttribPointer(index, descr.ElementCount, descr.Type, GL_FALSE, m_VertexSize, (const void*)pointer));
			}

			pointer += descr.ElementCount * descr.ElementSize;

			index++;
		}


	}

	VertexArray::~VertexArray()
	{
		glDeleteBuffers(1, &m_Vb);
		glDeleteVertexArrays(1, &m_Id);
	}

	void VertexArray::SetData(const void* data, uint32_t vertexCount, GLenum usage)
	{
		m_Count = vertexCount;
		glBindBuffer(GL_ARRAY_BUFFER, m_Vb);
		glBufferData(GL_ARRAY_BUFFER, vertexCount * m_VertexSize, data, usage);
	}

	void VertexArray::SetSubData(const void* data, uint32_t offset, uint32_t vertexCount)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_Vb);
		glBufferSubData(GL_ARRAY_BUFFER, offset, vertexCount * m_VertexSize, data);
	}
	
	void VertexArray::Draw(GLenum mode)
	{
		Draw(mode, m_Count);
	}

	void VertexArray::Draw(GLenum mode, uint32_t count)
	{
		glBindVertexArray(m_Id);
		glDrawArrays(mode, 0, count);
	}
}