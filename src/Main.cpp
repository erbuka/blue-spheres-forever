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

using namespace bsf;
using namespace glm;

// TODO Move convertion functions somewhere else

void ConvertSections();
void ConvertStages();


int main() 
{
	ConvertStages();
	ConvertSections();

	//auto scene = Ref<Scene>(new DisclaimerScene());
	//auto scene = Ref<Scene>(new StageEditorScene());
	//auto scene = Ref<Scene>(new SplashScene());
	auto scene = Ref<Scene>(new MenuScene());
	//auto scene = MakeRef<StageClearScene>(GameInfo{ GameMode::BlueSpheres, 10000, 1 }, 100, true);
	
	Application app;
	app.GotoScene(std::move(scene));
	app.Start();

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

		std::cout << i << std::endl;
		
		assert((data == Flip<uint32_t, 16>(data2)));
		assert((avoidSearch == Flip<uint8_t, 16>(avoidSearch2)));

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

void ConvertStages()
{
	namespace fs = std::filesystem;
	std::vector<Ref<Stage>> result;

	Stage stage;

	auto flipStage = [](const Stage& s) {
		Stage stage(s);

		auto& data = stage.GetData();
		auto& avoid = stage.GetAvoidSearch();
		auto w = stage.GetWidth();
		auto h = stage.GetHeight();

		for (size_t y = 0; y < stage.GetHeight() / 2; y++)
		{
			for (size_t x = 0; x < stage.GetWidth(); x++)
			{
				std::swap(data[y * w + x], data[(h - (y + 1)) * w + x]);
				std::swap(avoid[y * w + x], avoid[(h - (y + 1)) * w + x]);
			}
		}
		stage.StartPoint.y = stage.GetHeight() - (stage.StartPoint.y + 1);
		stage.StartDirection.y = -stage.StartDirection.y;

		return stage;
	};


	for (auto& entry : fs::directory_iterator("assets/data"))
	{
		auto path = entry.path();
		if (entry.is_regular_file() && path.extension() == ".bss")
		{
			stage.FromFile(path.string());
			stage.Name = path.filename().string();

			// Need to flip here
			auto flipped = flipStage(stage);

			assert((stage == flipStage(flipped)));

			path.replace_extension(".bssj");
			flipped.Save(path.string());
		}
	}

}

// Convert the stages to JSON