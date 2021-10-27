#pragma once

#include "Asset.h"
#include "Ref.h"
#include "Log.h"

#include <glm/glm.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <array>
#include <tuple>
#include <initializer_list>

#define UNIFORM_DECL(type, varType, size) void Uniform ## size ## type ## v(uint64_t hash, uint32_t count, const varType * ptr)
#define UNIFORM1_INL(type, varType, size) \
	inline void Uniform ## size ## type(uint64_t hash, std::array<varType,size> v) { \
		Uniform ## size ## type ## v(hash, 1, v.data()); \
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

		int32_t GetUniformLocation(uint64_t hash) const;

		void UniformMatrix4f(uint64_t hash, const glm::mat4& matrix);
		void UniformMatrix4fv(uint64_t hash, size_t count, const float* ptr);

		void Use();

		template<typename T>
		void UniformTexture(uint64_t hash, const Ref<T>& texture)
		{
			static_assert(std::is_base_of_v<Texture, T>);

			auto info = m_UniformInfo.find(hash);

			if (info != m_UniformInfo.end())
			{
				uint32_t texUnit = info->second.TextureUnit;
				Uniform1i(hash, { (int32_t)texUnit });
				texture->Bind(texUnit);
				return;
			}

			BSF_ERROR("Couldn't find uniform texture: {0}", hash);

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
		std::unordered_map<uint64_t, UniformInfo> m_UniformInfo;
	};

}

#undef UNIFORM_DECL
#undef UNIFORM1_INL
