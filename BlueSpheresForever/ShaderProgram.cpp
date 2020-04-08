#include "ShaderProgram.h"
#include "Common.h"
#include "Log.h"

#include <glm/ext.hpp>

#include <glad/glad.h>


#define UNIFORM_IMPL(type, varType, size) \
	void ShaderProgram::Uniform ## size ## type ## v(const std::string& name, uint32_t count, varType * ptr) { \
		BSF_GLCALL(glUniform ## size ## type ## v(GetUniformLocation(name), count, ptr)); \
	}


namespace bsf
{


	ShaderProgram::ShaderProgram(const std::string& vertexSource, const std::string& fragmentSource)
	{
		m_Id = LoadProgram(vertexSource, fragmentSource);
	}
	ShaderProgram::~ShaderProgram()
	{
		BSF_GLCALL(glDeleteProgram(m_Id));
	}

	int32_t ShaderProgram::GetUniformLocation(const std::string& name)
	{
		auto cachedLoc = m_UniformLocations.find(name);

		if (cachedLoc != m_UniformLocations.end())
		{
			return cachedLoc->second;
		}

		uint32_t location = BSF_GLCALL(glGetUniformLocation(m_Id, name.c_str()));

		if (location != -1)
			m_UniformLocations[name] = location;

		return location;

	}

	void ShaderProgram::UniformMatrix4f(const std::string& name, const glm::mat4& matrix)
	{
		BSF_GLCALL(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix)));
	}

	void ShaderProgram::Use()
	{
		BSF_GLCALL(glUseProgram(m_Id));
	}

	UNIFORM_IMPL(i, int32_t, 1);

	UNIFORM_IMPL(f, float, 1);
	UNIFORM_IMPL(f, float, 2);
	UNIFORM_IMPL(f, float, 3);
	UNIFORM_IMPL(f, float, 4);


}

#undef UNIFORM_IMPL