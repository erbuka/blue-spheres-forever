#include "BsfPch.h"

#include "Font.h"
#include "Log.h"
#include "Texture.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_freetype.h>
#undef STB_TRUETYPE_IMPLEMENTATION


namespace bsf
{

	static constexpr char s_FirstChar = ' ';
	static constexpr char s_LastChar = 'z';
	static constexpr uint32_t s_BitmapSize = 1024;


	struct Font::Impl
	{

		static Impl* Pack(std::string_view fileName, float fontSize, uint32_t fromCharacter, uint32_t toCharacter, uint32_t packWidth, uint32_t packHeight)
		{
			std::ifstream is;

			// Read file data
			is.open(fileName, std::ios_base::binary);

			if (!is.good())
			{
				BSF_ERROR("Can't load the font file: {0}", fileName);
				return nullptr;
			}

			is.seekg(0, std::ios_base::end);
			auto fileSize = is.tellg();
			std::vector<unsigned char> fileData(fileSize);
			is.seekg(0);
			is.read((char*)fileData.data(), fileSize);
			is.close();

			// Init font info
			stbtt_fontinfo fontInfo;
			memset(&fontInfo, 0, sizeof(fontInfo));

			if (!stbtt_InitFont(&fontInfo, fileData.data(), 0))
			{
				BSF_ERROR("Cannot inititialize font data: {0}", fileName);
				return nullptr;
			}

			// Get font metrics
			float scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);
			int32_t unAscent, unDescent;
			stbtt_GetFontVMetrics(&fontInfo, &unAscent, &unDescent, 0);

			const float ascent = unAscent * scale;
			const float descent = unDescent * scale;
			const float fontHeight = ascent - descent;

			// Packing begin
			stbtt_pack_context packContext;

			std::vector<unsigned char> pixels(packWidth * packHeight);

			if (!stbtt_PackBegin(&packContext, pixels.data(), packWidth, packHeight, 0, 2, 0))
			{
				BSF_ERROR("Can't begin packing the font: {0}", fileName);
				return nullptr;
			}

			// Create character range
			const uint32_t numCharacters = toCharacter - fromCharacter + 1;
			stbtt_pack_range range;
			std::memset(&range, 0, sizeof(range));

			range.font_size = fontSize;
			range.first_unicode_codepoint_in_range = fromCharacter;
			range.num_chars = numCharacters;
			range.chardata_for_range = new stbtt_packedchar[numCharacters];

			// Pack range
			if (!stbtt_PackFontRanges(&packContext, fileData.data(), 0, &range, 1))
			{
				BSF_ERROR("Can't pack the range: {0}", fileName);
				return nullptr;
			}

			// Invert the Y-axis
			std::vector<unsigned char> invertedPixels(packWidth * packHeight);
			for (uint32_t row = 0; row < packHeight; ++row)
			{
				auto beg = pixels.begin() + (packHeight - row - 1) * packWidth;
				auto end = beg + packWidth;
				std::copy(beg, end, invertedPixels.begin() + (row * packWidth));
			}

			// Create rgba packed texture
			Impl* result = new Impl();

			std::vector<uint32_t> rgbaPixels(packWidth * packHeight);
			for (uint32_t i = 0; i < packWidth * packHeight; i++)
				rgbaPixels[i] = 0x00ffffff | (invertedPixels[i] << 24);

			result->m_FontTex = MakeRef<Texture2D>(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
			result->m_FontTex->SetPixels(rgbaPixels.data(), packWidth, packHeight);
			result->m_FontTex->SetFilter(TextureFilter::LinearMipmapLinear, TextureFilter::Linear);

			// Create character info
			for (uint32_t i = 0; i < numCharacters; i++)
			{
				uint32_t codePoint = fromCharacter + i;
				auto& c = range.chardata_for_range[i];
				GlyphInfo charData;

				charData.Advance = (float)c.xadvance / fontHeight;
				charData.Min = { c.xoff / fontHeight, -(c.yoff + c.y1 - c.y0 + descent) / fontHeight };
				charData.Max = { (c.xoff + c.x1 - c.x0) / fontHeight, -(c.yoff + descent) / fontHeight };
				charData.UvMin = { (float)c.x0 / packWidth, (packHeight - c.y1) / (float)packHeight };
				charData.UvMax = { (float)c.x1 / packWidth, (packHeight - c.y0) / (float)packHeight };

				result->m_GlyphData[codePoint] = charData;
			}


			// Clean up
			stbtt_PackEnd(&packContext);
			delete[] range.chardata_for_range;

			return result;
		}

		Font::Impl() {}

		const Ref<Texture2D> GetTexture() { return m_FontTex; }

		const GlyphInfo& GetGlyphInfo(uint32_t codePoint) const
		{
			static GlyphInfo nullInfo;
			auto it = m_GlyphData.find(codePoint);
			if (it == m_GlyphData.end())
				throw std::runtime_error("Glyph not found");
			return it->second;
		}

		float GetStringWidth(std::string_view s) const
		{
			float result = 0.0f;

			for (uint32_t i = 0; i < s.size(); i++)
				result += GetGlyphInfo((uint32_t)s[i]).Advance;

			return result;

		}


		Ref<Texture2D> m_FontTex = nullptr;
		std::unordered_map<uint32_t, GlyphInfo> m_GlyphData;

	};


	
	Font::Font(std::string_view fileName, float fontSize)
	{
		m_Impl = std::unique_ptr<Impl>(Impl::Pack(fileName, fontSize, s_FirstChar, s_LastChar, s_BitmapSize, s_BitmapSize));
	}

	const GlyphInfo& Font::GetGlyphInfo(uint32_t c) const
	{
		return m_Impl->GetGlyphInfo(c);
	}

	const Ref<Texture2D>& Font::GetTexture() const
	{
		return m_Impl->m_FontTex;
	}

	float Font::GetStringWidth(std::string_view s) const
	{
		return m_Impl->GetStringWidth(s);
	}
}