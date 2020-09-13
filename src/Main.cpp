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
#include "GLTF.h"
#include "Assets.h"
#include "ShaderProgram.h"
#include "VertexArray.h"
#include "Texture.h"

using namespace bsf;
using namespace glm;

// TODO Move convertion functions somewhere else

//void ConvertSections();


class TestScene : public Scene
{


	Ref<GLTF> gltf;
	Ref<ShaderProgram> prog;

	MatrixStack m_Projection, m_Model, m_View;


	void OnAttach() override 
	{
		gltf = MakeRef<GLTF>();
		gltf->Load("assets/models/sonic.gltf", { GLTFAttributes::Position, GLTFAttributes::Normal, GLTFAttributes::Uv });

		prog = ShaderProgram::FromFile("assets/shaders/test.vert", "assets/shaders/test.frag");

	}

	void OnRender(const Time& time) override
	{
		auto& assets = Assets::GetInstance();
		auto windowSize = GetApplication().GetWindowSize();

		glEnable(GL_DEPTH_TEST);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_Projection.Reset();
		m_Projection.Perspective(glm::radians(45.0f), windowSize.x / windowSize.y, 0.1f, 100.0f);

		m_View.Reset();
		m_View.LookAt({ 0.0f, -3.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f });

		m_Model.Reset();
		m_Model.Rotate({ 0.0f, 0.0f, -1.0f }, time.Elapsed);

		prog->Use();

		prog->UniformMatrix4f("uProjection", m_Projection);
		prog->UniformMatrix4f("uView", m_View);
		prog->UniformMatrix4f("uModel", m_Model);

		GLTFRenderConfig config;

		config.Program = prog;
		config.BaseColorUniform = "uColor";
		config.BaseColorTextureUniform = "uMap";
		config.ModelMatrixUniform = "uModel";

		gltf->Render(time, config);



		
	}
};


int main() 
{

	//auto scene = MakeRef<TestScene>();
	auto scene = Ref<Scene>(new DisclaimerScene());
	//auto scene = Ref<Scene>(new StageEditorScene());
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