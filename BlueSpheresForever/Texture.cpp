#include "Texture.h"
#include "Log.h"

#include <lodepng.h>
#include <glad/glad.h>
#include <vector>


namespace bsf
{

	Texture::Texture(uint32_t width, uint32_t height, uint32_t pixelsPerUnit, void* pixels) :
		m_PixelsPerUnit(pixelsPerUnit)
	{
		uint32_t size = width * height;

		BSF_GLCALL(glGenTextures(1, &m_ID));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_ID));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels));

	}

	Texture::Texture(const std::string& fileName, uint32_t pixelsPerUnit) :
		m_PixelsPerUnit(pixelsPerUnit)
	{
		Load(fileName);
	}

	Texture::Texture(Texture&& other) noexcept :
		m_Width(0), m_Height(0), m_ID(0)
	{
		std::swap(m_ID, other.m_ID);
		std::swap(m_Width, other.m_Width);
		std::swap(m_Height, other.m_Height);
	}

	Texture::~Texture()
	{
		if (m_ID > 0)
		{
			BSF_GLCALL(glDeleteTextures(1, &m_ID));
			m_ID = 0;
		}
	}

	bool Texture::Load(const std::string& fileName)
	{
		std::vector<unsigned char> data, flippedData;
		unsigned error = lodepng::decode(data, m_Width, m_Height, fileName);

		// If there's an error, display it.
		if (error != 0) {
			BSF_ERROR("There was en error while loading file '{0}': {1}", fileName, lodepng_error_text(error));
			return false;
		}

		// Flip y pixels
		flippedData.resize(data.size());

		for (uint32_t y = 0; y < m_Height; y++)
		{
			std::memcpy(
				flippedData.data() + y * m_Width * sizeof(uint32_t),
				data.data() + (m_Height - y - 1) * m_Width * sizeof(uint32_t),
				m_Width * sizeof(uint32_t));
		}


		BSF_GLCALL(glGenTextures(1, &m_ID));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_ID));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, flippedData.data()));

		return true;
	}

}