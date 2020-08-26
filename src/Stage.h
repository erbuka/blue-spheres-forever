#pragma once

#include <array>
#include <glm/glm.hpp>
#include <execution>
#include <optional>

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

		[[nodiscard]] bool Load(const std::string& fileName);
		void Save(const std::string& fileName);
		bool FromFile(const std::string& filename);
		void Initialize(uint32_t width, uint32_t height);

		Stage();
		Stage(uint32_t width, uint32_t height);

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

		int32_t GetWidth() const { return m_Width; }
		int32_t GetHeight() const { return m_Height; }

		uint32_t Count(EStageObject object) const;

		bool IsPerfect() const { return Rings == 0; }

		std::vector<EStageObject>& GetData() { return m_Data; }
		std::vector<EAvoidSearch>& GetAvoidSearch() { return m_AvoidSearch; }

		bool operator==(const Stage& other) const;

	private:

		int32_t m_Width, m_Height;
	
		void WrapX(int32_t& x) const;
		void WrapY(int32_t& y) const;

		std::vector<EStageObject> m_Data;
		std::vector<EAvoidSearch> m_AvoidSearch;

	};
}

