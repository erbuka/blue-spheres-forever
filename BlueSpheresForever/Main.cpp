#include "BsfPch.h"

#include "Application.h"
#include "Stage.h"
#include "GameScene.h"
#include "Common.h"

#include "DisclaimerScene.h"

using namespace bsf;
using namespace glm;

int main() {
	

	auto stage = MakeRef<Stage>();
	//stage->FromFile("assets/data/playground.bss");
	stage->FromFile("assets/data/s3stage1.bss");
	auto scene = Ref<Scene>(new GameScene(stage));
	//auto scene = Ref<Scene>(new DisclaimerScene());
	
	Application app;
	app.GotoScene(std::move(scene));
	app.Start();
}

