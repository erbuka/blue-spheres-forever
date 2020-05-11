#include "BsfPch.h"

#include "Application.h"
#include "Stage.h"
#include "GameScene.h"
#include "Common.h"

using namespace bsf;
using namespace glm;

int main() {

	auto stage = MakeRef<Stage>();
		
	stage->FromFile("assets/data/s3stage1.bss");

	auto scene = Ref<Scene>(new GameScene(stage));
	Application app;
	app.GotoScene(std::move(scene));
	app.Start();
}