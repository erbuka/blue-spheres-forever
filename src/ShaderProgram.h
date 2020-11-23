#pragma once

#include "Asset.h"
#include "Ref.h"

#include <glm/glm.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <array>
#include <tuple>
#include <initializer_list>

#define UNIFORM_DECL(type, varType, size) void Uniform ## size ## type ## v(const std::string& name, uint32_t count, const varType * ptr)
#define UNIFORM1_INL(type, varType, size) \
	inline void Uniform ## size ## type(const std::string& name, std::array<varType,size> v) { \
		Uniform ## size ## type ## v(name, 1, v.data()); \
	}

namespace bsf
{
	class Texture;


	enum class ShaderType
	{
		Vertex,
		Geometry,
		Fragment
	};

	class ShaderProgram: public Asset
	{


	public:


		static Ref<ShaderProgram> FromFile(std::string_view vertex, std::string_view fragment, const std::initializer_list<std::string_view>& defines = {});

		ShaderProgram(std::string_view vertexSource, std::string_view fragmentSource);
		ShaderProgram(const std::initializer_list<std::pair<ShaderType, std::string_view>>& sources);

		ShaderProgram(ShaderProgram&) = delete;
		ShaderProgram(ShaderProgram&&) = delete;

		~ShaderProgram();

		int32_t GetUniformLocation(const std::string& name) const;

		void UniformMatrix4f(const std::string& name, const glm::mat4& matrix);
		void UniformMatrix4fv(const std::string& name, size_t count, const float* ptr);

		void Use();

		template<typename T>
		void UniformTexture(const std::string name, const Ref<T>& texture)
		{
			static_assert(std::is_base_of_v<Texture, T>);
			
			auto info = m_UniformInfo.find(name);

			if (info != m_UniformInfo.end())
			{
				uint32_t texUnit = info->second.TextureUnit;
				Uniform1i(name, { (int32_t)texUnit });
				texture->Bind(texUnit);
				return;
			}
			
			BSF_ERROR("Couldn't find uniform texture: {0}", name);

		}

		uint32_t GetId() const { return m_Id; }
		
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

		struct UniformInfo
		{
			std::string Name;
			uint32_t Location;
			uint32_t TextureUnit;
		};

		static void InjectDefines(std::string& source, const std::initializer_list<std::string_view>& defines);

		uint32_t m_Id;
		std::unordered_map<std::string, UniformInfo> m_UniformInfo;
	};

}

#undef UNIFORM_DECL
#undef UNIFORM1_INL
