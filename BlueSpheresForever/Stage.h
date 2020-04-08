#pragma once

#include <array>
#include <glm/glm.hpp>


namespace bsf
{
	static constexpr int32_t s_StageSize = 32;


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
		StarSphere = 3,
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

		static Stage FromFile(const std::string& filename);

		Stage();
		
		EStageObject GetValueAt(const glm::ivec2& position) const;
		EStageObject GetValueAt(int32_t x, int32_t y) const;
		void SetValueAt(int32_t x, int32_t y, EStageObject obj);
		void SetValueAt(const glm::ivec2& position, EStageObject obj);

		int32_t GetWidth() const { return s_StageSize; }
		int32_t GetHeight() const { return s_StageSize; }

	private:
	
		void WrapCoordinate(int32_t& coord) const;
		std::array<std::array<EStageObject, s_StageSize>, s_StageSize> m_Data;
		std::array<std::array<EAvoidSearch, s_StageSize>, s_StageSize> m_AvoidSearch;
	
	};
}

