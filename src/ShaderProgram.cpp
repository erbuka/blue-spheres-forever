#include "BsfPch.h"

#include "ShaderProgram.h"
#include "Log.h"
#include "Common.h"
#include "Texture.h"
#include "Table.h"



#define UNIFORM_IMPL(type, varType, size) \
	void ShaderProgram::Uniform ## size ## type ## v(uint64_t hash, uint32_t count, const varType * ptr) { \
		BSF_GLCALL(glUniform ## size ## type ## v(GetUniformLocation(hash), count, ptr)); \
	}


namespace bsf
{

	static constexpr Table<3, ShaderType, GLenum> s_glShaderType = {
		std::make_tuple(ShaderType::Vertex, GL_VERTEX_SHADER),
		std::make_tuple(ShaderType::Geometry, GL_GEOMETRY_SHADER),
		std::make_tuple(ShaderType::Fragment, GL_FRAGMENT_SHADER)
	};


	static uint32_t LoadShader(ShaderType type, std::string_view shaderSource)
	{
		// Create an empty vertex shader handle
		GLuint shader = glCreateShader(s_glShaderType.Get<0, 1>(type));

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
		BSF_INFO("Loading shader from file ({0}, {1})", vertex, fragment);

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


			const auto hash = Hash(buffer.data());

			if(std::find(samplerTypes.begin(), samplerTypes.end(), type) != samplerTypes.end())
			{
				m_UniformInfo[hash] = { std::string(buffer.data()), location, textureUnit++ };
			}
			else
			{
				m_UniformInfo[hash] = { std::string(buffer.data()), location, 0 };
			}

		}


		std::vector<UniformInfo> infos;
		std::transform(m_UniformInfo.begin(), m_UniformInfo.end(), std::back_inserter(infos), [](const auto& i) { return i.second; });
		std::sort(infos.begin(), infos.end(), [](const auto& a, const auto& b) -> bool { return a.Location < b.Location; });
		
		for (const auto& info : infos)
			BSF_DEBUG("\t{1} - {0}, textureUnit: {2}", info.Name, info.Location, info.TextureUnit);

	}

	ShaderProgram::~ShaderProgram()
	{
		BSF_GLCALL(glDeleteProgram(m_Id));
	}

	int32_t ShaderProgram::GetUniformLocation(uint64_t hash) const
	{

		auto it = m_UniformInfo.find(hash);

		if (it == m_UniformInfo.end())
		{
			BSF_ERROR("Can't find uniform '{0}'", hash);
			return -1;
		}

		return it->second.Location;

	}

	void ShaderProgram::UniformMatrix4f(uint64_t hash, const glm::mat4& matrix)
	{
		BSF_GLCALL(glUniformMatrix4fv(GetUniformLocation(hash), 1, GL_FALSE, glm::value_ptr(matrix)));
	}

	void ShaderProgram::UniformMatrix4fv(uint64_t hash, size_t count, const float* ptr)
	{
		BSF_GLCALL(glUniformMatrix4fv(GetUniformLocation(hash), count, GL_FALSE, ptr));
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
				definesStr += fmt::format("#define {0}\n", def.data());

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