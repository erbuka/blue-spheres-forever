#include "Renderer3D.h"
#include "Log.h"

#include <map>
#include <glm/ext.hpp>
#include <glm/gtx/transform.hpp>


namespace bsf
{

	struct GLAttribDescriptor
	{
		GLenum Type;
		uint32_t ElementCount;
		uint32_t ElementSize;
	};

	static std::map<VertexArrayAttributeType, GLAttribDescriptor> s_GLAttrDescr = {
		{ VertexArrayAttributeType::Float,  { GL_FLOAT, 1, 4 } },
		{ VertexArrayAttributeType::Float2, { GL_FLOAT, 2, 4 } },
		{ VertexArrayAttributeType::Float3, { GL_FLOAT, 3, 4 } },
		{ VertexArrayAttributeType::Float4, { GL_FLOAT, 4, 4 } },
		{ VertexArrayAttributeType::Int,	{ GL_INT, 1, 4 } },
		{ VertexArrayAttributeType::Int2,	{ GL_INT, 2, 4 } },
		{ VertexArrayAttributeType::Int3,	{ GL_INT, 3, 4 } },
		{ VertexArrayAttributeType::Int4,	{ GL_INT, 4, 4 } },
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

		
		glGenVertexArrays(1, &m_Id);
		glBindVertexArray(m_Id);

		glGenBuffers(1, &m_Vb);
		glBindBuffer(GL_ARRAY_BUFFER, m_Vb);


		uint32_t index = 0;
		uint32_t pointer = 0;

		for (const auto& attr : layout)
		{
			const auto& descr = s_GLAttrDescr[attr.Type];

			glEnableVertexAttribArray(index);

			glVertexAttribPointer(0, descr.Type, descr.ElementCount, GL_FALSE, m_VertexSize, (const void*)pointer);

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

	void Renderer3D::DrawArrays(const Ref<VertexArray>& va)
	{
		glBindVertexArray(va->GetId());
		glDrawArrays(GL_TRIANGLES, 0, va->GetCount());
	}
	void Renderer3D::PushMatrix()
	{
		m_MatrixStack.push(m_MatrixStack.top());
	}
	void Renderer3D::PopMatrix()
	{
		assert(m_MatrixStack.size() > 0);
		m_MatrixStack.pop();
	}
	void Renderer3D::LoadIdentity()
	{
		m_MatrixStack.top() = glm::identity<glm::mat4>();
	}
	void Renderer3D::Translate(const glm::vec3& translate)
	{
		m_MatrixStack.top() *= glm::translate(translate);

	}
	void Renderer3D::Rotate(const glm::vec3& axis, float angle)
	{
		m_MatrixStack.top() *= glm::rotate(angle, axis);
	}
	void Renderer3D::Scale(const glm::vec3& scale)
	{
		m_MatrixStack.top() *= glm::scale(scale);
	}

	void Renderer3D::Perspective(float fovY, float aspect, float zNear, float zFar)
	{
		m_Projection = glm::perspective(fovY, aspect, zNear, zFar);
	}

	void Renderer3D::Orthographic(float left, float right, float bottom, float top, float zNear, float zFar)
	{
		m_Projection = glm::ortho(left, right, bottom, top, zNear, zFar);
	}

}