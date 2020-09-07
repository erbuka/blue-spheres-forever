#include "BsfPch.h"

#include "ShaderProgram.h"
#include "Log.h"
#include "Common.h"
#include "Texture.h"


#define UNIFORM_IMPL(type, varType, size) \
	void ShaderProgram::Uniform ## size ## type ## v(const std::string& name, uint32_t count, const varType * ptr) { \
		BSF_GLCALL(glUniform ## size ## type ## v(GetUniformLocation(name), count, ptr)); \
	}


namespace bsf
{

	static std::unordered_map<ShaderType, GLenum> s_glShaderType = {
		{ ShaderType::Vertex, GL_VERTEX_SHADER },
		{ ShaderType::Geometry, GL_GEOMETRY_SHADER },
		{ ShaderType::Fragment, GL_FRAGMENT_SHADER }
	};


	static uint32_t LoadShader(ShaderType type, std::string_view shaderSource)
	{
		// Create an empty vertex shader handle
		GLuint shader = glCreateShader(s_glShaderType[type]);

		// Send the vertex shader source code to GL
		// Note that std::string's .c_str is NULL character terminated.
		const GLchar* source = (const GLchar*)shaderSource.data();
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
			BSF_ERROR("**** Code ****\n{0}**** End Code ****", shaderSource);

			return 0;
		}

		return shader;
	}

	static uint32_t LoadProgram(const std::initializer_list<std::pair<ShaderType, std::string_view>>& shaderSources)
	{

		GLuint program = glCreateProgram();

		std::vector<uint32_t> shaders;
		shaders.reserve(shaderSources.size());

		for (const auto& src : shaderSources)
			shaders.push_back(LoadShader(src.first, src.second));

		for (auto s : shaders)
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


	Ref<ShaderProgram> ShaderProgram::FromFile(std::string_view vertex, std::string_view fragment, const std::initializer_list<std::string_view>& defines)
	{

		auto vsSource = ReadTextFile(vertex);
		auto fsSource = ReadTextFile(fragment);

		InjectDefines(vsSource, defines);
		InjectDefines(fsSource, defines);

		return MakeRef<ShaderProgram>(vsSource, fsSource);
	}

	ShaderProgram::ShaderProgram(std::string_view vertexSource, std::string_view fragmentSource) :
		ShaderProgram({ { ShaderType::Vertex, vertexSource }, { ShaderType::Fragment, fragmentSource } })
	{
	}

	ShaderProgram::ShaderProgram(const std::initializer_list<std::pair<ShaderType, std::string_view>>& sources)
	{
		static constexpr std::array<GLenum, 2> samplerTypes = {
			GL_SAMPLER_2D,
			GL_SAMPLER_CUBE
		};

		BSF_INFO("Loading shader program");

		m_Id = LoadProgram(sources);

		uint32_t uniformCount;
		BSF_GLCALL(glGetProgramiv(m_Id, GL_ACTIVE_UNIFORMS, (int32_t*)&uniformCount));

		uint32_t textureUnit = 0;

		for (uint32_t i = 0; i < uniformCount; i++)
		{
			std::array<char, 50> buffer;
			int32_t size;
			uint32_t type;
			glGetActiveUniform(m_Id, i, buffer.size(), nullptr, &size, &type, buffer.data());

			uint32_t location = glGetUniformLocation(m_Id, buffer.data());


			if(std::find(samplerTypes.begin(), samplerTypes.end(), type) != samplerTypes.end())
			{
				m_UniformInfo[buffer.data()] = { std::string(buffer.data()), location, textureUnit++ };
			}
			else
			{
				m_UniformInfo[buffer.data()] = { std::string(buffer.data()), location, 0 };
			}

		}


		std::vector<UniformInfo> infos;
		std::transform(m_UniformInfo.begin(), m_UniformInfo.end(), std::back_inserter(infos), [](const auto& i) { return i.second; });
		std::sort(infos.begin(), infos.end(), [](const auto& a, const auto& b) -> bool { return a.Location < b.Location; });
		
		for (const auto& info : infos)
			BSF_INFO("\t{1} - {0}, textureUnit: {2}", info.Name, info.Location, info.TextureUnit);

	}

	ShaderProgram::~ShaderProgram()
	{
		BSF_GLCALL(glDeleteProgram(m_Id));
	}

	int32_t ShaderProgram::GetUniformLocation(const std::string& name)
	{

		auto it = m_UniformInfo.find(name);

		if (it == m_UniformInfo.end())
		{
			BSF_ERROR("Can't find uniform '{0}'", name);
			return -1;
		}

		return it->second.Location;


		/*
		auto cachedLoc = m_UniformLocations.find(name);

		if (cachedLoc != m_UniformLocations.end())
		{
			return cachedLoc->second;
		}

		uint32_t location = BSF_GLCALL(glGetUniformLocation(m_Id, name.c_str()));

		if (location != -1)
			m_UniformLocations[name] = location;

		return location;
		*/
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
	UNIFORM_IMPL(ui, uint32_t, 1);

	UNIFORM_IMPL(f, float, 1);
	UNIFORM_IMPL(f, float, 2);
	UNIFORM_IMPL(f, float, 3);
	UNIFORM_IMPL(f, float, 4);



	void ShaderProgram::InjectDefines(std::string& source, const std::initializer_list<std::string_view>& defines)
	{
		if (defines.size() > 0)
		{
			std::string definesStr;

			for (const auto& def : defines)
				definesStr += Format("#define %s\n", def.data());

			auto pos = source.find("#version");
				
			if (pos != std::string::npos)
			{
				pos = source.find("\n", pos);
				source.insert(pos + 1, definesStr);
			}
			else
			{
				source += definesStr;
			}
		}

	}

}

#undef UNIFORM_IMPL