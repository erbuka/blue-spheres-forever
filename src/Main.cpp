#include "BsfPch.h"

#include "Application.h"
#include "DisclaimerScene.h"

#include "Character.h"
#include "VertexArray.h"


namespace bsf
{
	void Run()
	{
		auto scene = Ref<Scene>(new DisclaimerScene());
		Application app;
		app.GotoScene(std::move(scene));
		app.Start();
	}
}



int main(int argc, char** argv)
{
	bsf::Run();
	return 0;
}
