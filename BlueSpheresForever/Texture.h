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

	enum class TextureCubeFace
	{
		Front, Back, Left, Right, Bottom, Top
	};

	class Texture
	{
	public:
		Texture();
		virtual ~Texture();

		virtual void Bind(uint32_t textureUnit) = 0;

		uint32_t GetId() const { return m_Id; }

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

	protected:
		uint32_t m_Id;
		uint32_t m_Width, m_Height;
	};

	
	class TextureCube : public Texture
	{
	public:
		TextureCube(const std::string& front, const std::string& back, const std::string& left, const std::string& right, const std::string& bottom, const std::string& top);
		TextureCube(uint32_t width, uint32_t height);
		TextureCube(TextureCube&) = delete;
		TextureCube(TextureCube&&) = delete;

		void SetPixels(TextureCubeFace face, const void* pixels);

		void Bind(uint32_t textureUnit) override;

		void Filter(TextureFilter filter, TextureFilterMode mode);


	};

	class Texture2D : public Texture
	{
	public:

		Texture2D(uint32_t width, uint32_t height, const void* pixels);
		Texture2D(const std::string& fileName);
		Texture2D(Texture2D&) = delete;
		Texture2D(Texture2D&&) noexcept;

		void Filter(TextureFilter filter, TextureFilterMode mode);

		void Bind(uint32_t textureUnit) override;

	private:

		bool Load(const std::string& fileName);

	};

}