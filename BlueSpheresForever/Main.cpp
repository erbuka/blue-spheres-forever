#include "Application.h"
#include "GameScene.h"
#include "Common.h"
#include "Log.h"
#include "Texture.h"


#include <glm/glm.hpp>
#include <glm/ext.hpp>

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