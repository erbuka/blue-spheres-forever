#include "Application.h"
#include "GameScene.h"
#include "Common.h"
#include "Log.h"


#include <glm/glm.hpp>
#include <glm/ext.hpp>

using namespace bsf;

using namespace glm;

int main() {
	auto scene = Ref<Scene>(new GameScene());
	Application app;
	app.GotoScene(std::move(scene));
	app.Start();
}