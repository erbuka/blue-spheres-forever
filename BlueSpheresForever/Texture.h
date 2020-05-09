#pragma once

#include "Common.h"

#include <string>
#include <vector>


namespace bsf
{

	enum class TextureFilter
	{
		MinFilter,
		MagFilter
	};

	enum class TextureFilterMode
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

	class Texture
	{
	public:
		Texture();
		Texture(Texture&) = delete;

		virtual ~Texture();

		virtual void Bind(uint32_t textureUnit) = 0;

		uint32_t GetId() const { return m_Id; }

	protected:
		uint32_t m_Id;
	};

	
	class TextureCube : public Texture
	{
	public:
		TextureCube(uint32_t size, const std::string& front, const std::string& back, const std::string& left, const std::string& right, const std::string& bottom, const std::string& top);
		TextureCube(uint32_t size, GLenum internalFormat, GLenum format, GLenum type);
		TextureCube(TextureCube&) = delete;
		TextureCube(TextureCube&&) = delete;

		void SetPixels(TextureCubeFace face, const void* pixels);

		void Bind(uint32_t textureUnit) override;

		void Filter(TextureFilter filter, TextureFilterMode mode);
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

		void Filter(TextureFilter filter, TextureFilterMode mode);
		void SetAnisotropy(float value);
		void Bind(uint32_t textureUnit) override;

	private:

		bool Load(const std::string& fileName);

	};

}