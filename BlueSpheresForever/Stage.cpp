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

		is.Read(MaxRings); // uint32

		m_Data.resize(m_Width * m_Height);
		m_AvoidSearch.resize(m_Width * m_Height);

		is.ReadSome(m_Data.size(), m_Data.data()); // 1024 bytes
		is.ReadSome(m_AvoidSearch.size(), m_AvoidSearch.data()); // 1024 bytes;

		stdIs.close();

	}

	EStageObject Stage::GetValueAt(const glm::ivec2& position) const
	{
		return GetValueAt(position.x, position.y);
	}

	EStageObject Stage::GetValueAt(int32_t x, int32_t y) const
	{
		WrapX(x);
		WrapY(y);


		return m_Data[y * m_Width + x];

	}
	void Stage::SetValueAt(int32_t x, int32_t y, EStageObject obj)
	{
		WrapX(x);
		WrapY(y);
		m_Data[y * m_Width + x] = obj;
	}
	void Stage::SetValueAt(const glm::ivec2& position, EStageObject obj)
	{
		SetValueAt(position.x, position.y, obj);
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
}