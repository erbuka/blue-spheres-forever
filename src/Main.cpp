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
#include <fmt/core.h>

// TODO Move convertion functions somewhere else

//void ConvertSections();

namespace bsf
{
	void Run()
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
	}
}

#ifdef _WIN32
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	bsf::Run();
	return 0;
}
#else
int main(int argc, char** argv)
{
	bsf::Run();
	return 0;
}
#endif


