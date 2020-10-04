#include "BsfPch.h"

#include <thread>

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

#include <glm/gtc/noise.hpp>
#include <imgui.h>
#include <fmt/core.h>

using namespace bsf;
using namespace glm;

// TODO Move convertion functions somewhere else

//void ConvertSections();


class TestScene : public Scene
{
	glm::vec3 bgBaseColor = Colors::Blue;

	float bgNoiseScale = 4.0f;
	float bgDarkenFactor = 0.9f;

	float starPow = 8.0f;
	float starMultipler = 24.0f;
	float starBrightnessScale = 200.0f;
	float starScale = 180.0f;

	float cloudScale = 4.0f;

	Ref<VertexArray> quad;
	Ref<ShaderProgram> shader;

	void OnAttach() override 
	{
		shader = ShaderProgram::FromFile("assets/shaders/sky_generator/sky_gen.vert", "assets/shaders/sky_generator/sky_gen.frag");

		std::array<glm::vec3, 12> vertexData = {
			glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f),
			glm::vec3(+1.0f, -1.0f, -1.0f), glm::vec3(0.0f),
			glm::vec3(+1.0f, +1.0f, -1.0f), glm::vec3(0.0f),

			glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f),
			glm::vec3(+1.0f, +1.0f, -1.0f), glm::vec3(0.0f),
			glm::vec3(-1.0f, +1.0f, -1.0f), glm::vec3(0.0f),
		};

		auto vb = Ref<VertexBuffer>(new VertexBuffer({
			{ "aPosition", AttributeType::Float3 },
			{ "aUv", AttributeType::Float3 }
		}, (void*)vertexData.data(), 6));
	
		quad = Ref<VertexArray>(new VertexArray(vb->GetVertexCount(), { vb }));

	}

	void OnRender(const Time& time) override
	{
		auto size = GetApplication().GetWindowSize();
		
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, size.x, size.y);

		float aspect = size.x / size.y;

		glm::mat4 proj = glm::ortho(-1.0f * aspect, 1.0f * aspect, -1.0f, 1.0f);

		shader->Use();
		shader->UniformMatrix4f("uProjection", proj);
		shader->UniformMatrix4f("uView", glm::identity<glm::mat4>());
		shader->UniformMatrix4f("uModel", glm::identity < glm::mat4>());

		shader->Uniform3fv("uBackColor", 1, glm::value_ptr(bgBaseColor));

		shader->Uniform3fv("uStarColor", 1, glm::value_ptr(Colors::White));
		
		shader->Uniform1fv("uBackgroundScale", 1, &bgNoiseScale);
		shader->Uniform1fv("uStarBrightnessScale", 1, &starBrightnessScale);
		shader->Uniform1fv("uStarScale", 1, &starScale);
		shader->Uniform1fv("uStarPower", 1, &starPow);
		shader->Uniform1fv("uStarMultipler",1, &starMultipler);

		shader->Uniform1fv("uCloudScale", 1, &cloudScale);


		quad->Draw(GL_TRIANGLES);

		ImGui::Begin("Test");
		
		ImGui::DragFloat("Bg Darken Factor", &bgDarkenFactor, 0.01f, 0.0f, 1.0f);
		
		ImGui::ColorEdit3("Base Color", glm::value_ptr(bgBaseColor));

		ImGui::DragFloat("Bg Noise Scale", &bgNoiseScale, 1.0f, 1.0f, 500.0f);

		ImGui::DragFloat("Stars Noise Scale", &starScale, 0.1f, 1.0f, 500.0f);

		ImGui::DragFloat("Stars Brighness Noise Scale", &starBrightnessScale, 0.1f, 1.0f, 500.0f);

		ImGui::DragFloat("Stars Multipler", &starMultipler, 0.1f, 1.0f, 100.0f);

		ImGui::DragFloat("Stars Power", &starPow, 0.1f, 1.0f, 100.0f);

		ImGui::DragFloat("Clouds Scale", &cloudScale, 0.1f, 1.0f, 500.0f);

		
		ImGui::End();



	}
};

int main() 
{
	
	//auto scene = MakeRef<TestScene>();
	//auto scene = Ref<Scene>(new DisclaimerScene());
	//auto scene = Ref<Scene>(new StageEditorScene());
	auto scene = Ref<Scene>(new SplashScene());
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