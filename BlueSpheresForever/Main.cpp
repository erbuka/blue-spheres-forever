#include "BsfPch.h"

#include <json/json.hpp>

#include "Application.h"
#include "Stage.h"
#include "GameScene.h"
#include "Common.h"
#include "MenuScene.h"
#include "DisclaimerScene.h"

using namespace bsf;
using namespace glm;

static void LoadSectionsBinary();


int main() 
{

	//auto stage = StageGenerator().Generate(365989603263);
	auto stage = MakeRef<Stage>();
	stage->FromFile("assets/data/s3stage1.bss");
	//auto scene = Ref<Scene>(new GameScene(stage));
	auto scene = Ref<Scene>(new DisclaimerScene());
	//auto scene = Ref<Scene>(new MenuScene());

	Application app;
	app.GotoScene(std::move(scene));
	app.Start();
}




// Load sections as binary and save as json
static void LoadSectionsBinary()
{

	using json = nlohmann::json;

	std::ifstream nis;

	nis.open("assets/data/sections.dat", std::ios_base::binary);

	if (!nis.is_open())
	{
		BSF_ERROR("Can't open sections binary file");
		return;
	}

	InputStream<ByteOrder::LittleEndian> is(nis);
	auto count = is.Read<uint32_t>();

	auto sections = json::array();

	for (uint32_t i = 0; i < count; i++)
	{
		auto section = json::object();

		section["maxRings"] = is.Read<uint32_t>();

		std::vector<uint32_t> data(16 * 16);
		is.ReadSome(data.size(), data.data());

		std::vector<uint8_t> avoidSearch(16 * 16);
		is.ReadSome(avoidSearch.size(), avoidSearch.data());


		for (uint32_t y = 0; y < 16; y++)
		{
			for (uint32_t x = 0; x < 16; x++)
			{
				std::swap(data[y * 16 + x], data[(16 - y - 1) * 16 + x]);
				std::swap(avoidSearch[y * 16 + x], avoidSearch[(16 - y - 1) * 16 + x]);
			}
		}

		section["data"] = data;
		section["avoidSearch"] = avoidSearch;

		sections.push_back(section);

	}

	nis.close();

	std::ofstream os;

	os.open("assets/data/sections.json");

	os << sections.dump();

	os.close();

}

