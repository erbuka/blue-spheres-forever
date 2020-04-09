#include "Application.h"
#include "GameScene.h"
#include "Common.h"

using namespace bsf;

int main() {
	auto scene = Ref<Scene>(new GameScene());
	Application app;
	app.GotoScene(std::move(scene));
	app.Start();
}