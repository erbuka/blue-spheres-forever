#include "BsfPch.h"

#include <bitset>

#include "Stage.h"
#include "Common.h"
#include "Log.h";

#include <json/json.hpp>




namespace bsf
{
	bool Stage::FromFile(const std::string& filename)
	{
		std::ifstream stdIs;

		stdIs.open(filename, std::ios_base::binary);

		if (!stdIs.good())
		{
			BSF_ERROR("Bad file: {0}", filename);
			return false;
		}

		InputStream<ByteOrder::LittleEndian> is(stdIs);

		is.Read(Version);

		is.Read(m_Width); // Width
		is.Read(m_Height); // Height

		is.Read(FloorRenderingMode); // uint8;
		is.Read(BumpMappingEnabled); // uint8;

		uint32_t texStrSize = is.Read<uint32_t>();
		uint32_t bumpStrSize = is.Read<uint32_t>();
	
		if (texStrSize > 0)
		{
			Texture.resize(texStrSize);
			is.ReadSome<char>(texStrSize, Texture.data());
		}

		if (bumpStrSize > 0)
		{
			NormalMap.resize(bumpStrSize);
			is.ReadSome(bumpStrSize, NormalMap.data());
		}
		

		is.Read(StartPoint); // ivec2
		is.Read(StartDirection); // ivec2
		is.Read(EmeraldColor);// vec3

		is.ReadSome(2, CheckerColors.data()); // 2 * vec3
		is.ReadSome(2, SkyColors.data()); // 2 * vec3
		is.ReadSome(2, StarColors.data()); // 2 * vec3

		is.Read(Rings); // uint32

		m_Data.resize(m_Width * m_Height);
		m_AvoidSearch.resize(m_Width * m_Height);

		is.ReadSome(m_Data.size(), m_Data.data()); // 1024 bytes
		is.ReadSome(m_AvoidSearch.size(), m_AvoidSearch.data()); // 1024 bytes;

		stdIs.close();

	}

	void Stage::Initialize(uint32_t width, uint32_t height)
	{
		m_Width = (int32_t)width;
		m_Height = (int32_t)height;
		m_Data.resize(m_Width * m_Height);
		m_AvoidSearch.resize(m_Width * m_Height);
		std::fill(m_Data.begin(), m_Data.end(), EStageObject::None);
		std::fill(m_AvoidSearch.begin(), m_AvoidSearch.end(), EAvoidSearch::No);
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
		return m_Data[y * m_Width + x];
	}
	void Stage::SetValueAt(int32_t x, int32_t y, EStageObject obj)
	{
		WrapX(x); WrapY(y);
		m_Data[y * m_Width + x] = obj;
	}
	
	EAvoidSearch Stage::GetAvoidSearchAt(int32_t x, int32_t y) const
	{
		WrapX(x); WrapY(y);
		return m_AvoidSearch[y * m_Width + x];
	}
	void Stage::SetAvoidSearchAt(int32_t x, int32_t y, EAvoidSearch val)
	{
		WrapX(x); WrapY(y);
		m_AvoidSearch[y * m_Width + x] = val;
	}

	uint32_t Stage::Count(EStageObject object) const
	{
		return std::count(std::execution::par_unseq, m_Data.begin(), m_Data.end(), object);
	}
	void Stage::Dump()
	{
		for (int32_t y = 0; y < m_Height; y++)
		{
			for (int32_t x = 0; x < m_Width; x++)
			{
				std::cout << (int)m_Data[y * m_Width + x];
			}
			std::cout << std::endl;
		}
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
		inline EStageObject GetValueAt(int32_t x, int32_t y) { return Data[y * s_SectionSize + x]; }
		inline EStageObject GetValueAt(const glm::ivec2& pos) { return GetValueAt(pos.x, pos.y); }

		inline EAvoidSearch GetAvoidSearchAt(int32_t x, int32_t y) { return AvoidSearch[y * s_SectionSize + x]; }
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

					result.Data[y * s_SectionSize + x] = Data[swapIndex];
					result.AvoidSearch[y * s_SectionSize + x] = AvoidSearch[swapIndex];
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
		auto levelOpt = GetLevelFromCode(code);

		if (!levelOpt.has_value())
			return nullptr;

		uint32_t level = levelOpt.value();

		// Calculating sections

		uint32_t tr = (level - 1) % 128;					// Top right
		uint32_t br = (1 + ((level - 1) % 127) * 3) % 127;	// Bottom righe
		uint32_t tl = (2 + ((level - 1) % 126) * 5) % 126;	// Top left
		uint32_t bl = (3 + ((level - 1) % 125) * 7) % 125;	// Bottom left


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
				auto& s = sections[sy * 2 + sx];

				result->Rings += s.Rings;
				
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


		result->FloorRenderingMode = EFloorRenderingMode::CheckerBoard;
		result->BumpMappingEnabled = false;
		result->CheckerColors = {
			s_CheckerBoardPatterns[(tl % 16) * 2],
			s_CheckerBoardPatterns[(tl % 16) * 2 + 1]
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
		result->StartPoint = { 28, 16 }; 

		return result;

	}

	uint64_t StageGenerator::GetCodeFromLevel(uint32_t level)
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
		cb = (level + 19088742);

		// Calculate code in binary form (note that some values (ca) are missing here)

		for (uint32_t i = 0; i <= 5; i++)
			ca[i + 26] = cb[i];
		for (uint32_t i = 6; i <= 26; i++)
			ca[i - 6] = cb[i];


		// Stage is decreased by 1 and then the binary form is recalculated
		cb = (level - 1);

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

	std::optional<uint32_t> StageGenerator::GetLevelFromCode(uint64_t code)
	{
		uint32_t level = 0;
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

		// Getting the level number (binary) from the code
		for (uint32_t i = 0; i < 6; i++)
			cb[i] = ca[i + 26];

		for (uint32_t i = 6; i < 27; i++)
			cb[i] = ca[i - 6];

		/* Calculate the level number in decimal form */
		level = (uint32_t)cb.to_ulong();

		/* The true level number is found by subtracting 19088742 from the result */
		level -= 19088742;

		/* If the stage is <= 0, we add the max_stage (cyclic stages) */
		level %= s_MaxStage;

		/* Checking that the given code is fully correct. If not, return -1 (invalid code) */
		if (GetCodeFromLevel(level) != code) {
			BSF_ERROR("Invalid level code: {0}", code);
			return std::nullopt;
		}
		return level;
	}


	#pragma endregion
}