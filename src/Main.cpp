#include "BsfPch.h"

#include <json/json.hpp>

#include "Application.h"
#include "Stage.h"
#include "GameScene.h"
#include "Common.h"
#include "MenuScene.h"
#include "DisclaimerScene.h"
#include "SplashScene.h"
#include "StageClearScene.h"
#include "StageEditorScene.h"

#include "Renderer2D.h"

using namespace bsf;
using namespace glm;

// TODO Move convertion functions somewhere else

//void ConvertSections();


int main() 
{
	//auto scene = Ref<Scene>(new DisclaimerScene());
	auto scene = Ref<Scene>(new StageEditorScene());
	//auto scene = Ref<Scene>(new SplashScene());
	//auto scene = Ref<Scene>(new MenuScene());
	//auto scene = MakeRef<StageClearScene>(GameInfo{ GameMode::BlueSpheres, 10000, 1 }, 100, true);
	
	Application app;
	app.GotoScene(std::move(scene));
	app.Start();
	return 0;
}


template<typename T, size_t N>
std::vector<T> Flip(const std::vector<T>& v)
{
	std::vector<T> data(v);

	for (uint32_t y = 0; y < N / 2; y++)
	{
		for (uint32_t x = 0; x < N; x++)
		{
			std::swap(data[y * N + x], data[(N - y - 1) * N + x]);
		}
	}

	return data;
}

// Load sections as binary and save as json
/*
void ConvertSections()
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

		auto data2 = Flip<uint32_t, 16>(data);
		auto avoidSearch2 = Flip<uint8_t, 16>(avoidSearch);
		
		assert((data == Flip<uint32_t, 16>(data2)));
		assert((avoidSearch == Flip<uint8_t, 16>(avoidSearch2)));

		section["data"] = data;
		section["avoidSearch"] = avoidSearch;

		sections.push_back(section);
		BSF_INFO("Converted stage section #{0}", i);

	}

	nis.close();

	std::ofstream os;

	os.open("assets/data/sections.json");

	os << sections.dump();

	os.close();

}
*/