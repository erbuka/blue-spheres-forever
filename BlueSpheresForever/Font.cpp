#include "BsfPch.h"

#include "Font.h"
#include "Log.h"
#include "Texture.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_freetype.h>
#undef STB_TRUETYPE_IMPLEMENTATION


namespace bsf
{

	static constexpr char s_FirstChar = 'A';
	static constexpr char s_LastChar = 'z';
	static constexpr uint32_t s_CharCount = s_LastChar - s_FirstChar;
	static constexpr uint32_t s_FontSize = 64;
	static constexpr uint32_t s_BitmapSize = 512;

	struct Font::Impl {
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
			m_BitmapData = new unsigned char[s_BitmapSize * s_BitmapSize];
			stbtt_bakedchar* bakedChars = new stbtt_bakedchar[s_CharCount];
			stbtt_BakeFontBitmap(m_FontFileData, 0, s_FontSize, m_BitmapData, s_BitmapSize, s_BitmapSize, s_FirstChar, s_CharCount, bakedChars);
			
			// Create glyphs
			CreateGlyphs(bakedChars);

			// Delete baked characters
			delete[] bakedChars;

			// Create font texture
			m_Texture = MakeRef<Texture2D>();
			m_Texture->Bind(0);



		}

		~Impl()
		{
			if(m_FontFileData)
				delete[] m_FontFileData;
			
			if(m_BitmapData)
				delete[] m_BitmapData;

		}

	private:
		unsigned char* m_FontFileData = nullptr;
		unsigned char* m_BitmapData = nullptr;
		std::vector<Glyph> m_Glyphs;
		std::shared_ptr<Texture2D> m_Texture = nullptr;

		void CreateGlyphs(stbtt_bakedchar* chars)
		{
			m_Glyphs.reserve(s_CharCount);

			for (uint32_t i = 0; i < s_CharCount; i++)
			{
				const auto& c = chars[i];
				m_Glyphs.push_back({
					glm::vec2(c.xoff / s_FontSize, c.yoff / s_FontSize),  // Min
					glm::vec2((c.xoff + c.x1 - c.x0) / s_FontSize, (c.yoff + c.y1 - c.y0) / s_FontSize),  // Max
					glm::vec2(c.x0 / s_BitmapSize, c.y0 / s_BitmapSize), // UvMin
					glm::vec2(c.x1 / s_BitmapSize, c.y1 / s_BitmapSize), // UvMax
					c.xadvance / s_FontSize
				});
			}
		}


	};

	Font::Font(const std::string& fileName)
	{
		m_Impl = MakeRef<Impl>(fileName);
	}
}