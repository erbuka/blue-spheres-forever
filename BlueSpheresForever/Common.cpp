#include "Common.h"
#include "Log.h"
#include "Texture.h";

#include <glad/glad.h>

namespace bsf
{

	uint32_t LoadShader(const std::string& shaderSource, GLenum type)
	{
		// Create an empty vertex shader handle
		GLuint shader = glCreateShader(type);

		// Send the vertex shader source code to GL
		// Note that std::string's .c_str is NULL character terminated.
		const GLchar* source = (const GLchar*)shaderSource.c_str();
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

	uint32_t LoadProgram(const std::string& vertexSource, const std::string& fragmentSource)
	{


		// Vertex and fragment shaders are successfully compiled.
		// Now time to link them together into a program.
		// Get a program object.
		GLuint program = glCreateProgram();

		GLuint vertexShader = LoadShader(vertexSource, GL_VERTEX_SHADER);
		GLuint fragmentShader = LoadShader(fragmentSource, GL_FRAGMENT_SHADER);

		// Attach our shaders to our program
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);

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

			// We don't need the shader anymore.
			glDeleteShader(fragmentShader);
			// Either of them. Don't leak shaders.
			glDeleteShader(vertexShader);

			// Use the infoLog as you see fit.
			BSF_ERROR("There were some errors while linking the program: {0}", infoLog.data());

			// In this simple program, we'll just leave
			return 0;
		}

		// Always detach shaders after a successful link.
		glDetachShader(program, vertexShader);
		glDetachShader(program, fragmentShader);

		return program;
	}

	Ref<Texture2D> CreateCheckerBoard(std::array<uint32_t, 2> colors)
	{
		std::array<uint32_t, 4> data = {
			colors[0], colors[1],
			colors[1], colors[0]
		};

		return MakeRef<Texture2D>(2, 2, data.data());
	}

}

