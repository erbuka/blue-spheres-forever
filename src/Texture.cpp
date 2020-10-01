#include "BsfPch.h"

#include "Texture.h"
#include "Log.h"
#include "Table.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include <stb_image.h>


namespace bsf
{

	std::tuple<std::vector<std::byte>, uint32_t, uint32_t> ImageLoad(std::string_view fileName, bool flipY)
	{
		uint32_t width, height;

		std::ifstream is;

		is.open(fileName, std::ios_base::binary);

		if (!is.is_open())
		{
			BSF_ERROR("There was en error while opengin file '{0}'", fileName);
			return { };
		}

		is.seekg(0, std::ios_base::end);
		auto length = is.tellg();
		is.seekg(0, std::ios_base::beg);

		std::vector<uint8_t> fileData;
		fileData.resize(length);

		is.read((char*)fileData.data(), length);

		BSF_INFO("Loading image from file: {0}", fileName.data());

		return ImageLoad(fileData.data(), fileData.size(), flipY);

	}

	std::tuple<std::vector<std::byte>, uint32_t, uint32_t> ImageLoad(const void* ptr, size_t length, bool flipY)
	{
		constexpr size_t channels = 4;
		int32_t width, height;
		stbi_set_flip_vertically_on_load(flipY);
			
		std::byte* imageData = (std::byte*)stbi_load_from_memory((const uint8_t*)ptr, length, &width, &height, nullptr, channels);

		if (imageData == nullptr)
			BSF_ERROR("Cannot load the given image");

		BSF_DEBUG("Loading image ({0} x {1}, {2} channels)", width, height, channels);

		size_t size = channels * width * height;
		std::vector<std::byte> pixels(size);
		std::memcpy(pixels.data(), imageData, size);

		stbi_image_free(imageData);

		return { std::move(pixels), width, height };

	}

	static constexpr Table<3, TextureFilter, GLenum> s_glTextureFilter = {
		std::make_tuple(TextureFilter::Linear,				GL_LINEAR),
		std::make_tuple(TextureFilter::LinearMipmapLinear,	GL_LINEAR_MIPMAP_LINEAR),
		std::make_tuple(TextureFilter::Nearest,				GL_NEAREST),
	};

	static constexpr Table<6, TextureCubeFace, GLenum> s_glTexCubeFace = {
		std::make_tuple(TextureCubeFace::Front,			GL_TEXTURE_CUBE_MAP_POSITIVE_Z),
		std::make_tuple(TextureCubeFace::Back,			GL_TEXTURE_CUBE_MAP_NEGATIVE_Z),
		std::make_tuple(TextureCubeFace::Left,			GL_TEXTURE_CUBE_MAP_NEGATIVE_X),
		std::make_tuple(TextureCubeFace::Right,			GL_TEXTURE_CUBE_MAP_POSITIVE_X),
		std::make_tuple(TextureCubeFace::Top,			GL_TEXTURE_CUBE_MAP_POSITIVE_Y),
		std::make_tuple(TextureCubeFace::Bottom,		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y),
	};

	Texture2D::Texture2D(GLenum internalFormat, GLenum format, GLenum type) :
		m_InternalFormat(internalFormat),
		m_Format(format),
		m_Type(type)
	{
		SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
	}


	Texture2D::Texture2D(uint32_t color) : Texture2D(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE)
	{
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Id));
		SetPixels(&color, 1, 1);
	}


	Texture2D::Texture2D(std::string_view fileName) : Texture2D(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE)
	{
		Load(fileName);
	}

	Texture2D::Texture2D(Texture2D&& other) noexcept : Texture2D(other.m_InternalFormat, other.m_Format, other.m_Type)
	{
		m_Id = 0;
		std::swap(m_Id, other.m_Id);
	}

	void Texture2D::SetPixels(const void* pixels, uint32_t width, uint32_t height)
	{
		Bind(0);
		BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, width, height, 0, m_Format, m_Type, pixels));
	}

	void Texture2D::SetFilter(TextureFilter minFilter, TextureFilter magFilter)
	{

		Bind(0);
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, s_glTextureFilter.Get<0, 1>(minFilter)));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, s_glTextureFilter.Get<0, 1>(magFilter)));

		if (minFilter == TextureFilter::LinearMipmapLinear)
		{
			BSF_GLCALL(glGenerateMipmap(GL_TEXTURE_2D));
		}
	}

	
	void Texture2D::SetAnisotropy(float value)
	{
		float max = 0.0f;
		BSF_GLCALL(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max));
		
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Id));
		BSF_GLCALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(max, value)));


	}

	void Texture2D::Bind(uint32_t textureUnit) const
	{
		BSF_GLCALL(glActiveTexture(GL_TEXTURE0 + textureUnit));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Id));
	}

	void Texture2D::GenerateMipmaps()
	{
		Bind(0);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	uint32_t Texture2D::GetWidth() const
	{
		GLint width;
		Bind(0);
		BSF_GLCALL(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width));
		return (uint32_t)width;
	}

	uint32_t Texture2D::GetHeight() const
	{
		GLint height;
		Bind(0);
		BSF_GLCALL(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height));
		return (uint32_t)height;
	}



	bool Texture2D::Load(std::string_view fileName)
	{
		auto [pixels, width, height] = std::move(ImageLoad(fileName, true));

		//BSF_GLCALL(glGenTextures(1, &m_Id));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Id));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data()));

		return true;
	}


	Texture::Texture() :
		m_Id(0)
	{
		BSF_GLCALL(glGenTextures(1, &m_Id));
		BSF_DEBUG("Create texture id: {0}", m_Id);
	}

	Texture::~Texture()
	{
		if (m_Id != 0)
		{
			BSF_DEBUG("Delete texture id: {0}", m_Id);
			BSF_GLCALL(glDeleteTextures(1, &m_Id));
			m_Id = 0;
		}
	}


	TextureCube::TextureCube(uint32_t size, std::string_view crossImage) :
		TextureCube(size, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE)
	{
		auto image = ImageLoad(crossImage, false);

		const auto& pixels = std::get<0>(image);
		auto width = std::get<1>(image);
		auto height = std::get<2>(image);

		assert(width / size == 4 && height / size == 3);

		auto slice = [&](uint32_t row, uint32_t col) {
			std::vector<unsigned char> result(size * size * 4);

			for (uint32_t y = 0; y < size; y++)
			{
				std::memcpy(&(result.data()[y * size * 4]), &(pixels.data()[(row * width * size + col * size + y * width) * 4]), size * 4);
			}

			return result;
		};

		std::vector<unsigned char> data;

		data = slice(1, 0);SetPixels(TextureCubeFace::Left, data.data());
		data = slice(1, 1);SetPixels(TextureCubeFace::Front, data.data());
		data = slice(1, 2);SetPixels(TextureCubeFace::Right, data.data());
		data = slice(1, 3);SetPixels(TextureCubeFace::Back, data.data());
		data = slice(0, 1);SetPixels(TextureCubeFace::Top, data.data());
		data = slice(2, 1);SetPixels(TextureCubeFace::Bottom, data.data());

	}

	TextureCube::TextureCube(uint32_t size, std::string_view front, std::string_view back, std::string_view left, 
		std::string_view right, std::string_view bottom, std::string_view top) :
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

		std::array<std::string_view, 6> files = { front, back, left, right, bottom, top };

		Initialize();
		
		for (uint32_t i = 0; i < files.size(); i++)
		{
			auto [pixels, width, height] = std::move(ImageLoad(files[i], false));

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
		BSF_GLCALL(glTexImage2D(s_glTexCubeFace.Get<0, 1>(face), 0, m_InternalFormat, m_Size, m_Size, 0, m_Format, m_Type, pixels));
	}

	void TextureCube::Bind(uint32_t textureUnit) const
	{
		BSF_GLCALL(glActiveTexture(GL_TEXTURE0 + textureUnit));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, m_Id));
	}

	void TextureCube::SetFilter(TextureFilter minFilter, TextureFilter magFilter)
	{

		Bind(0);
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, s_glTextureFilter.Get<0, 1>(minFilter)));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, s_glTextureFilter.Get<0, 1>(magFilter)));

		if (minFilter == TextureFilter::LinearMipmapLinear)
		{
			BSF_GLCALL(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
		}
	}

	void TextureCube::Initialize()
	{
		//BSF_GLCALL(glGenTextures(1, &m_Id));
		BSF_GLCALL(glBindTexture(GL_TEXTURE_CUBE_MAP, m_Id));

		SetFilter(TextureFilter::Linear, TextureFilter::Linear);

		BSF_GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		BSF_GLCALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));

	}
}