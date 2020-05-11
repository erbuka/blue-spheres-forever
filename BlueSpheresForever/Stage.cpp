#include "BsfPch.h"

#include "Stage.h"
#include "Common.h"
#include "Log.h";


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

		is.Read<uint32_t>(Version);

		auto width = is.Read<uint32_t>(); // Width
		auto height = is.Read<uint32_t>(); // Height

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

		is.Read(MaxRings); // uint32

		is.ReadSome(m_Data.size(), m_Data.data()); // 1024 bytes
		is.ReadSome(m_AvoidSearch.size(), m_Data.data()); // 1024 bytes;

		stdIs.close();

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