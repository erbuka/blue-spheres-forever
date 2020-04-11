#include "MatrixStack.h"

#include <glm/ext.hpp>
#include <glm/gtx/transform.hpp>

namespace bsf
{
	MatrixStack::MatrixStack() 
	{
		m_Stack.push(glm::identity<glm::mat4>());
	}
	
	MatrixStack::operator glm::mat4()
	{
		return m_Stack.top();
	}

	void MatrixStack::Push()
	{
		m_Stack.push(m_Stack.top());
	}
	void MatrixStack::Pop()
	{
		assert(m_Stack.size() > 0);
		m_Stack.pop();
	}
	void MatrixStack::LoadIdentity()
	{
		m_Stack.top() = glm::identity<glm::mat4>();
	}
	void MatrixStack::Translate(const glm::vec3& translate)
	{
		m_Stack.top() *= glm::translate(translate);

	}
	void MatrixStack::Rotate(const glm::vec3& axis, float angle)
	{
		m_Stack.top() *= glm::rotate(angle, axis);
	}
	void MatrixStack::Scale(const glm::vec3& scale)
	{
		m_Stack.top() *= glm::scale(scale);
	}

	void MatrixStack::Multiply(const glm::mat4& mat)
	{
		m_Stack.top() *= mat;
	}

	void MatrixStack::Perspective(float fovY, float aspect, float zNear, float zFar)
	{
		m_Stack.top() *= glm::perspective(fovY, aspect, zNear, zFar);
	}

	void MatrixStack::Orthographic(float left, float right, float bottom, float top, float zNear, float zFar)
	{
		m_Stack.top() *= glm::ortho(left, right, bottom, top, zNear, zFar);
	}
	void MatrixStack::LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
	{
		m_Stack.top() *= glm::lookAt(position, target, up);
	}
}