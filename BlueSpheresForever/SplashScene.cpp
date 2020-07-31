#include "BsfPch.h"

#include "SplashScene.h"
#include "Assets.h"
#include "Application.h"
#include "Model.h"
#include "ShaderProgram.h"
#include "Framebuffer.h"
#include "VertexArray.h"
#include "Texture.h"
#include "Audio.h"
#include "MenuScene.h"
#include "SkyGenerator.h"

namespace bsf
{
	static constexpr float s_SplashTime = 5.0f;
	static constexpr float s_FadeOutDuration = 0.5f;
	static constexpr float s_FadeOutTime = s_SplashTime - s_FadeOutDuration;
	static constexpr float s_EmeraldAngularVelocity = glm::pi<float>();
	static constexpr float s_EmeraldStartingRadius = 4.0f;

	void SplashScene::OnAttach()
	{
		auto& assets = Assets::GetInstance();
		auto& app = GetApplication();
		auto windowSize = app.GetWindowSize();

		// Sky
		m_Sky = assets.Get<SkyGenerator>(AssetName::SkyGenerator)->Generate({ 
			1024,
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f)
		});

		// Framebuffers
		m_fbDeferred = MakeRef<Framebuffer>(windowSize.x, windowSize.y, true);
		m_fbDeferred->AddColorAttachment("color", GL_RGB16F, GL_RGB, GL_HALF_FLOAT);

		// Shaders
		m_pPBR = ShaderProgram::FromFile("assets/shaders/pbr.vert", "assets/shaders/pbr.frag", { "NO_SHADOWS", "NO_UV_OFFSET" });
		m_pDeferred = ShaderProgram::FromFile("assets/shaders/deferred.vert", "assets/shaders/deferred.frag");
		m_pSky = ShaderProgram::FromFile("assets/shaders/skybox.vert", "assets/shaders/skybox.frag");

		// Event handlers
		m_Subscriptions.push_back(app.WindowResized.Subscribe(this, &SplashScene::OnResize));

		// fadeIn
		auto fadeIn = MakeRef<FadeTask>(glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.5f);
		ScheduleTask<FadeTask>(ESceneTaskEvent::PostRender, fadeIn);

		// fadeOut
		auto waitFadeOut = MakeRef<WaitForTask>(s_FadeOutTime);
		waitFadeOut->SetDoneFunction([&](SceneTask& self) {
			auto fadeOut = MakeRef<FadeTask>(glm::vec4{ 1.0f, 1.0f, 1.0f, 0.0f }, 
				glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, 
				s_FadeOutDuration);

			fadeOut->SetDoneFunction([&](SceneTask& self) {
				// Goto empty scene for now
				app.GotoScene(MakeRef<MenuScene>());
			});

			ScheduleTask<FadeTask>(ESceneTaskEvent::PostRender, fadeOut);
		});

		ScheduleTask<WaitForTask>(ESceneTaskEvent::PostRender, waitFadeOut);

		// Play intro sound
		auto playIntroSound = MakeRef<SceneTask>();
		playIntroSound->SetUpdateFunction([&](SceneTask& self, const Time& time) {
			Assets::GetInstance().Get<Audio>(AssetName::SfxIntro)->Play();
			self.SetDone();
		});

		ScheduleTask<SceneTask>(ESceneTaskEvent::PreRender, playIntroSound);

	}
	
	void SplashScene::OnRender(const Time& time)
	{
		static constexpr std::array<glm::vec4, 7> colors = {
			glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
			glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
			glm::vec4(0.0f, 1.0f, 1.0f, 1.0f),
			glm::vec4(0.7f, 0.7f, 0.7f, 1.0f),
			glm::vec4(1.0f, 0.0f, 1.0f, 1.0f),
			glm::vec4(0.5f, 0.0f, 1.0f, 1.0f)
		};

		auto windowSize = GetApplication().GetWindowSize();
		auto& assets = Assets::GetInstance();
		auto& emerald = assets.Get<Model>(AssetName::ModChaosEmerald)->GetMesh(0);

		// Rotate sky
		m_Sky->ApplyMatrix(glm::rotate(time.Delta, glm::vec3{ 0.0f, 1.0f, 0.0f }));

		// Draw to deferred framebuffer
		m_fbDeferred->Bind();
		{
			GLEnableScope scope({ GL_DEPTH_TEST });
			glEnable(GL_DEPTH_TEST);
			
			glViewport(0, 0, windowSize.x, windowSize.y);

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			m_Projection.LoadIdentity();
			m_Projection.Perspective(glm::pi<float>() / 4.0f, windowSize.x / windowSize.y, 0.1f, 10.0f);

			// Sky
			{

				m_View.LoadIdentity();
				m_Model.LoadIdentity();
				
				glDepthMask(GL_FALSE);
				m_pSky->Use();
				m_pSky->UniformMatrix4f("uProjection", m_Projection);
				m_pSky->UniformMatrix4f("uView", m_View);
				m_pSky->UniformMatrix4f("uModel", m_Model);
				m_pSky->UniformTexture("uSkyBox", m_Sky->GetEnvironment(), 0);
				assets.Get<VertexArray>(AssetName::ModSkyBox)->Draw(GL_TRIANGLES);
				glDepthMask(GL_TRUE);
			}

			// Emeralds
			{
				m_View.LoadIdentity();
				m_View.Translate({ 0.0f, 0.0f, -5.0f });

				m_Model.LoadIdentity();

				glm::vec3 lightPos(0.0f, 0.0f, 1.0f);
				glm::vec3 cameraPos = glm::inverse(m_View.GetMatrix()) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

				m_pPBR->Use();

				m_pPBR->UniformMatrix4f("uProjection", m_Projection.GetMatrix());
				m_pPBR->UniformMatrix4f("uView", m_View.GetMatrix());

				m_pPBR->Uniform3fv("uCameraPos", 1, glm::value_ptr(cameraPos));
				m_pPBR->Uniform3fv("uLightPos", 1, glm::value_ptr(lightPos));

				m_pPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexWhite), 0);
				m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexWhite), 1);
				m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexBlack), 2);
				m_pPBR->UniformTexture("uAo", assets.Get<Texture2D>(AssetName::TexWhite), 3);

				m_pPBR->UniformTexture("uBRDFLut", assets.Get<Texture2D>(AssetName::TexBRDFLut), 4);
				m_pPBR->UniformTexture("uEnvironment", m_Sky->GetEnvironment(), 5);
				m_pPBR->UniformTexture("uIrradiance", m_Sky->GetIrradiance(), 6);

				for (uint32_t i = 0; i < 7; i++)
				{
					float angle = 2.0f * glm::pi<float>() / 7.0f * i + time.Elapsed * glm::pi<float>();
					float radius = s_EmeraldStartingRadius * std::max(0.0f, s_SplashTime - time.Elapsed) / s_SplashTime;

					m_Model.Push();

					m_Model.Translate({ std::cos(angle) * radius, std::sin(angle) * radius, 0.0f });
					m_Model.Rotate({ 0.0f, 1.0f, 0.0f }, angle + time.Elapsed * s_EmeraldAngularVelocity);
					m_Model.Rotate({ 1.0f, 0.0f, 0.0f }, angle + time.Elapsed * s_EmeraldAngularVelocity);

					m_pPBR->UniformMatrix4f("uModel", m_Model.GetMatrix());
					m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(colors[i]));

					emerald->Draw(GL_TRIANGLES);

					m_Model.Pop();

				}

			}

		}
		m_fbDeferred->Unbind();


		// Draw to screen
		{
			GLEnableScope scope({ GL_DEPTH_TEST }) ;

			glViewport(0, 0, windowSize.x, windowSize.y);

			glDisable(GL_DEPTH_TEST);

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			m_pDeferred->Use();
			m_pDeferred->UniformTexture("uColor", m_fbDeferred->GetColorAttachment("color"), 0);
			m_pDeferred->Uniform1f("uExposure", { 1.0f });
			assets.Get<VertexArray>(AssetName::ModClipSpaceQuad)->Draw(GL_TRIANGLES);
		

		}

	}
	
	void SplashScene::OnDetach()
	{
		for (auto& unsubscribe : m_Subscriptions)
			unsubscribe();
	}

	void SplashScene::OnResize(const WindowResizedEvent& evt)
	{
		m_fbDeferred->Resize(evt.Width, evt.Height);
	}


}