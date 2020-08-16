#pragma once

#include "Common.h"
#include "Asset.h"

#include <string>
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

	class Texture : public Asset
	{
	public:
		Texture();
		Texture(Texture&) = delete;

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
		TextureCube(uint32_t size, const std::string& crossImage);
		TextureCube(uint32_t size, const std::string& front, const std::string& back, const std::string& left, const std::string& right, const std::string& bottom, const std::string& top);
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
		Texture2D();
		Texture2D(uint32_t color);
		Texture2D(const std::string& fileName);

		Texture2D(Texture2D&) = delete;
		Texture2D(Texture2D&&) noexcept;

		void SetPixels(void * pixels, uint32_t width, uint32_t height, GLenum internalFormat, GLenum format, GLenum type);

		void SetFilter(TextureFilter minFilter, TextureFilter magFilter) override;

		void SetAnisotropy(float value);
		void Bind(uint32_t textureUnit) const override;

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;
	
	private:

		bool Load(const std::string& fileName);

	};

}