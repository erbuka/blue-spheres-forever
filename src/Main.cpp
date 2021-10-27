#include "BsfPch.h"

#include "Application.h"
#include "DisclaimerScene.h"



#include <glm/gtc/noise.hpp>
#include <fmt/core.h>

// TODO Move convertion functions somewhere else

//void ConvertSections();

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

#ifdef BSF_DISTRIBUTION
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
#else
int main(int argc, char** argv)
{
	bsf::Run();
	return 0;
}
#endif
