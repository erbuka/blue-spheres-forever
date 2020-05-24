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
	static constexpr uint32_t s_CharCount = s_LastChar - s_FirstChar + 1;
	static constexpr uint32_t s_FontSize = 128;
	static constexpr uint32_t s_BitmapSize = 512;

	struct Font::Impl {

		unsigned char* m_FontFileData = nullptr;
		std::vector<GlyphInfo> m_Glyphs;
		std::shared_ptr<Texture2D> m_Texture = nullptr;

		Impl(const std::string& fileName)
		{
			std::ifstream is;

			// Load the font file first
			is.open(fileName, std::ios_base::binary | std::ios_base::in);

			if (!is.is_open())
			{
				BSF_ERROR("Can't open file: {0}", fileName);
				return;
			}

			is.seekg(0, std::ios_base::end);
			auto end = is.tellg();
			is.seekg(0);

			m_FontFileData = new unsigned char[end];

			is.read(reinterpret_cast<char*>(m_FontFileData), end);

			is.close();
			
			// Bake the font
			auto bitmap = new unsigned char[s_BitmapSize * s_BitmapSize];
			std::memset(bitmap, 0, s_BitmapSize * s_BitmapSize);

			// Create glyphs
			stbtt_bakedchar* bakedChars = new stbtt_bakedchar[s_CharCount];
			auto res = stbtt_BakeFontBitmap(m_FontFileData, 0, s_FontSize, bitmap, s_BitmapSize, s_BitmapSize, s_FirstChar, s_CharCount, bakedChars);
			CreateGlyphs(bakedChars);
			delete[] bakedChars;

			// Create font texture
			// Convert to rgba
			std::vector<uint32_t> pixels(s_BitmapSize * s_BitmapSize);
			for (uint32_t i = 0; i < s_BitmapSize*s_BitmapSize; i++)
				pixels[i] = (bitmap[i] << 24) | 0xffffff;
			delete[] bitmap;


			m_Texture = MakeRef<Texture2D>();
			m_Texture->SetPixels(pixels.data(), s_BitmapSize, s_BitmapSize, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
			m_Texture->SetFilter(TextureFilter::Linear, TextureFilter::Linear);
		}

		~Impl()
		{
			if(m_FontFileData)
				delete[] m_FontFileData;
		}


		void CreateGlyphs(stbtt_bakedchar* chars)
		{
			m_Glyphs.reserve(s_CharCount);

			for (uint32_t i = 0; i < s_CharCount; i++)
			{

				const auto& c = chars[i];
				
				m_Glyphs.push_back({
					glm::vec2(c.xoff / s_FontSize, -c.yoff / s_FontSize),  // Min
					glm::vec2((c.xoff + c.x1 - c.x0) / s_FontSize, -(c.yoff + c.y1 - c.y0) / s_FontSize),  // Max
					glm::vec2(c.x0 / float(s_BitmapSize), c.y0 / float(s_BitmapSize)), // UvMin
					glm::vec2(c.x1 / float(s_BitmapSize), c.y1 / float(s_BitmapSize)), // UvMax
					c.xadvance / s_FontSize
				});

			}
		}


	};

	Font::Font(const std::string& fileName)
	{
		m_Impl = MakeRef<Impl>(fileName);
	}

	const GlyphInfo& Font::GetGlyphInfo(char c) const
	{
		assert(c >= s_FirstChar && c <= s_LastChar);
		return m_Impl->m_Glyphs[c - s_FirstChar];
	}

	const Ref<Texture2D>& Font::GetTexture() const
	{
		return m_Impl->m_Texture;
	}

	float Font::GetStringWidth(const std::string& s) const
	{
		float result = 0.0f;

		for (uint32_t i = 0; i < s.size(); i++)
			result += GetGlyphInfo(s[i]).Advance;

		return result;

	}
}