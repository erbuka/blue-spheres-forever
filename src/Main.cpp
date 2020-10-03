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
	static constexpr size_t size = 512;

	float zoom = 1.0f;
	
	float starsPow = 8.0f;
	float starsMultipler = 1.0f;
	float starsBrightnessNoiseScale = 1.0f;
	float starsNoiseScale = 2.0f;

	float bgNoiseScale = 100.0f;
	float bgDarkenFactor = 0.75f;
	glm::vec3 bgBaseColor = Colors::Blue;

	Ref<Texture2D> noiseTex;
	std::vector<uint32_t> pixels;
	std::vector<glm::vec3> colors;

	void UpdateNoise()
	{
		constexpr size_t numThreads = 4;
		constexpr size_t sliceSize = size / numThreads;
		std::array<std::thread, numThreads> threads;
		auto baseColor2 = (glm::vec3)Darken(glm::vec4(bgBaseColor, 1.0f), bgDarkenFactor);

		auto updateSlice = [&](size_t xMin, size_t xMax) {
			for (size_t x = xMin; x < xMax; ++x)
			{
				for (size_t y = 0; y < size; ++y)
				{
					float val = glm::simplex(glm::vec3(x, y, 0.0) / (float)bgNoiseScale) * 0.5f + 0.5f;
					colors[y * size + x] = glm::lerp(baseColor2, bgBaseColor, val);
				}
			}

			for (size_t x = xMin; x < xMax; ++x)
			{
				for (size_t y = 0; y < size; ++y)
				{
					float skyVal = 0.5f + glm::simplex(glm::vec3(x, y, 0.0) / (float)bgNoiseScale) * 0.25f + 0.25f;
					float brightness = glm::simplex(glm::vec3(x, y, 0.0) / (float)starsBrightnessNoiseScale) * 0.5f + 0.5f;
					float val = glm::simplex(glm::vec3(x, y, 0.0) / (float)starsNoiseScale) * 0.5f + 0.5f;
					val = glm::pow(val * skyVal * brightness, starsPow);
					colors[y * size + x] += glm::vec3(val) *starsMultipler;
				}
			}


			// Normalize + transfer
			for (size_t x = xMin; x < xMax; ++x)
			{
				for (size_t y = 0; y < size; ++y)
				{
					colors[y * size + x] /= (colors[y * size + x] + 1.0f);
					pixels[y * size + x] = ToHexColor(colors[y * size + x]);
				}
			}
		};

		for (size_t i = 0; i < numThreads; ++i)
			threads[i] = std::thread(updateSlice, i * sliceSize, (i + 1) * sliceSize);
		
		for (size_t i = 0; i < numThreads; ++i)
			threads[i].join();


		noiseTex->SetPixels(pixels.data(), size, size);

	}

	void OnAttach() override 
	{
		pixels.reserve(size * size);
		colors.reserve(size * size);

		noiseTex = MakeRef<Texture2D>(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);

		UpdateNoise();

	}

	void OnRender(const Time& time) override
	{
		auto size = GetApplication().GetWindowSize();
		auto& r2 = GetApplication().GetRenderer2D();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, size.x, size.y);

		float aspect = size.x / size.y;

		r2.Begin(glm::ortho(-1.0f * aspect, 1.0f * aspect, -1.0f, 1.0f, -1.0f, 1.0f));

		r2.Pivot(EPivot::Center);

		r2.Color(Colors::White);
		r2.Texture(noiseTex);
		r2.DrawQuad({}, { zoom,zoom });

		r2.End();

		ImGui::Begin("Test");
		
		if (ImGui::DragFloat("Zoom", &zoom, 0.01f, 1.0f, 10.0f))
			UpdateNoise();

		if (ImGui::DragFloat("Bg Darken Factor", &bgDarkenFactor, 0.01f, 0.0f, 1.0f))
			UpdateNoise();

		if (ImGui::DragFloat("Bg Noise Scale", &bgNoiseScale, 1.0f, 1.0f, 500.0f))
			UpdateNoise();

		if (ImGui::DragFloat("Stars Noise Scale", &starsNoiseScale, 0.1f, 1.0f, 10.0f))
			UpdateNoise();

		if (ImGui::DragFloat("Stars Brighness Noise Scale", &starsBrightnessNoiseScale, 0.1f, 1.0f, 10.0f))
			UpdateNoise();

		if (ImGui::DragFloat("Stars Multipler", &starsMultipler, 0.1f, 1.0f, 100.0f))
			UpdateNoise();

		if (ImGui::DragFloat("Stars Power", &starsPow, 0.1f, 1.0f, 1.0f))
			UpdateNoise();

		if (ImGui::ColorEdit3("Base Color", glm::value_ptr(bgBaseColor)))
			UpdateNoise();
		
		ImGui::End();



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