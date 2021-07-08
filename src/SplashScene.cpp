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
#include "BlurFilter.h"
#include "Color.h"
#include "Renderer2D.h"
#include "Font.h"

namespace bsf
{
	static constexpr float s_VirtualHeight = 10.0f;
	static constexpr float s_SplashTime = 5.0f;
	static constexpr float s_FadeDuration = 0.5f;
	static constexpr float s_FadeOutTime = s_SplashTime - s_FadeDuration;
	static constexpr float s_EmeraldAngularVelocity = glm::pi<float>();
	static constexpr float s_EmeraldStartingRadius = 4.0f;

	void SplashScene::OnAttach()
	{
		auto &assets = Assets::GetInstance();
		auto &app = GetApplication();
		auto windowSize = app.GetWindowSize();

		// Sky
		m_Sky = GenerateDefaultSky();

		// Framebuffers
		m_fbPBR = MakeRef<Framebuffer>(windowSize.x, windowSize.y, true);
		m_fbPBR->CreateColorAttachment("color", GL_RGB16F, GL_RGB, GL_HALF_FLOAT);
		m_fbPBR->CreateColorAttachment("emission", GL_RGB16F, GL_RGB, GL_HALF_FLOAT)->SetFilter(TextureFilter::Linear, TextureFilter::Linear);

		// PP
		m_fBlur = MakeRef<BlurFilter>(m_fbPBR->GetColorAttachment("emission"));

		// Shaders
		m_pPBR = ShaderProgram::FromFile("assets/shaders/pbr.vert", "assets/shaders/pbr.frag", {"NO_SHADOWS", "NO_UV_OFFSET"});
		m_pDeferred = ShaderProgram::FromFile("assets/shaders/deferred.vert", "assets/shaders/deferred.frag");
		m_pSky = ShaderProgram::FromFile("assets/shaders/skybox.vert", "assets/shaders/skybox.frag");

		// Event handlers
		AddSubscription(app.WindowResized, this, &SplashScene::OnResize);

		// fadeIn
		auto fadeIn = MakeRef<FadeTask>(glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, glm::vec4{1.0f, 1.0f, 1.0f, 0.0f}, 0.5f);
		ScheduleTask(ESceneTaskEvent::PostRender, fadeIn);

		{
			auto wait = MakeRef<WaitForTask>(s_FadeOutTime);
			auto fadeOut = MakeRef<FadeTask>(glm::vec4{1.0f, 1.0f, 1.0f, 0.0f}, glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, s_FadeDuration);
			auto change = MakeRef<SceneTask>([&](SceneTask& self, const Time& time) { m_DisplayTitle = true; self.SetDone(); });
			auto fadeIn = MakeRef<FadeTask>(glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, glm::vec4{1.0f, 1.0f, 1.0f, 0.0f}, s_FadeDuration);
			wait->Chain(fadeOut)->Chain(change)->Chain(fadeIn);
			ScheduleTask(ESceneTaskEvent::PostRender, wait);
		}

		// Play intro sound
		auto playIntroSound = MakeRef<SceneTask>();
		playIntroSound->SetUpdateFunction([&](SceneTask &self, const Time &time) {
			Assets::GetInstance().Get<Audio>(AssetName::SfxIntro)->Play();
			self.SetDone();
		});

		ScheduleTask(ESceneTaskEvent::PreRender, playIntroSound);

		AddSubscription(GetApplication().KeyPressed, [&](const KeyPressedEvent &evt) {
			if (evt.KeyCode == GLFW_KEY_ENTER)
			{
				auto fadeOut = MakeRef<FadeTask>(glm::vec4{1.0f, 1.0f, 1.0f, 0.0f}, glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, 0.5f);

				fadeOut->SetDoneFunction([&](SceneTask &self) {
					GetApplication().GotoScene(MakeRef<MenuScene>());
				});

				ScheduleTask(ESceneTaskEvent::PostRender, fadeOut);
			}
		});
	}

	void SplashScene::OnRender(const Time &time)
	{

		auto windowSize = GetApplication().GetWindowSize();
		auto &r2 = GetApplication().GetRenderer2D();
		auto &assets = Assets::GetInstance();

		// Rotate sky
		m_Sky->ApplyMatrix(glm::rotate(time.Delta, glm::vec3{0.0f, 1.0f, 0.0f}));

		// Draw to deferred framebuffer
		m_fbPBR->Bind();
		{
			GLEnableScope scope({GL_DEPTH_TEST, GL_BLEND});
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);

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
				m_pSky->UniformMatrix4f(HS("uProjection"), m_Projection);
				m_pSky->UniformMatrix4f(HS("uView"), m_View);
				m_pSky->UniformMatrix4f(HS("uModel"), m_Model);
				m_pSky->UniformTexture(HS("uSkyBox"), m_Sky->GetEnvironment());
				assets.Get<VertexArray>(AssetName::ModSkyBox)->DrawArrays(GL_TRIANGLES);
				glDepthMask(GL_TRUE);
			}

			if (!m_DisplayTitle)
				DrawEmeralds(time);
		}
		m_fbPBR->Unbind();

		// Post processing
		m_fBlur->Apply(3, 2);

		// Draw to screen
		{
			GLEnableScope scope({GL_DEPTH_TEST});

			glViewport(0, 0, windowSize.x, windowSize.y);

			glDisable(GL_DEPTH_TEST);

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			m_pDeferred->Use();
			m_pDeferred->UniformTexture(HS("uColor"), m_fbPBR->GetColorAttachment("color"));
			m_pDeferred->UniformTexture(HS("uEmission"), m_fBlur->GetResult());
			m_pDeferred->Uniform1f(HS("uExposure"), {1.0f});
			assets.Get<VertexArray>(AssetName::ModClipSpaceQuad)->DrawArrays(GL_TRIANGLES);
		}

		if (m_DisplayTitle)
		{
			constexpr float height = s_VirtualHeight;
			const float width = height * windowSize.x / windowSize.y;
			r2.Begin(glm::ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f));
			r2.TextShadowOffset({0.025f, -0.025f});
			r2.Translate({width / 2.0f, height / 2.0f});
			DrawTitle(r2, time);
			r2.End();
		}
	}

	void SplashScene::OnDetach()
	{
	}

	void SplashScene::OnResize(const WindowResizedEvent &evt)
	{
		m_fbPBR->Resize(evt.Width, evt.Height);
	}

	void SplashScene::DrawEmeralds(const Time &time)
	{
		auto &assets = Assets::GetInstance();
		auto &emerald = assets.Get<Model>(AssetName::ModChaosEmerald)->GetMesh(0);

		static constexpr std::array<glm::vec4, 7> colors = {
			glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
			glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
			glm::vec4(0.0f, 1.0f, 1.0f, 1.0f),
			glm::vec4(0.7f, 0.7f, 0.7f, 1.0f),
			glm::vec4(1.0f, 0.0f, 1.0f, 1.0f),
			glm::vec4(0.5f, 0.0f, 1.0f, 1.0f)};

		m_View.LoadIdentity();
		m_View.Translate({0.0f, 0.0f, -5.0f});

		m_Model.LoadIdentity();

		glm::vec3 lightPos(0.0f, 0.0f, 1.0f);
		glm::vec3 cameraPos = glm::inverse(m_View.GetMatrix()) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		m_pPBR->Use();

		m_pPBR->UniformMatrix4f(HS("uProjection"), m_Projection.GetMatrix());
		m_pPBR->UniformMatrix4f(HS("uView"), m_View.GetMatrix());

		m_pPBR->Uniform3fv(HS("uCameraPos"), 1, glm::value_ptr(cameraPos));
		m_pPBR->Uniform3fv(HS("uLightPos"), 1, glm::value_ptr(lightPos));

		m_pPBR->Uniform1f(HS("uEmission"), { 2.0f });

		m_pPBR->UniformTexture(HS("uMap"), assets.Get<Texture2D>(AssetName::TexWhite));
		m_pPBR->UniformTexture(HS("uMetallic"), assets.Get<Texture2D>(AssetName::TexEmeraldMetallic));
		m_pPBR->UniformTexture(HS("uRoughness"), assets.Get<Texture2D>(AssetName::TexEmeraldRoughness));

		m_pPBR->UniformTexture(HS("uBRDFLut"), assets.Get<Texture2D>(AssetName::TexBRDFLut));
		m_pPBR->UniformTexture(HS("uEnvironment"), m_Sky->GetEnvironment());
		m_pPBR->UniformTexture(HS("uIrradiance"), m_Sky->GetIrradiance());
		m_pPBR->UniformTexture(HS("uReflections"), assets.Get<Texture2D>(AssetName::TexBlack));
		m_pPBR->UniformTexture(HS("uReflectionsEmission"), assets.Get<Texture2D>(AssetName::TexBlack));

		for (uint32_t i = 0; i < 7; i++)
		{
			float angle = 2.0f * glm::pi<float>() / 7.0f * i + time.Elapsed * glm::pi<float>();
			float radius = s_EmeraldStartingRadius * std::max(0.0f, s_SplashTime - time.Elapsed) / s_SplashTime;

			m_Model.Push();

			m_Model.Translate({std::cos(angle) * radius, std::sin(angle) * radius, 0.0f});
			m_Model.Rotate({0.0f, 1.0f, 0.0f}, angle + time.Elapsed * s_EmeraldAngularVelocity);
			m_Model.Rotate({1.0f, 0.0f, 0.0f}, angle + time.Elapsed * s_EmeraldAngularVelocity);

			m_pPBR->UniformMatrix4f(HS("uModel"), m_Model.GetMatrix());
			m_pPBR->Uniform4fv(HS("uColor"), 1, glm::value_ptr(colors[i]));

			emerald->DrawArrays(GL_TRIANGLES);

			m_Model.Pop();
		}
	}

	void SplashScene::DrawTitle(Renderer2D &r2, const Time &time)
	{
		auto& assets = Assets::GetInstance();
		auto font = assets.Get<Font>(AssetName::FontMain);
		auto texLogo = assets.Get<Texture2D>(AssetName::TexLogo);

		constexpr glm::vec4 s_ShadowColor = { 0.0f, 0.0f, 0.0f, 0.5f };

		const float aspect = (float)texLogo->GetWidth() / texLogo->GetHeight();
		const float alpha = std::abs(std::sin(time.Elapsed * 5.0f));
		
		r2.Pivot(EPivot::Center);

		r2.Push();
		r2.Texture(texLogo);
		r2.Scale(5.0f);
		r2.DrawQuad({ 0.0f, 0.0f }, { aspect, 1.0f });
		r2.Pop();

		r2.Push();
		r2.Translate({ 0.0f, -2.5f });
		r2.Color(glm::vec4(1.0f, 1.0f, 1.0f, alpha));
		r2.TextShadowColor(s_ShadowColor * alpha);
		r2.DrawStringShadow(font, "Press Start");
		r2.Pop();
	}

} // namespace bsf