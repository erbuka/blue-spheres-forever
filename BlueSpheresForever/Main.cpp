#include "Application.h"
#include "GameScene.h"
#include "Common.h"

using namespace bsf;

enum class X {
	Test, test
};

struct A {
	std::string v;
	X x;
};

void func(const std::initializer_list<A>& a)
{

}

int main() {
	auto scene = Ref<Scene>(new GameScene());
	Application app;
	app.GotoScene(std::move(scene));
	app.Start();
}