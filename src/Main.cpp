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

#include "Character.h"
#include "VertexArray.h"

namespace bsf
{

	/*
	class TestScene : public Scene
	{
	private:
		MatrixStack m_Projection, m_View, m_Model;
		Ref<Character> m_Character;
		Ref<ShaderProgram> m_Skel, m_Test;

		glm::vec3 m_CameraPos = { 0, 0, 6 };

	public:

		void OnAttach() override
		{
			m_Character = Assets::GetInstance().Get<Character>(AssetName::ChrSonic);
			m_Skel = ShaderProgram::FromFile("assets/shaders/test.vert", "assets/shaders/test.frag", { "SKELETAL" });
			m_Test = ShaderProgram::FromFile("assets/shaders/test.vert", "assets/shaders/test.frag");
			m_Character->PlayAnimation(CharacterAnimation::Ball, true, 0.25f);
		}
		void OnRender(const Time& time) override
		{
			auto& assets = Assets::GetInstance();
			auto windowSize = GetApplication().GetWindowSize();

			const auto& texBlack = assets.Get<Texture2D>(AssetName::TexBlack);
			const auto& texWhite = assets.Get<Texture2D>(AssetName::TexWhite);
			const auto& modSphere = assets.Get<VertexArray>(AssetName::ModSphere);

			m_View.Reset();
			m_Projection.Reset();
			m_Model.Reset();

			m_Projection.LoadIdentity();
			m_Projection.Perspective(glm::radians(45.0f), windowSize.x / windowSize.y, 0.1, 30);

			m_View.LoadIdentity();
			m_View.LookAt(m_CameraPos, { 0, 0, 0 });

			m_Model.LoadIdentity();

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glEnable(GL_DEPTH_TEST);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


			m_Test->Use();
			m_Test->UniformMatrix4f(HS("uProjection"), m_Projection.GetMatrix());
			m_Test->UniformMatrix4f(HS("uView"), m_View.GetMatrix());
			m_Test->UniformMatrix4f(HS("uModel"), m_Model.GetMatrix());
			m_Test->UniformTexture(HS("uMap"), texWhite);

			modSphere->DrawArrays(GL_TRIANGLES);


			m_Model.Rotate(glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(90.0f));

			m_Skel->Use();
			m_Skel->UniformMatrix4f(HS("uProjection"), m_Projection.GetMatrix());
			m_Skel->UniformMatrix4f(HS("uView"), m_View.GetMatrix());
			m_Skel->UniformMatrix4f(HS("uModel"), m_Model.GetMatrix());


			GLTFRenderConfig config;
			config.Program = m_Skel;
			m_Character->Update(time);
			m_Character->Render(time, config);

		}
	};
	*/

	void Run()
	{
		//auto scene = MakeRef<TestScene>();
		auto scene = Ref<Scene>(new DisclaimerScene());
		//auto scene = Ref<Scene>(new StageEditorScene());
		//auto scene = Ref<Scene>(new SplashScene());
		//auto scene = Ref<Scene>(new MenuScene());
		//auto scene = MakeRef<StageClearScene>(GameInfo{ GameMode::BlueSpheres, 10000, 1 }, 100, true);
		//auto scene = MakeRef<TestScene>();

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
#endif
#else
int main(int argc, char** argv)
{
	bsf::Run();
	return 0;
}
#endif


