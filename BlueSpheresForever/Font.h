#pragma once

#include "Common.h"

#include <glm/glm.hpp>

namespace bsf
{

	struct Glyph
	{
		glm::vec2 Min, Max;
		glm::vec2 UvMin, UvMax;
		float Advance;
	};

	class Font
	{
	public:
		Font(const std::string& fileName);

	private:
		struct Impl;
		Ref<Impl> m_Impl;
	};
}
