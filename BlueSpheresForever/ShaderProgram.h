#pragma once

#include "Common.h"

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <array>

#define UNIFORM_DECL(type, varType, size) void Uniform ## size ## type ## v(const std::string& name, uint32_t count, varType * ptr)
#define UNIFORM1_INL(type, varType, size) \
	inline void Uniform ## size ## type(const std::string& name, std::array<varType,size> v) { \
		Uniform ## size ## type ## v(name, 1, v.data()); \
	}

namespace bsf
{
	class Texture;

	class ShaderProgram
	{
	public:

		ShaderProgram(const std::string& vertexSource, const std::string& fragmentSource);
		ShaderProgram(const std::string& vertexSource, const std::string& geometrySource, const std::string& fragmentSource);
		~ShaderProgram();

		int32_t GetUniformLocation(const std::string& name);

		void UniformMatrix4f(const std::string& name, const glm::mat4& matrix);

		void Use();

		template<typename T>
		void UniformTexture(const std::string name, const Ref<T>& texture, uint32_t textureUnit)
		{
			static_assert(std::is_base_of_v<Texture, T>);
			Uniform1i(name, { (int32_t)textureUnit });
			texture->Bind(textureUnit);
		}

		uint32_t GetId() { return m_Id; }
		
		UNIFORM1_INL(i, int32_t, 1);
		UNIFORM_DECL(i, int32_t, 1);

		UNIFORM1_INL(ui, uint32_t, 1);
		UNIFORM_DECL(ui, uint32_t, 1);

		UNIFORM1_INL(f, float, 1);
		UNIFORM1_INL(f, float, 2);
		UNIFORM1_INL(f, float, 3);
		UNIFORM1_INL(f, float, 4);

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
#undef UNIFORM1_INL
