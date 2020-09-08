#pragma once

#include <array>
#include <glm/glm.hpp>
#include <execution>
#include <optional>
#include <string_view>

#include "Asset.h"
#include "Ref.h"
#include "Color.h"

namespace bsf
{
	class Stage;
	class StageGenerator;

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




	class StageGenerator : public Asset
	{
	public:
		StageGenerator();
		~StageGenerator();
		Ref<Stage> Generate(uint64_t code);
		uint64_t GetCodeFromStage(uint32_t stage);
		std::optional<uint32_t> GetStageFromCode(uint64_t code);
	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
	};

	class Stage
	{
	public:

		uint32_t Version;

		std::string Name = "Untitled";
		
		glm::ivec2 StartPoint = { 0, 0 }, StartDirection = { 1, 0 };

		uint32_t MaxRings = 0;
		uint32_t Rings = 0;

		std::string Texture, NormalMap;

		EFloorRenderingMode FloorRenderingMode;
		bool BumpMappingEnabled;
		glm::vec3 EmeraldColor = Colors::Green;
		std::array<glm::vec3, 2> PatternColors = { Colors::Red, Colors::White }, 
			SkyColors = { Colors::Blue, Colors::Blue }, StarColors;

		static void SaveStageFiles(const std::vector<std::string>& files);
		static std::vector<std::string> GetStageFiles();

		[[nodiscard]] bool Load(std::string_view fileName);
		void Save(std::string_view fileName);
		void Initialize(int32_t size);

		Stage();
		Stage(int32_t size);

		uint32_t GetCollectedRings() const { return MaxRings - Rings; }
		void CollectRing(const glm::ivec2& position);

		EStageObject GetValueAt(int32_t x, int32_t y) const;
		EStageObject GetValueAt(const glm::ivec2& pos) const { return GetValueAt(pos.x, pos.y); }

		void SetValueAt(int32_t x, int32_t y, EStageObject obj);
		void SetValueAt(const glm::ivec2& pos, EStageObject obj) { SetValueAt(pos.x, pos.y, obj); }

		EAvoidSearch GetAvoidSearchAt(int32_t x, int32_t y) const;
		EAvoidSearch GetAvoidSearchAt(const glm::ivec2& pos) const { return GetAvoidSearchAt(pos.x, pos.y); }

		void SetAvoidSearchAt(int32_t x, int32_t y, EAvoidSearch val);
		void SetAvoidSearchAt(const glm::ivec2& pos, EAvoidSearch val) { SetAvoidSearchAt(pos.x, pos.y, val); }

		uint32_t Count(EStageObject object) const;

		bool IsPerfect() const { return Rings == 0; }

		std::vector<EStageObject>& GetData() { return m_Data; }
		std::vector<EAvoidSearch>& GetAvoidSearch() { return m_AvoidSearch; }
		
		glm::ivec2 WrapCoordinates(glm::ivec2 pos) { Wrap(pos.x); Wrap(pos.y); return pos; }

		int32_t GetSize() const { return m_Size; }

		bool Resize(int32_t size);

		bool operator==(const Stage& other) const;

	private:

		int32_t m_Size;
	
		void Wrap(int32_t& coord) const;

		std::vector<EStageObject> m_Data;
		std::vector<EAvoidSearch> m_AvoidSearch;

	};
}

