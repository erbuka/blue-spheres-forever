#pragma once

#include <glm/glm.hpp>
#include <stack>

namespace bsf
{
	class MatrixStack
	{
	public:

		MatrixStack();

		operator glm::mat4();

		void Push();
		void Pop();

		void LoadIdentity();
		void Translate(const glm::vec3& translate);
		void Rotate(const glm::vec3& axis, float angle);
		void Scale(const glm::vec3& scale);
		void Multiply(const glm::mat4& mat);

		void Perspective(float fovY, float aspect, float zNear, float zFar);
		void Orthographic(float left, float right, float bottom, float top, float zNear, float zFar);
		void LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up = { 0.0f, 1.0f, 0.0f });

		const glm::mat4& GetMatrix() const { return m_Stack.top(); }

	private:
		std::stack<glm::mat4> m_Stack;
	};
}

