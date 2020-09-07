#pragma once

#include "Common.h"
#include "Asset.h"

#include <glm/glm.hpp>

namespace bsf
{

	struct GlyphInfo
	{
		glm::vec2 Min, Max;
		glm::vec2 UvMin, UvMax;
		float Advance;
	};

	class Font : public Asset
	{
	public:
		Font(std::string_view fileName, float fontSize);
		const GlyphInfo& GetGlyphInfo(uint32_t c) const;
		const Ref<Texture2D>& GetTexture() const;
		float GetStringWidth(std::string_view s) const;

	private:
		struct Impl;
		Ref<Impl> m_Impl;
	};



}
