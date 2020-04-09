#include "Texture.h"
#include "Log.h"

#include <lodepng.h>
#include <glad/glad.h>
#include <vector>
#include <unordered_map>


namespace bsf
{

	static std::unordered_map<TextureFilter, GLenum> s_glFilter = {
		{ TextureFilter::MinFilter, GL_TEXTURE_MIN_FILTER },
		{ TextureFilter::MagFilter, GL_TEXTURE_MAG_FILTER }
	};

	static std::unordered_map<TextureFilterMode, GLenum> s_glMode = {
		{ TextureFilterMode::Linear, GL_LINEAR },
		{ TextureFilterMode::LinearMipmapLinear, GL_LINEAR_MIPMAP_LINEAR },
		{ TextureFilterMode::Nearest, GL_NEAREST },
	};

	static std::unordered_map<TextureCubeFace, GLenum> s_glTexCubeFace = {
		{ TextureCubeFace::Front, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z  },
		{ TextureCubeFace::Back, GL_TEXTURE_CUBE_MAP_POSITIVE_Z  },
		{ TextureCubeFace::Left, GL_TEXTURE_CUBE_MAP_NEGATIVE_X  },
		{ TextureCubeFace::Right, GL_TEXTURE_CUBE_MAP_POSITIVE_X  },
		{ TextureCubeFace::Top, GL_TEXTURE_CUBE_MAP_POSITIVE_Y  },
		{ TextureCubeFace::Bottom, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y  },
	};

	Texture2D::Texture2D(uint32_t width, uint32_t height, const void* pixels)
	{
		uint32_t size = width * height;

		BSF_GLCALL(glGenTextures(1, &m_Id));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Id));
		BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels));

		Filter(TextureFilter::MinFilter, TextureFilterMode::Linear);
		Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
	}

	Texture2D::Texture2D(const std::string& fileName)
	{
		Load(fileName);
	}

	Texture2D::Texture2D(Texture2D&& other) noexcept	
	{
		m_Id = 0;
		m_Width = 0;
		m_Height = 0;
		std::swap(m_Id, other.m_Id);
		std::swap(m_Width, other.m_Width);
		std::swap(m_Height, other.m_Height);
	}


	void Texture2D::Filter(TextureFilter filter, TextureFilterMode mode)
	{

		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Id));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, s_glFilter[filter], s_glMode[mode]));

		if (filter == TextureFilter::MinFilter && mode == TextureFilterMode::LinearMipmapLinear)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}


	}

	void Texture2D::Bind(uint32_t textureUnit)
	{
		BSF_GLCALL(glActiveTexture(GL_TEXTURE0 + textureUnit));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Id));
	}

	bool Texture2D::Load(const std::string& fileName)
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


		BSF_GLCALL(glGenTextures(1, &m_Id));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Id));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, flippedData.data()));

		return true;
	}


	Texture::Texture() :
		m_Id(0), m_Width(0), m_Height(0)
	{
	}

	Texture::~Texture()
	{
		if (m_Id != 0)
		{
			BSF_GLCALL(glDeleteTextures(1, &m_Id));
		}
	}


	TextureCube::TextureCube(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;

		BSF_GLCALL(glGenTextures(1, &m_Id));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, m_Id));

		std::vector<uint32_t> pixels(width * height);
		std::fill(pixels.begin(), pixels.end(), 0xffffffff);

		SetPixels(TextureCubeFace::Front, pixels.data());
		SetPixels(TextureCubeFace::Back, pixels.data());
		SetPixels(TextureCubeFace::Left, pixels.data());
		SetPixels(TextureCubeFace::Right, pixels.data());
		SetPixels(TextureCubeFace::Top, pixels.data());
		SetPixels(TextureCubeFace::Bottom, pixels.data());

	}


	void TextureCube::SetPixels(TextureCubeFace face, const void* pixels)
	{
		BSF_GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, m_Id));
		BSF_GLCALL(glTexImage2D(s_glTexCubeFace[face], 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels));
	}

	void TextureCube::Bind(uint32_t textureUnit)
	{
		BSF_GLCALL(glActiveTexture(GL_TEXTURE0 + textureUnit));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, m_Id));
	}

	void TextureCube::Filter(TextureFilter filter, TextureFilterMode mode)
	{

		BSF_GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, m_Id));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, s_glFilter[filter], s_glMode[mode]));

		if (filter == TextureFilter::MinFilter && mode == TextureFilterMode::LinearMipmapLinear)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}
	}
}