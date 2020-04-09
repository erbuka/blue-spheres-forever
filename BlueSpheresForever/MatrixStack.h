#pragma once

#include <glm/glm.hpp>
#include <stack>

namespace bsf
{
	class MatrixStack
	{
	public:

		MatrixStack();

		void Push();
		void Pop();

		void LoadIdentity();
		void Translate(const glm::vec3& translate);
		void Rotate(const glm::vec3& axis, float angle);
		void Scale(const glm::vec3& scale);

		void Perspective(float fovY, float aspect, float zNear, float zFar);
		void Orthographic(float left, float right, float bottom, float top, float zNear, float zFar);
		void LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up = { 0.0f, 1.0f, 0.0f });

		const glm::mat4& GetModelView() const { return m_Stack.top(); }
		const glm::mat4& GetProjection() const { return m_Projection; }

	private:
		glm::mat4 m_Projection;
		std::stack<glm::mat4> m_Stack;
	};
}

