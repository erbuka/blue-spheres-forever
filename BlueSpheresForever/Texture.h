#pragma once

#include "Common.h"

#include <string>
#include <vector>


namespace bsf
{

	class Texture
	{
	public:

		Texture(uint32_t width, uint32_t height, uint32_t pixelsPerUnit, void* pixels);
		Texture(const std::string& fileName, uint32_t pixelsPerUnit);
		Texture(Texture&) = delete;
		Texture(Texture&&) noexcept;

		~Texture();

		uint32_t GetID() const { return m_ID; }

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		uint32_t GetPixelsPerUnit() const { return m_PixelsPerUnit; }

	private:

		bool Load(const std::string& fileName);

		uint32_t m_PixelsPerUnit;
		uint32_t m_Width = 0, m_Height = 0;
		uint32_t m_ID = 0;
	};

}