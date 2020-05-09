#include "BsfPch.h"

#include "Texture.h"
#include "Log.h"

#include <lodepng.h>


namespace bsf
{

	static std::tuple<std::vector<unsigned char>, uint32_t, uint32_t> LoadPng(const std::string& fileName, bool flipY)
	{
		uint32_t width, height;

		std::vector<unsigned char> data, flippedData;
		unsigned error = lodepng::decode(data, width, height, fileName);

		// If there's an error, display it.
		if (error != 0) {
			BSF_ERROR("There was en error while loading file '{0}': {1}", fileName, lodepng_error_text(error));
			return { };
		}

		// Flip y pixels
		if (flipY)
		{
			flippedData.resize(data.size());

			for (uint32_t y = 0; y < height; y++)
			{
				std::memcpy(
					flippedData.data() + y * width * sizeof(uint32_t),
					data.data() + (height - y - 1) * width * sizeof(uint32_t),
					width * sizeof(uint32_t));
			}

			return { flippedData, width, height };
		}
		else
		{
			return { data, width, height };

		}
	}

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
		{ TextureCubeFace::Front, GL_TEXTURE_CUBE_MAP_POSITIVE_Z  },
		{ TextureCubeFace::Back, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,  },
		{ TextureCubeFace::Left, GL_TEXTURE_CUBE_MAP_NEGATIVE_X  },
		{ TextureCubeFace::Right, GL_TEXTURE_CUBE_MAP_POSITIVE_X  },
		{ TextureCubeFace::Top, GL_TEXTURE_CUBE_MAP_POSITIVE_Y  },
		{ TextureCubeFace::Bottom, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y  },
	};

	Texture2D::Texture2D()
	{
		BSF_GLCALL(glGenTextures(1, &m_Id));
	}


	Texture2D::Texture2D(uint32_t color) : Texture2D()
	{
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Id));
		BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color));

		Filter(TextureFilter::MinFilter, TextureFilterMode::Nearest);
		Filter(TextureFilter::MagFilter, TextureFilterMode::Nearest);
	}


	Texture2D::Texture2D(const std::string& fileName)
	{
		Load(fileName);
	}

	Texture2D::Texture2D(Texture2D&& other) noexcept	
	{
		m_Id = 0;
		std::swap(m_Id, other.m_Id);
	}

	void Texture2D::SetPixels(void* pixels, uint32_t width, uint32_t height, GLenum internalFormat, GLenum format, GLenum type)
	{
		Bind(0);
		BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, pixels));
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

	void Texture2D::SetAnisotropy(float value)
	{
		float max = 0.0f;
		BSF_GLCALL(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max));
		
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Id));
		BSF_GLCALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(max, value)));


	}

	void Texture2D::Bind(uint32_t textureUnit)
	{
		BSF_GLCALL(glActiveTexture(GL_TEXTURE0 + textureUnit));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Id));
	}

	bool Texture2D::Load(const std::string& fileName)
	{
		auto [pixels, width, height] = LoadPng(fileName, true);

		BSF_GLCALL(glGenTextures(1, &m_Id));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Id));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data()));

		return true;
	}


	Texture::Texture() :
		m_Id(0)
	{
	}

	Texture::~Texture()
	{
		if (m_Id != 0)
		{
			BSF_GLCALL(glDeleteTextures(1, &m_Id));
		}
	}


	TextureCube::TextureCube(uint32_t size, const std::string& front, const std::string& back, const std::string& left, const std::string& right, const std::string& bottom, const std::string& top) :
		TextureCube(size, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE)
	{
		static std::array<TextureCubeFace, 6> faces = {
			TextureCubeFace::Front,
			TextureCubeFace::Back,
			TextureCubeFace::Left,
			TextureCubeFace::Right,
			TextureCubeFace::Bottom,
			TextureCubeFace::Top
		};

		std::array<std::string, 6> files = { front, back, left, right, bottom, top };

		Initialize();
		
		for (uint32_t i = 0; i < files.size(); i++)
		{
			auto [pixels, width, height] = LoadPng(files[i], false);

			if (width != m_Size || height != m_Size)
			{
				BSF_ERROR("Invalid size for cubemap face");
				return;
			}

			SetPixels(faces[i], pixels.data());
		}
		

	}

	TextureCube::TextureCube(uint32_t size, GLenum internalFormat, GLenum format, GLenum type) :
		m_Size(size),
		m_InternalFormat(internalFormat),
		m_Format(format),
		m_Type(type)
	{
		Initialize();


		SetPixels(TextureCubeFace::Front, nullptr);
		SetPixels(TextureCubeFace::Back, nullptr);
		SetPixels(TextureCubeFace::Left, nullptr);
		SetPixels(TextureCubeFace::Right, nullptr);
		SetPixels(TextureCubeFace::Top, nullptr);
		SetPixels(TextureCubeFace::Bottom, nullptr);

	}

	void TextureCube::SetPixels(TextureCubeFace face, const void* pixels)
	{
		BSF_GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, m_Id));
		BSF_GLCALL(glTexImage2D(s_glTexCubeFace[face], 0, m_InternalFormat, m_Size, m_Size, 0, m_Format, m_Type, pixels));
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
			BSF_GLCALL(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
		}
	}
	void TextureCube::Initialize()
	{
		BSF_GLCALL(glGenTextures(1, &m_Id));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, m_Id));

		Filter(TextureFilter::MinFilter, TextureFilterMode::Linear);
		Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);

		BSF_GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));

	}
}