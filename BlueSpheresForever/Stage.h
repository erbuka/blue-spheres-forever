#pragma once

#include <array>
#include <glm/glm.hpp>


namespace bsf
{

	enum class EAvoidSearch : uint8_t
	{
		No = 0,
		Yes = 1
	};

	enum class EStageObject : uint32_t
	{
		None = 0,
		RedSphere = 1,
		BlueSphere = 2,
		Bumper = 3,
		Ring = 4,
		YellowSphere = 5
	};

	enum class EFloorRenderingMode : uint8_t
	{
		CheckerBoard = 0,
		Texture = 1
	};


	class Stage
	{
	public:

		uint32_t Version;
		
		glm::ivec2 StartPoint, StartDirection;

		uint32_t MaxRings;

		std::string Texture, NormalMap;

		EFloorRenderingMode FloorRenderingMode;
		bool BumpMappingEnabled;
		glm::vec3 EmeraldColor;
		std::array<glm::vec3, 2> CheckerColors, SkyColors, StarColors;

		bool FromFile(const std::string& filename);

		Stage() = default;
		
		EStageObject GetValueAt(const glm::ivec2& position) const;
		EStageObject GetValueAt(int32_t x, int32_t y) const;
		void SetValueAt(int32_t x, int32_t y, EStageObject obj);
		void SetValueAt(const glm::ivec2& position, EStageObject obj);

		int32_t GetWidth() const { return m_Width; }
		int32_t GetHeight() const { return m_Height; }

		void Dump();

	private:

		int32_t m_Width, m_Height;
	
		void WrapX(int32_t& x) const;
		void WrapY(int32_t& y) const;

		std::vector<EStageObject> m_Data;
		std::vector<EAvoidSearch> m_AvoidSearch;

	};
}

