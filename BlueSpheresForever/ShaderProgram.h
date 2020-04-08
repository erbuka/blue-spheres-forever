#pragma once

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>


#define UNIFORM_DECL(type, varType, size) void Uniform ## size ## type ## v(const std::string& name, uint32_t count, varType * ptr)

namespace bsf
{


	class ShaderProgram
	{
	public:

		ShaderProgram(const std::string& vertexSource, const std::string& fragmentSource);
		~ShaderProgram();

		int32_t GetUniformLocation(const std::string& name);

		void UniformMatrix4f(const std::string& name, const glm::mat4& matrix);

		void Use();
		
		UNIFORM_DECL(i, int32_t, 1);

		UNIFORM_DECL(f, float, 1);
		UNIFORM_DECL(f, float, 2);
		UNIFORM_DECL(f, float, 3);
		UNIFORM_DECL(f, float, 4);


	private:
		uint32_t m_Id;
		std::unordered_map<std::string, int32_t> m_UniformLocations;
	};

}

#undef UNIFORM_DECL
#undef UNIFORM1_DECL