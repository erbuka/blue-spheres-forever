#include "GameSceneTest.h"
#include "Application.h"
#include "Renderer2D.h"
#include "Log.h"
#include "GameLogic.h"
#include "Assets.h"
#include "Texture.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>


namespace bsf
{


	void GameSceneTest::OnAttach(Application& app)
	{

		m_Renderer2D = MakeRef<Renderer2D>();

		m_Stage = MakeRef<Stage>();
		m_Stage->FromFile("assets/data/playground.bss");
		m_GameLogic = MakeRef<GameLogic>(*m_Stage);

		app.KeyPressed.Subscribe([&](const KeyPressedEvent& evt) {
			if (evt.KeyCode == GLFW_KEY_LEFT) 
			{
				m_GameLogic->Rotate(GameLogic::ERotate::Left);
			} 
			else if(evt.KeyCode == GLFW_KEY_RIGHT) 
			{
				m_GameLogic->Rotate(GameLogic::ERotate::Right);
			}
			else if (evt.KeyCode == GLFW_KEY_UP)
			{
				m_GameLogic->RunForward();
			}
			else if (evt.KeyCode == GLFW_KEY_SPACE)
			{
				m_GameLogic->Jump();
			}
		});

	}

	void GameSceneTest::OnRender(Application& app, const Time& time)
	{
		auto& assets = Assets::Get();
		auto size = app.GetWindowSize();
		
		float height = 32.0f;
		float width = size.x / size.y * height;

		glViewport(0, 0, size.x, size.y);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		auto pos = m_GameLogic->GetPosition();


		const int steps = 5;
		for(int i = 0; i < steps; i++)
			m_GameLogic->Advance({ time.Delta / steps, time.Elapsed });

		m_Renderer2D->Begin(glm::ortho(-width / 2.0f, width / 2.0f, -height / 2.0f, height / 2.0f, -1.0f, 1.0f));
		m_Renderer2D->Rotate(m_GameLogic->GetRotationAngle());



		int32_t ix = pos.x, iy = pos.y;
		float fx = pos.x - ix, fy = pos.y - iy;

		for (int32_t x = -10; x < 10; x++)
		{
			for (int32_t y = -10; y < 10; y++)
			{
				auto val = m_Stage->GetValueAt(x + ix, y + iy);

				if (val != EStageObject::None)
				{
					switch (val)
					{
					case EStageObject::RedSphere: m_Renderer2D->Color({ 1.0f, 0.0f, 0.0f, 1.0 }); break;
					case EStageObject::BlueSphere: m_Renderer2D->Color({ 0.0f, 0.0f, 1.0f, 1.0 }); break;
					case EStageObject::YellowSphere: m_Renderer2D->Color({ 1.0f, 1.0f, 0.0f, 1.0 }); break;
					case EStageObject::StarSphere: m_Renderer2D->Color({ 1.0f, 1.0f, 1.0f, 1.0 }); break;
					case EStageObject::Ring: m_Renderer2D->Color({ 0.0f, 1.0f, 0.0f, 1.0 }); break;
					}

					m_Renderer2D->Push();
					m_Renderer2D->Translate({ x - fx, y - fy });
					m_Renderer2D->DrawQuad({ 0, 0 });
					m_Renderer2D->Pop();
				}

			}
		}


		m_Renderer2D->Push();
		m_Renderer2D->Texture(std::dynamic_pointer_cast<Texture2D>(assets.GetTexture(AssetName::TexBumper)));
		m_Renderer2D->Color({ 1.0f, 0.0f, 1.0f, 1.0f });
		m_Renderer2D->Translate({ 0, 0 });
		m_Renderer2D->Scale({ 1.0f + m_GameLogic->GetHeight(),  1.0f + m_GameLogic->GetHeight() });
		m_Renderer2D->DrawQuad({ 0, 0 });
		m_Renderer2D->Pop();

		m_Renderer2D->End();

	}

	void GameSceneTest::OnDetach(Application& app)
	{
	}
}
