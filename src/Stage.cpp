#include "BsfPch.h"

#include <bitset>
#include <json/json.hpp>

#include "Stage.h"
#include "Log.h";
#include "Common.h"

namespace glm
{
	using namespace nlohmann;

	template<template<int, typename, glm::qualifier> typename V, int N, typename T, glm::qualifier Q>
	void to_json(json& json, const V<N, T, Q>& v)
	{
		const T* ptr = glm::value_ptr<T>(v);
		json = json::array();
		for (size_t i = 0; i < N; i++)
			json[i] = *(ptr + i);
	}

	template<template<int, typename, glm::qualifier> typename V, int N, typename T, glm::qualifier Q>
	void from_json(const json& j, V<N, T, Q>& v) {
		T* ptr = glm::value_ptr<T>(v);
		for (size_t i = 0; i < N; i++)
			*(ptr + i) = j[i].get<T>();
	}
}

namespace bsf
{
	using namespace nlohmann;

	static constexpr char* s_SortedStagesFile = "assets/data/stages.json";


	void Stage::SaveStageFiles(const std::vector<std::string>& files)
	{
		std::ofstream os;

		os.open(s_SortedStagesFile);

		if (!os.is_open())
		{
			BSF_ERROR("Cannot save stages ordering");
			return;
		}

		os << json(files).dump();

		os.close();
	}

	std::vector<std::string> Stage::GetStageFiles()
	{
		
		namespace fs = std::filesystem;


		std::vector<std::string> result;

		if (fs::is_regular_file(s_SortedStagesFile))
		{
			auto files = json::parse(ReadTextFile(s_SortedStagesFile));

			for (auto& item : files)
			{
				std::string name = item.get<std::string>();

				if (fs::is_regular_file(name))
					result.push_back(name);

			}

		}


		for (auto& entry : fs::directory_iterator("assets/data"))
		{
			
			if (entry.is_regular_file() && std::find(result.begin(), result.end(), entry.path().string()) == result.end() &&
				entry.path().extension().string() == ".bssj")
			{
				result.push_back(entry.path().string());
			}
		}

		return result;

	}

	bool Stage::Load(std::string_view fileName)
	{
		try
		{
			auto root = json::parse(ReadTextFile(fileName));

			Version = root.at("version").get<uint32_t>();
			Name = root.at("name").get<std::string>();
			m_Width = root.at("width").get<int32_t>();
			m_Height = root.at("height").get<int32_t>();
			StartPoint = root.at("startPoint").get<glm::vec2>();
			StartDirection = root.at("startDirection").get<glm::vec2>();
			Rings = MaxRings = root.at("maxRings").get<uint32_t>();
			EmeraldColor = root.at("emeraldColor").get<glm::vec3>();
			PatternColors = root.at("patternColors").get<std::array<glm::vec3, 2>>();
			SkyColors = root.at("skyColors").get<std::array<glm::vec3, 2>>();
			m_Data = root.at("data").get<std::vector<EStageObject>>();
			m_AvoidSearch = root.at("avoidSearch").get<std::vector<EAvoidSearch>>();
		}
		catch (std::exception& err)
		{
			BSF_ERROR("Invalid stage file: {0}", fileName);
			return false;
		}

		return true;
	}

	void Stage::Save(std::string_view fileName)
	{

		auto root = json::object();

		root = {
			{ "version", Version },
			{ "name", Name },
			{ "width", m_Width },
			{ "height", m_Height },
			{ "startPoint", StartPoint },
			{ "startDirection", StartDirection },
			{ "maxRings" , MaxRings },
			{ "emeraldColor", EmeraldColor },
			{ "patternColors", PatternColors },
			{ "skyColors", SkyColors },
			{ "data", m_Data },
			{ "avoidSearch", m_AvoidSearch }
		};

		std::ofstream os;
		os.open(fileName.data());

		if (!os.is_open())
		{
			BSF_ERROR("Can't open the file: {0}", fileName.data());
			return;
		}

		os << root.dump();

		os.close();

	}

	void Stage::Initialize(uint32_t width, uint32_t height)
	{

		m_Width = (int32_t)width;
		m_Height = (int32_t)height;
		m_Data.resize((size_t)m_Width * m_Height);
		m_AvoidSearch.resize((size_t)m_Width * m_Height);
		std::fill(m_Data.begin(), m_Data.end(), EStageObject::None);
		std::fill(m_AvoidSearch.begin(), m_AvoidSearch.end(), EAvoidSearch::No);
	}

	Stage::Stage() : Stage(32, 32)
	{
	}

	Stage::Stage(uint32_t width, uint32_t height)
	{
		Initialize(width, height);
	}

	void Stage::CollectRing(const glm::ivec2& position)
	{
		if (GetValueAt(position) != EStageObject::Ring) {
			BSF_ERROR("Not a ring!");
			return;
		}
		SetValueAt(position, EStageObject::None);
		Rings--;
	}

	EStageObject Stage::GetValueAt(int32_t x, int32_t y) const
	{
		WrapX(x); WrapY(y);
		return m_Data[(size_t)y * m_Width + x];
	}
	void Stage::SetValueAt(int32_t x, int32_t y, EStageObject obj)
	{
		WrapX(x); WrapY(y);
		m_Data[(size_t)y * m_Width + x] = obj;
	}
	
	EAvoidSearch Stage::GetAvoidSearchAt(int32_t x, int32_t y) const
	{
		WrapX(x); WrapY(y);
		return m_AvoidSearch[(size_t)y * m_Width + x];
	}
	void Stage::SetAvoidSearchAt(int32_t x, int32_t y, EAvoidSearch val)
	{
		WrapX(x); WrapY(y);
		m_AvoidSearch[(size_t)y * m_Width + x] = val;
	}

	uint32_t Stage::Count(EStageObject object) const
	{
		return std::count(std::execution::par_unseq, m_Data.begin(), m_Data.end(), object);
	}


	bool Stage::operator==(const Stage& other) const
	{
		return m_Data == other.m_Data &&
			m_AvoidSearch == other.m_AvoidSearch &&
			StartPoint == other.StartPoint &&
			StartDirection == other.StartDirection &&
			MaxRings == other.MaxRings &&
			EmeraldColor == other.EmeraldColor &&
			SkyColors == other.SkyColors &&
			PatternColors == other.PatternColors;

	}

	void Stage::WrapX(int32_t& x) const
	{
		while (x < 0)
			x += m_Width;

		x %= m_Width;
	}
	void Stage::WrapY(int32_t& y) const
	{
		while (y < 0)
			y += m_Height;

		y %= m_Width;
	}
	
	#pragma region Stage Generator

	static constexpr uint32_t s_SectionSize = 16;
	static constexpr uint32_t s_MinStage = 1;
	static constexpr uint32_t s_MaxStage = 134217728;

	static constexpr std::array<glm::vec3, 8> s_EmeraldColors = { 
		glm::vec3(0.0, 1.0, 0.0),
		glm::vec3(1.0, 0.0, 0.0),
		glm::vec3(1.0, 0.0, 1.0),
		glm::vec3(0.0, 0.0, 1.0),
		glm::vec3(0.7, 0.7, 0.7),
		glm::vec3(1.0, 0.0, 0.0),
		glm::vec3(0.5, 0.5, 1.0),
		glm::vec3(1.0, 1.0, 0.0)
	};

	static constexpr std::array<glm::vec3, 32> s_CheckerBoardPatterns = {
		(1.0f / 255.0f) * glm::vec3(224, 128, 0),		(1.0f / 255.0f) * glm::vec3(96, 32, 0),
		(1.0f / 255.0f) * glm::vec3(32, 96, 0),			(1.0f / 255.0f) * glm::vec3(0, 224, 160),
		(1.0f / 255.0f) * glm::vec3(224, 96, 0),		(1.0f / 255.0f) * glm::vec3(224, 224, 160),
		(1.0f / 255.0f) * glm::vec3(224, 224, 224),		(1.0f / 255.0f) * glm::vec3(192, 0, 0),
		(1.0f / 255.0f) * glm::vec3(64, 32, 64),		(1.0f / 255.0f) * glm::vec3(224, 128, 0),
		(1.0f / 255.0f) * glm::vec3(0, 64, 192),		(1.0f / 255.0f) * glm::vec3(160, 224, 224),
		(1.0f / 255.0f) * glm::vec3(96, 32, 160),		(1.0f / 255.0f) * glm::vec3(224, 192, 0),
		(1.0f / 255.0f) * glm::vec3(0, 0, 160),			(1.0f / 255.0f) * glm::vec3(64, 160, 0),
		(1.0f / 255.0f) * glm::vec3(32, 224, 0),		(1.0f / 255.0f) * glm::vec3(192, 224, 128),
		(1.0f / 255.0f) * glm::vec3(224, 96, 0),		(1.0f / 255.0f) * glm::vec3(128, 224, 192),
		(1.0f / 255.0f) * glm::vec3(192, 0, 160),		(1.0f / 255.0f) * glm::vec3(160, 192, 0),
		(1.0f / 255.0f) * glm::vec3(0, 0, 192),			(1.0f / 255.0f) * glm::vec3(192, 160, 0),
		(1.0f / 255.0f) * glm::vec3(128, 0, 224),		(1.0f / 255.0f) * glm::vec3(32, 224, 160),
		(1.0f / 255.0f) * glm::vec3(160, 224, 0),		(1.0f / 255.0f) * glm::vec3(96, 0, 224),
		(1.0f / 255.0f) * glm::vec3(0, 192, 224),		(1.0f / 255.0f) * glm::vec3(192, 32, 160),
		(1.0f / 255.0f) * glm::vec3(0, 0, 192),			(1.0f / 255.0f) * glm::vec3(192, 192, 192)
	};

	struct StageSection
	{
		uint32_t Rings;
		std::array<EStageObject, s_SectionSize * s_SectionSize> Data;
		std::array<EAvoidSearch, s_SectionSize * s_SectionSize> AvoidSearch;
		inline EStageObject GetValueAt(int32_t x, int32_t y) { return Data[(size_t)y * s_SectionSize + x]; }
		inline EStageObject GetValueAt(const glm::ivec2& pos) { return GetValueAt(pos.x, pos.y); }

		inline EAvoidSearch GetAvoidSearchAt(int32_t x, int32_t y) { return AvoidSearch[(size_t)y * s_SectionSize + x]; }
		inline EAvoidSearch GetAvoidSearchAt(const glm::ivec2& pos) { return GetAvoidSearchAt(pos.x, pos.y); }

		StageSection Flip(bool horizontal) 
		{
			StageSection result;

			result.Rings = Rings;

			for (uint32_t y = 0; y < s_SectionSize; y++)
			{
				for (uint32_t x = 0; x < s_SectionSize; x++)
				{
					uint32_t swapIndex = horizontal ?
						y * s_SectionSize + s_SectionSize - (x + 1) :
						(s_SectionSize - (y + 1)) * s_SectionSize + x;

					result.Data[(size_t)y * s_SectionSize + x] = Data[swapIndex];
					result.AvoidSearch[(size_t)y * s_SectionSize + x] = AvoidSearch[swapIndex];
				}
			}

			return result;
		}
	};

	struct StageGenerator::Impl { std::vector<StageSection> m_Sections; };

	StageGenerator::StageGenerator()
	{
		using json = nlohmann::json;

		m_Impl = std::make_unique<Impl>();

		std::ifstream is;

		is.open("assets/data/sections.json");

		if (!is.is_open() || !is.good())
		{
			BSF_ERROR("Cannot open sections.json");
			return;
		}

		auto data = json::parse(is);
		is.close();

		m_Impl->m_Sections.reserve(data.size());

		for (auto s : data)
		{
			StageSection section;

			section.Rings = s["maxRings"];
			std::copy(s["data"].begin(), s["data"].end(), section.Data.data());
			std::copy(s["avoidSearch"].begin(), s["avoidSearch"].end(), section.AvoidSearch.data());

			m_Impl->m_Sections.push_back(section);
		}

	}

	StageGenerator::~StageGenerator()
	{
		m_Impl = nullptr;
	}

	Ref<Stage> StageGenerator::Generate(uint64_t code)
	{
		auto stageOpt = GetStageFromCode(code);

		if (!stageOpt.has_value())
			return nullptr;

		uint32_t stage = stageOpt.value();

		// Calculating sections

		uint32_t tr = (stage - 1) % 128;					// Top right
		uint32_t br = (1 + ((stage - 1) % 127) * 3) % 127;	// Bottom righe
		uint32_t tl = (2 + ((stage - 1) % 126) * 5) % 126;	// Top left
		uint32_t bl = (3 + ((stage - 1) % 125) * 7) % 125;	// Bottom left


		std::array<StageSection, 4> sections = { 
			m_Impl->m_Sections[bl],
			m_Impl->m_Sections[br].Flip(true),
			m_Impl->m_Sections[tl].Flip(false),
			m_Impl->m_Sections[tr].Flip(false).Flip(true),
		};

		auto result = MakeRef<Stage>();

		result->Initialize(s_SectionSize * 2, s_SectionSize * 2);

		// Auto version number
		result->Version = 300;

		// Copy data
		for (uint32_t sy = 0; sy < 2; sy++)
		{
			for (uint32_t sx = 0; sx < 2; sx++)
			{
				auto& s = sections[(size_t)sy * 2 + sx];

				result->MaxRings += s.Rings;
				
				for (uint32_t y = 0; y < s_SectionSize; y++)
				{
					for (uint32_t x = 0; x < s_SectionSize; x++)
					{
						// Absolute x
						int32_t ax = sx * s_SectionSize + x;
						
						// Absolute y
						int32_t ay = sy * s_SectionSize + y;

						result->SetValueAt(ax, ay, s.GetValueAt({ x, y }));
						result->SetAvoidSearchAt(ax, ay, s.GetAvoidSearchAt({ x, y }));
					}
				}


			}
		}

		result->Rings = result->MaxRings;


		result->FloorRenderingMode = EFloorRenderingMode::CheckerBoard;
		result->BumpMappingEnabled = false;
		result->PatternColors = {
			s_CheckerBoardPatterns[((size_t)tl % 16) * 2],
			s_CheckerBoardPatterns[((size_t)tl % 16) * 2 + 1]
		};
		
		result->SkyColors = {
			glm::vec3(0, 23, 178) / 255.0f,
			glm::vec3(84, 176, 255) / 255.0f
		};

		result->StarColors = {
			glm::vec3(255, 255, 255) / 255.0f,
			glm::vec3(255, 255, 128) / 255.0f
		};

		result->EmeraldColor = s_EmeraldColors[tr % s_EmeraldColors.size()];

		result->StartDirection = { 0, 1 };
		result->StartPoint = { 28, 15 }; 

		return result;

	}

	uint64_t StageGenerator::GetCodeFromStage(uint32_t stage)
	{

		// cb -> stage number (binary form)
		// ca -> code (binary form)

		std::bitset<39> ca;
		std::bitset<28> cb;

		// Zeroing code
		ca.reset();
		cb.reset();

		// First bit is always 1
		ca[38] = true;

		// Before calculating the binary form, stage number is increased by 19088742
		cb = ((uint64_t)stage + 19088742);

		// Calculate code in binary form (note that some values (ca) are missing here)

		for (uint32_t i = 0; i <= 5; i++)
			ca[i + 26] = cb[i];
		for (uint32_t i = 6; i <= 26; i++)
			ca[i - 6] = cb[i];


		// Stage is decreased by 1 and then the binary form is recalculated
		cb = (stage - 1);

		// The missing code values are filled with pseudo-random data generated from the stage number decreased by 1

		ca[37] = (1 + cb[6] + cb[23]) % 2;
		ca[36] = (1 + cb[5] + cb[22] + cb[17] + cb[0]) % 2;
		ca[35] = (1 + cb[4] + cb[21] + cb[16]) % 2;
		ca[34] = (1 + cb[3] + cb[20]) % 2;
		ca[33] = (1 + cb[2] + cb[19]) % 2;
		ca[32] = (1 + cb[1] + cb[18]) % 2;

		ca[25] = (0 + cb[11] + cb[16]) % 2;
		ca[24] = (0 + cb[10]) % 2;
		ca[23] = (0 + cb[9] + cb[26]) % 2;
		ca[22] = (0 + cb[8] + cb[25]) % 2;
		ca[21] = (0 + cb[7] + cb[24]) % 2;

		for (uint32_t i = 1; i < 38; i = i + 2)
			ca[i] = !ca[i];

		return (uint64_t)ca.to_ullong();

	}

	std::optional<uint32_t> StageGenerator::GetStageFromCode(uint64_t code)
	{
		uint32_t stage = 0;
		std::bitset<39> ca;
		std::bitset<28> cb;

		ca.reset();
		cb.reset();


		// Get the other bits from the code
		ca = code;
		
		// First bit of the code is always 1
		ca[38] = true;

		// Getting the original ca bin inverting odd bits
		for (uint32_t i = 1; i < 38; i += 2)
			ca[i] = !ca[i];

		// This bit is always 0 (it seems)
		cb[27] = false;

		// Getting the stage number (binary) from the code
		for (uint32_t i = 0; i < 6; i++)
			cb[i] = ca[i + 26];

		for (uint32_t i = 6; i < 27; i++)
			cb[i] = ca[i - 6];

		/* Calculate the stage number in decimal form */
		stage = (uint32_t)cb.to_ulong();

		/* The true stage number is found by subtracting 19088742 from the result */
		stage -= 19088742;

		/* If the stage is <= 0, we add the max_stage (cyclic stages) */
		stage %= s_MaxStage;

		/* Checking that the given code is fully correct. If not, return -1 (invalid code) */
		if (GetCodeFromStage(stage) != code) {
			BSF_ERROR("Invalid stage code: {0}", code);
			return std::nullopt;
		}
		return stage;
	}



	#pragma endregion

}