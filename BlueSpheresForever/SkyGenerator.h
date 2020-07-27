#pragma once

#include <glm/glm.hpp>

#include "Common.h"
#include "Asset.h"

namespace bsf
{
	class TextureCube;
	class VertexArray;
	class ShaderProgram;

	class SkyGenerator : public Asset
	{
	public:

		struct Options
		{
			uint32_t Size;
			glm::vec3 BaseColor0, BaseColor1;
		};

		SkyGenerator();

		Ref<TextureCube> Generate(const Options& options);
	private:
		Ref<VertexArray> m_vaCube, m_vaBillboard;
		Ref<ShaderProgram> m_pGenBg, m_pGenStars;
	};
}

