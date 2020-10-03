#pragma once

#include "Ref.h"
#include "Asset.h"

#include <string_view>
#include <vector>


namespace bsf
{

	enum class TextureFilter
	{
		Nearest,
		Linear,
		LinearMipmapLinear
	};

	enum class TextureCubeFace : int32_t
	{
		Right = 0,
		Left = 1,
		Top = 2,
		Bottom = 3,
		Front = 4,
		Back = 5
	};

	constexpr std::array<TextureCubeFace, 6> TextureCubeFaces = {
		TextureCubeFace::Right,
		TextureCubeFace::Left,
		TextureCubeFace::Top,
		TextureCubeFace::Bottom,
		TextureCubeFace::Front,
		TextureCubeFace::Back
	};

	std::tuple<std::vector<std::byte>, uint32_t, uint32_t> ImageLoad(const void* ptr, size_t length, bool flipY);
	std::tuple<std::vector<std::byte>, uint32_t, uint32_t> ImageLoad(std::string_view fileName, bool flipY);

	class Texture : public Asset
	{
	public:
		Texture();
		Texture(const Texture&) = delete;

		virtual ~Texture();

		virtual void Bind(uint32_t textureUnit) const = 0;

		virtual void SetFilter(TextureFilter minFilter, TextureFilter magFilter) = 0;

		uint32_t GetId() const { return m_Id; }

	protected:

		uint32_t m_Id;
	};

	
	class TextureCube : public Texture
	{
	public:
		TextureCube(uint32_t size, std::string_view crossImage);
		TextureCube(uint32_t size, std::string_view front, std::string_view back, std::string_view left, 
			std::string_view right, std::string_view bottom, std::string_view top);
		TextureCube(uint32_t size, GLenum internalFormat, GLenum format, GLenum type);
		TextureCube(TextureCube&) = delete;
		TextureCube(TextureCube&&) = delete;

		void SetPixels(TextureCubeFace face, const void* pixels);

		void Bind(uint32_t textureUnit) const override;

		void SetFilter(TextureFilter minFilter, TextureFilter magFilter) override;

		uint32_t GetSize() const { return m_Size; }



	private:
		void Initialize();
		uint32_t m_Size;
		GLenum m_InternalFormat, m_Format, m_Type;

	};

	class Texture2D : public Texture
	{
	public:
		Texture2D(GLenum internalFormat, GLenum format, GLenum type);
		Texture2D(uint32_t color);
		Texture2D(std::string_view fileName);

		Texture2D(const Texture2D&) = delete;
		Texture2D(Texture2D&&) noexcept;

		void SetPixels(const void * pixels, uint32_t width, uint32_t height);

		void SetFilter(TextureFilter minFilter, TextureFilter magFilter) override;

		void SetAnisotropy(float value);
		void Bind(uint32_t textureUnit) const override;

		void GenerateMipmaps();

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		GLenum GetInternalFormat() const { return m_InternalFormat; }
		GLenum GetFormat() const { return m_Format; }
		GLenum GetType() const { return m_Type; }
	
	private:

		GLenum m_InternalFormat, m_Format, m_Type;

		bool Load(std::string_view fileName);

	};

}