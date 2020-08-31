#pragma once

#include <glm/glm.hpp>

namespace bsf
{
	constexpr uint32_t ToHexColor(const glm::vec4& rgba)
	{
		uint8_t r = uint8_t(rgba.r * 255.0f);
		uint8_t g = uint8_t(rgba.g * 255.0f);
		uint8_t b = uint8_t(rgba.b * 255.0f);
		uint8_t a = uint8_t(rgba.a * 255.0f);

		return  (a << 24) | (b << 16) | (g << 8) | (r << 0);
	}

	constexpr uint32_t ToHexColor(const glm::vec3& rgb)
	{
		return ToHexColor({ rgb, 1.0f });
	}

	constexpr glm::vec4 Lighten(const glm::vec4 color, float factor)
	{
		return {
			color.r + (1.0f - color.r) * factor,
			color.g + (1.0f - color.g) * factor,
			color.b + (1.0f - color.b) * factor,
			color.a
		};
	}


	constexpr glm::vec4 Darken(const glm::vec4 color, float factor)
	{
		return {
			color.r * factor,
			color.g * factor,
			color.b * factor,
			color.a
		};
	}

	constexpr glm::vec4 ToColor(uint32_t rgba)
	{
		return {
			((rgba & 0x000000ff) >> 0) / 255.0f,
			((rgba & 0x0000ff00) >> 8) / 255.0f,
			((rgba & 0x00ff0000) >> 16) / 255.0f,
			((rgba & 0xff000000) >> 24) / 255.0f,
		};
	}

	namespace Colors
	{
		constexpr glm::vec4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
		constexpr glm::vec4 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
		constexpr glm::vec4 Yellow = ToColor(0xff0ae4e8);
		constexpr glm::vec4 Red = ToColor(0xff0000f0);
		constexpr glm::vec4 Blue = ToColor(0xfff04820);
		constexpr glm::vec4 Green = ToColor(0xff48f020);
		constexpr glm::vec4 Transparent = { 0.0f, 0.0f, 0.0f, 0.0f };

		constexpr glm::vec4 BlueSphere = Blue;
		constexpr glm::vec4 RedSphere = Red;
		constexpr glm::vec4 YellowSphere = Yellow;
		constexpr glm::vec4 Ring = { glm::vec3(0.83f, 0.69f, 0.22f), 1.0f };
	}
}