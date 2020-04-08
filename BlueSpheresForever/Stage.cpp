#include "Stage.h"
#include "Common.h"
#include "Log.h";

#include <fstream>

namespace bsf
{
	Stage Stage::FromFile(const std::string& filename)
	{
		Stage result;

		std::ifstream is;

		is.open(filename, std::ios_base::binary);

		if (!is.good())
		{
			BSF_ERROR("Bad file: {0}", filename);
			return std::move(result);
		}

		Read(is, result.Version); // uint32

		Read<uint32_t>(is); // Width
		Read<uint32_t>(is); // Height

		Read(is, result.FloorRenderingMode); // uint8;
		Read(is, result.BumpMappingEnabled); // uint8;

		uint32_t texStrSize = Read<uint32_t>(is);
		uint32_t bumpStrSize = Read<uint32_t>(is);
	
		if (texStrSize > 0)
			result.Texture = std::string(Read<char>(is, texStrSize).data(), texStrSize);

		if (bumpStrSize > 0)
			result.NormalMap = std::string(Read<char>(is, bumpStrSize).data(), bumpStrSize);


		Read(is, result.StartPoint); // ivec2
		Read(is, result.StartDirection); // ivec2
		Read(is, result.EmeraldColor); // vec3

		Read(is, result.CheckerColors); // 2 * vec3
		Read(is, result.SkyColors); // 2 * vec3
		Read(is, result.StarColors); // 2 * vec3

		Read(is, result.MaxRings); // uint32

		Read(is, result.m_Data); // 1024 bytes
		Read(is, result.m_AvoidSearch); // 1024 bytes;

		is.close();


		return result;
	}

	Stage::Stage()
	{

	}

	EStageObject Stage::GetValueAt(const glm::ivec2& position) const
	{
		return GetValueAt(position.x, position.y);
	}

	EStageObject Stage::GetValueAt(int32_t x, int32_t y) const
	{
		WrapCoordinate(x);
		WrapCoordinate(y);


		return m_Data[y][x];

	}
	void Stage::SetValueAt(int32_t x, int32_t y, EStageObject obj)
	{
		WrapCoordinate(x);
		WrapCoordinate(y);
		m_Data[y][x] = obj;
	}
	void Stage::SetValueAt(const glm::ivec2& position, EStageObject obj)
	{
		SetValueAt(position.x, position.y, obj);
	}
	void Stage::WrapCoordinate(int32_t& coord) const
	{
		while (coord < 0)
			coord += (int32_t)s_StageSize;

		coord = coord % s_StageSize;
	}
}