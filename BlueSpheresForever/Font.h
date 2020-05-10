#pragma once

#include "Common.h"

#include <glm/glm.hpp>

namespace bsf
{

	struct GlyphInfo
	{
		glm::vec2 Min, Max;
		glm::vec2 UvMin, UvMax;
		float Advance;
	};

	class Font
	{
	public:
		Font(const std::string& fileName);
		const GlyphInfo& GetGlyphInfo(char c) const;
		const Ref<Texture2D>& GetTexture() const;
		float GetStringWidth(const std::string& s) const;

	private:
		struct Impl;
		Ref<Impl> m_Impl;
	};
}
