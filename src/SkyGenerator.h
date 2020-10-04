#pragma once

#include <glm/glm.hpp>

#include "Ref.h"
#include "Asset.h"
#include "Texture.h"

// TODO Rename shaders and delete obsolete ones
namespace bsf
{
	class TextureCube;
	class VertexArray;
	class ShaderProgram;
	class CubeCamera;
	class SkyGenerator;

	class Sky
	{
	public:
		void ApplyMatrix(const glm::mat4& matrix);

		const Ref<TextureCube>& GetEnvironment();
		const Ref<TextureCube>& GetIrradiance();

	private:

		Ref<ShaderProgram> m_pSky;
		Ref<TextureCube> m_SrcEnv, m_SrcIrr;
		Ref<CubeCamera> m_Env, m_Irr;
		std::vector<std::array<glm::vec3, 2>> m_Vertices;
		Ref<VertexArray> m_VertexArray;

		Sky(const Ref<TextureCube>& env, const Ref<TextureCube>& irradiance);

		void Update(Ref<CubeCamera>& camera, Ref<TextureCube>& source);

		friend class SkyGenerator;
	};

	class SkyGenerator : public Asset
	{
	public:
		
		struct Options
		{
			uint32_t Size;
			glm::vec3 BaseColor;
			glm::vec3 StarsColor;
		};

		SkyGenerator();

		Ref<Sky> Generate(const Options& options);

	private:

		Ref<TextureCube> GenerateEnvironment(const Options& options);
		Ref<TextureCube> GenerateIrradiance(const Ref<TextureCube>& sky, uint32_t size);

		Ref<VertexArray> m_vaCube;
		Ref<ShaderProgram> m_pGenEnv, m_pGenIrr;
	};

	Ref<Sky> GenerateDefaultSky();
}

