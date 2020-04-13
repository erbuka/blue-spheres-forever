#include "Common.h"
#include "Log.h"
#include "Texture.h";

#include <unordered_map>
#include <initializer_list>

#include <glad/glad.h>

namespace bsf
{

	static std::unordered_map<ShaderType, GLenum> s_glShaderType = {
		{ ShaderType::Vertex, GL_VERTEX_SHADER },
		{ ShaderType::Geometry, GL_GEOMETRY_SHADER },
		{ ShaderType::Fragment, GL_FRAGMENT_SHADER }
	};


	uint32_t LoadShader(const ShaderSource& shaderSource)
	{
		// Create an empty vertex shader handle
		GLuint shader = glCreateShader(s_glShaderType[shaderSource.Type]);

		// Send the vertex shader source code to GL
		// Note that std::string's .c_str is NULL character terminated.
		const GLchar* source = (const GLchar*)shaderSource.Source.c_str();
		glShaderSource(shader, 1, &source, 0);

		// Compile the vertex shader
		glCompileShader(shader);

		GLint isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

			// We don't need the shader anymore.
			glDeleteShader(shader);

			// Use the infoLog as you see fit.
			BSF_ERROR("There were some errors while compiling the shader: {0}", infoLog.data());

			// In this simple program, we'll just leave
			return 0;
		}

		return shader;
	}

	uint32_t LoadProgram(const std::initializer_list<ShaderSource>& shaderSources)
	{


		// Vertex and fragment shaders are successfully compiled.
		// Now time to link them together into a program.
		// Get a program object.
		GLuint program = glCreateProgram();
		
		std::vector<uint32_t> shaders(shaderSources.size());
		
		uint32_t i = 0;
		for (auto src = shaderSources.begin(); src != shaderSources.end(); ++src, ++i)
			shaders[i] = LoadShader(*src);
		
		for(uint32_t s: shaders)
			glAttachShader(program, s);

		// Link our program
		glLinkProgram(program);

		// Note the different functions here: glGetProgram* instead of glGetShader*.
		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);


			for (uint32_t s : shaders)
				glDetachShader(program, s);

			// Use the infoLog as you see fit.
			BSF_ERROR("There were some errors while linking the program: {0}", infoLog.data());

			// In this simple program, we'll just leave
			return 0;
		}

		// Always detach shaders after a successful link.
		for (uint32_t s : shaders)
			glDetachShader(program, s);

		return program;
	}

	Ref<Texture2D> CreateCheckerBoard(const std::array<uint32_t, 2>& colors)
	{
		std::array<uint32_t, 4> data = {
			colors[0], colors[1],
			colors[1], colors[0]
		};

		return MakeRef<Texture2D>(2, 2, data.data());
	}

}

