#pragma once
#include "Common.h"

#include <initializer_list>
#include <stack>

#include <glad/glad.h>
#include <glm/glm.hpp>


namespace bsf
{


	enum class VertexArrayAttributeType
	{
		Float, Float2, Float3, Float4,
		Int, Int2, Int3, Int4
	};

	struct VertexArrayAttribute
	{
		std::string Name;
		VertexArrayAttributeType Type;
	};

	class VertexArray
	{
	public:
		VertexArray(const std::initializer_list<VertexArrayAttribute>& layout);
		~VertexArray();

		uint32_t GetId() const { return m_Id; }
		uint32_t GetCount() const { return m_Count; }

		void SetData(const void* data, uint32_t vertexCount, GLenum usage = GL_STATIC_DRAW);

	private:
		uint32_t m_VertexSize;
		uint32_t m_Id, m_Vb, m_Count;
	};


	class Renderer3D
	{
	public:

		Renderer3D() {}
		~Renderer3D() {}

		void DrawArrays(const Ref<VertexArray>& va);

		void PushMatrix();
		void PopMatrix();

		void LoadIdentity();
		void Translate(const glm::vec3& translate);
		void Rotate(const glm::vec3& axis, float angle);
		void Scale(const glm::vec3& scale);

		void Perspective(float fovY, float aspect, float zNear, float zFar);
		void Orthographic(float left, float right, float bottom, float top, float zNear, float zFar);


	private:
		glm::mat4 m_Projection;
		std::stack<glm::mat4> m_MatrixStack;
	};
}

