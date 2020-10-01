#include "BsfPch.h"

#include "GameScene.h"
#include "VertexArray.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "Framebuffer.h"
#include "GameLogic.h"
#include "Log.h"
#include "Assets.h"
#include "Renderer2D.h"
#include "CubeCamera.h"
#include "Stage.h"
#include "Font.h"
#include "Model.h"
#include "Audio.h"
#include "MenuScene.h"
#include "SkyGenerator.h"
#include "SplashScene.h"
#include "BlurFilter.h"
#include "StageClearScene.h"
#include "Profiler.h"
#include "GLTF.h"
#include "Character.h"

namespace bsf
{

	static constexpr float s_MsgSlideDuration = 0.5f;
	static constexpr float s_MsgDuration = 2.0f;
	static constexpr float s_MsgSlideInTime = s_MsgSlideDuration;
	static constexpr float s_MsgTime = s_MsgSlideDuration + s_MsgDuration;
	static constexpr float s_MsgSlideOutTime = s_MsgSlideDuration * 2.0f + s_MsgDuration;

	GameScene::GameScene(const Ref<Stage>& stage, const GameInfo& gameInfo) :
		m_Stage(stage),
		m_GameInfo(gameInfo)
	{

	}

	void GameScene::OnAttach()
	{
		auto& app = GetApplication();
		auto& assets = Assets::GetInstance();

		auto windowSize = app.GetWindowSize();

		m_GameLogic = MakeRef<GameLogic>(*m_Stage);

		// Framebuffers
		m_fbPBR = MakeRef<Framebuffer>(windowSize.x, windowSize.y, true);
		m_fbPBR->CreateColorAttachment("color", GL_RGB16F, GL_RGB, GL_HALF_FLOAT);
		m_fbPBR->CreateColorAttachment("emission", GL_RGB16F, GL_RGB, GL_HALF_FLOAT);

		m_fbGroundReflections = MakeRef<Framebuffer>(windowSize.x, windowSize.y, true);
		m_fbGroundReflections->CreateColorAttachment("color", GL_RGB16F, GL_RGB, GL_HALF_FLOAT);
		m_fbGroundReflections->CreateColorAttachment("emission", GL_RGB16F, GL_RGB, GL_HALF_FLOAT);

		
		// Post processing
		m_fBloom = MakeRef<BlurFilter>(m_fbPBR->GetColorAttachment("emission"));

		// Programs
		m_pPBR = ShaderProgram::FromFile("assets/shaders/pbr.vert", "assets/shaders/pbr.frag");
		m_pSkeletalPBR = ShaderProgram::FromFile("assets/shaders/pbr.vert", "assets/shaders/pbr.frag", { "SKELETAL" });
		m_pDeferred = ShaderProgram::FromFile("assets/shaders/deferred.vert", "assets/shaders/deferred.frag");
		m_pSkyBox = ShaderProgram::FromFile("assets/shaders/skybox.vert", "assets/shaders/skybox.frag");
		
		// Textures
		m_txGroundMap = CreateCheckerBoard({ ToHexColor(m_Stage->PatternColors[0]), ToHexColor(m_Stage->PatternColors[1]) });
		m_txGroundMap->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);;

		// Skybox
		auto& skyGenerator = assets.Get<SkyGenerator>(AssetName::SkyGenerator);
		m_Sky = skyGenerator->Generate({ 1024, m_Stage->SkyColors[0], m_Stage->SkyColors[1] });

		// Event hanlders
		AddSubscription(app.WindowResized, this, &GameScene::OnResize);
		AddSubscription(m_GameLogic->GameStateChanged, this, &GameScene::OnGameStateChanged);
		AddSubscription(m_GameLogic->GameAction, this, &GameScene::OnGameAction);


		AddSubscription(app.KeyPressed, [&](const KeyPressedEvent& evt) {
			if (evt.KeyCode == GLFW_KEY_LEFT)
			{
				m_GameLogic->Rotate(GameLogic::ERotate::Left);
			}
			else if (evt.KeyCode == GLFW_KEY_RIGHT)
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
			else if (evt.KeyCode == GLFW_KEY_ENTER)
			{
				m_Paused = !m_Paused;
			}

		});

		auto fadeIn = MakeRef<FadeTask>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), 0.5f);
		ScheduleTask(ESceneTaskEvent::PostRender, fadeIn);

		// Play music
		auto music = assets.Get<Audio>(AssetName::SfxMusic);
		music->Play();

	}
	
	// TODO too many shared_ptr copies
	// TODO remove ambient occlusion textures
	void GameScene::OnRender(const Time& time)
	{
		BSF_DIAGNOSTIC_FUNC();

		auto windowSize = GetApplication().GetWindowSize();
		float aspect = windowSize.x / windowSize.y;
		auto& assets = Assets::GetInstance();

		auto modSphere = assets.Get<VertexArray>(AssetName::ModSphere);
		auto modRing = assets.Get<Model>(AssetName::ModRing);

		auto character = assets.Get<Character>(AssetName::ChrSonic);

		auto texWhite = assets.Get<Texture2D>(AssetName::TexWhite);
		auto texBlack = assets.Get<Texture2D>(AssetName::TexBlack);
		auto texSphereMetallic = assets.Get<Texture2D>(AssetName::TexSphereMetallic);
		auto texSphereRoughness = assets.Get<Texture2D>(AssetName::TexSphereRoughness);
		auto texRingMetallic = assets.Get<Texture2D>(AssetName::TexRingMetallic);
		auto texRingRoughness = assets.Get<Texture2D>(AssetName::TexRingRoughness);
		auto texBumper = assets.Get<Texture2D>(AssetName::TexBumper);
		auto texBumperMetallic = assets.Get<Texture2D>(AssetName::TexBumperMetallic);
		auto texBumperRoughness = assets.Get<Texture2D>(AssetName::TexBumperRoughness);


		if (!m_Paused)
		{
			const int steps = 5;
			for (int i = 0; i < steps; i++)
				m_GameLogic->Advance({ time.Delta / steps, time.Elapsed });

			character->Model.SetAnimationGlobalTimeWarp(m_GameLogic->GetNormalizedVelocity() * (m_GameLogic->IsGoindBackward() ? -1.0f : 1.0f));
			character->Model.Update(time);
		}

		glCullFace(GL_BACK);

		glm::vec2 pos = m_GameLogic->GetPosition();
		glm::vec2 deltaPos = m_GameLogic->GetDeltaPosition();
		glm::vec2 viewDir = m_GameLogic->GetViewDirection();
		glm::vec2 viewOrigin = -viewDir;
		int32_t ix = pos.x, iy = pos.y;
		float fx = pos.x - ix, fy = pos.y - iy;

		const auto setupView = [&]() {
			// Setup the player view
			m_View.Reset();
			m_Model.Reset();

			m_View.LoadIdentity();
			m_Model.LoadIdentity();

			m_View.LookAt({ -1.5f, 2.5f, 0.0f }, { 1.0f, 0.0, 0.0f }, { 0.0f, 1.0f, 0.0f });
			m_View.Rotate({ 0.0f, 1.0f, 0.0f }, -m_GameLogic->GetRotationAngle());
			m_Model.Rotate({ 1.0f, 0.0f, 0.0f }, -glm::pi<float>() / 2.0f);
		};

		const auto isObjectVisible = [&](const glm::vec2& position) -> bool {
			return glm::dot(position - viewOrigin, viewDir) >= 0.0f;
		};



		m_Projection.Reset();
		m_Projection.Perspective(glm::pi<float>() / 4.0f, aspect, 0.1f, 30.0f);

		// Update skybox
		{
			m_Model.Reset();
			m_View.Reset();
			RotateSky(deltaPos);
		}

		// Render shadow map
		//RenderShadowMap(time);

		// Begin scene
		glViewport(0, 0, windowSize.x, windowSize.y);


		// Draw ground reflections
		m_fbGroundReflections->Bind();
		{

			GLEnableScope scope({ GL_DEPTH_TEST, GL_CULL_FACE });

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);

			setupView();

			glm::vec3 cameraPosition = glm::inverse(m_View.GetMatrix()) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			glm::vec3 cameraWorldPosition = glm::inverse(m_Model.GetMatrix()) * glm::vec4(cameraPosition, 1.0f);
			glm::vec3 lightVector = { 0.0f, -1.0f, 0.0f };

			// Draw player
			{
				m_Model.Push();

				m_Model.Scale({ 1.0f, 1.0f, -1.0f });

				if (m_GameLogic->IsJumping())
					m_Model.Translate({ 0.0f, 0.0f, m_GameLogic->GetHeight() });

				m_Model.Rotate({ 0.0f, 0.0f, 1.0f }, m_GameLogic->GetRotationAngle());
				m_Model.Multiply(character->Matrix);

				m_pSkeletalPBR->Use();

				m_pSkeletalPBR->UniformMatrix4f("uProjection", m_Projection.GetMatrix());
				m_pSkeletalPBR->UniformMatrix4f("uView", m_View.GetMatrix());
				m_pSkeletalPBR->UniformMatrix4f("uModel", m_Model.GetMatrix());

				m_pSkeletalPBR->Uniform3fv("uCameraPos", 1, glm::value_ptr(cameraPosition));
				m_pSkeletalPBR->Uniform3fv("uLightPos", 1, glm::value_ptr(lightVector));

				m_pSkeletalPBR->Uniform1f("uEmission", { 0.0f });

				m_pSkeletalPBR->UniformTexture("uMetallic", texBlack);
				m_pSkeletalPBR->UniformTexture("uRoughness", texWhite);
				m_pSkeletalPBR->UniformTexture("uAo", texWhite);

				m_pSkeletalPBR->UniformTexture("uBRDFLut", assets.Get<Texture2D>(AssetName::TexBRDFLut));
				m_pSkeletalPBR->UniformTexture("uEnvironment", m_Sky->GetEnvironment());
				m_pSkeletalPBR->UniformTexture("uIrradiance", m_Sky->GetIrradiance());
				m_pSkeletalPBR->UniformTexture("uReflections", texBlack);
				m_pSkeletalPBR->UniformTexture("uReflectionsEmission", texBlack);

				m_pSkeletalPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::White));
				m_pSkeletalPBR->Uniform2f("uUvOffset", { 0, 0 });

				GLTFRenderConfig config;
				config.Program = m_pSkeletalPBR;
				character->Model.Render(time, config);

				m_Model.Pop();
			}
			
			// Draw Objects
			{
				m_pPBR->Use();

				m_pPBR->UniformMatrix4f("uProjection", m_Projection.GetMatrix());
				m_pPBR->UniformMatrix4f("uView", m_View.GetMatrix());
				m_pPBR->UniformMatrix4f("uModel", m_Model.GetMatrix());

				m_pPBR->Uniform3fv("uCameraPos", 1, glm::value_ptr(cameraPosition));
				m_pPBR->Uniform3fv("uLightPos", 1, glm::value_ptr(lightVector));

				m_pPBR->UniformTexture("uAo", assets.Get<Texture2D>(AssetName::TexWhite));

				m_pPBR->UniformTexture("uBRDFLut", assets.Get<Texture2D>(AssetName::TexBRDFLut));
				m_pPBR->UniformTexture("uEnvironment", m_Sky->GetEnvironment());
				m_pPBR->UniformTexture("uIrradiance", m_Sky->GetIrradiance());
				//m_pPBR->UniformTexture("uShadowMap", m_fbShadow->GetColorAttachment("depth"));
				m_pPBR->UniformTexture("uReflections", texBlack);
				m_pPBR->UniformTexture("uReflectionsEmission", texBlack);

				m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::White));
				m_pPBR->Uniform2f("uUvOffset", { 0.0f, 0.0f });

				// Stage objects
				for (int32_t x = -s_SightRadius; x <= s_SightRadius; x++)
				{
					for (int32_t y = -s_SightRadius; y <= s_SightRadius; y++)
					{
						auto value = m_Stage->GetValueAt(x + ix, y + iy);

						if (value == EStageObject::None || !isObjectVisible({ x - fx, y - fy }))
							continue;

						auto [visible, p, tbn] = Reflect(cameraWorldPosition, { x - fx, y - fy, 0.15f + m_GameOverObjectsHeight }, 0.15f);

						if (!visible)
							continue;

						m_Model.Push();
						m_Model.Translate(p);
						m_Model.Multiply(tbn);

						if (value == EStageObject::Ring)
							m_Model.Rotate({ 0.0f, 0.0f, -1.0f }, glm::pi<float>() * time.Elapsed);

						m_pPBR->Uniform1f("uEmission", { value == EStageObject::Ring ? 0.75f : 0.0f });

						m_pPBR->UniformMatrix4f("uModel", m_Model);
						switch (value)
						{
						case EStageObject::Ring:

							m_pPBR->UniformTexture("uMap", texWhite);
							m_pPBR->UniformTexture("uMetallic", texRingMetallic); // Sphere metallic
							m_pPBR->UniformTexture("uRoughness", texRingRoughness); // Sphere roughness
							m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::Ring));
							modRing->GetMesh(0)->DrawArrays(GL_TRIANGLES);
							break;
						case EStageObject::RedSphere:
							m_pPBR->UniformTexture("uMap", texWhite);
							m_pPBR->UniformTexture("uMetallic", texSphereMetallic);
							m_pPBR->UniformTexture("uRoughness", texSphereRoughness);
							m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::RedSphere));
							modSphere->DrawArrays(GL_TRIANGLES);
							break;
						case EStageObject::BlueSphere:
							m_pPBR->UniformTexture("uMap", texWhite);
							m_pPBR->UniformTexture("uMetallic", texSphereMetallic);
							m_pPBR->UniformTexture("uRoughness", texSphereRoughness);
							m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::BlueSphere));
							modSphere->DrawArrays(GL_TRIANGLES);
							break;
						case EStageObject::YellowSphere:
							m_pPBR->UniformTexture("uMap", texWhite);
							m_pPBR->UniformTexture("uMetallic", texSphereMetallic);
							m_pPBR->UniformTexture("uRoughness", texSphereRoughness);
							m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::YellowSphere));
							modSphere->DrawArrays(GL_TRIANGLES);
							break;
						case EStageObject::Bumper:
							m_pPBR->UniformTexture("uMap", texBumper);
							m_pPBR->UniformTexture("uMetallic", texBumperMetallic);
							m_pPBR->UniformTexture("uRoughness", texBumperRoughness);
							m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::White));
							modSphere->DrawArrays(GL_TRIANGLES);

							break;
						}

						m_Model.Pop();
					}
				}
			}

			// Emerald
			if (m_GameLogic->IsEmeraldVisible())
			{

				auto emeraldPos = glm::vec2(m_GameLogic->GetDirection()) * m_GameLogic->GetEmeraldDistance();
				auto [visible, pos, tbn] = Reflect(cameraWorldPosition, { emeraldPos.x, emeraldPos.y, 0.8f }, 0.15f);

				if (visible)
				{
					m_pPBR->UniformTexture("uMap", texWhite);
					m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexEmeraldMetallic));
					m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexEmeraldRoughness));
					m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(m_Stage->EmeraldColor));

					m_Model.Push();
					m_Model.Translate(pos);
					m_Model.Multiply(tbn);
					m_Model.Rotate({ 0.0f, 0.0f, 1.0f }, time.Elapsed * glm::pi<float>());
					m_pPBR->UniformMatrix4f("uModel", m_Model);
					assets.Get<Model>(AssetName::ModChaosEmerald)->GetMesh(0)->DrawArrays(GL_TRIANGLES);
					m_Model.Pop();
				}


			}

		}

		// Draw to deferred frame buffer
		m_fbPBR->Bind();
		{

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Draw Sky
			{
				GLEnableScope scope({ GL_DEPTH_TEST });

				glDisable(GL_DEPTH_TEST);

				m_View.Reset();
				m_View.LoadIdentity();
				m_View.LookAt({ 0.0f, 0.0f, 0.0f }, { 0.0f, -2.5f, -2.5f }, { 0.0f, 1.0f, 0.0f });
				m_View.Rotate({ 0.0f, 1.0f, 0.0f }, -m_GameLogic->GetRotationAngle() + glm::pi<float>() / 2.0f);
				
				m_Model.Reset();
				m_Model.LoadIdentity();
				
				glDepthMask(GL_FALSE);
				m_pSkyBox->Use();
				m_pSkyBox->UniformMatrix4f("uProjection", m_Projection);
				m_pSkyBox->UniformMatrix4f("uView", m_View);
				m_pSkyBox->UniformMatrix4f("uModel", m_Model);
				m_pSkyBox->UniformTexture("uSkyBox", m_Sky->GetEnvironment());
				assets.Get<VertexArray>(AssetName::ModSkyBox)->DrawArrays(GL_TRIANGLES);
				glDepthMask(GL_TRUE);

			}

			// Draw scene
			{
				GLEnableScope scope({ GL_DEPTH_TEST, GL_CULL_FACE });

				glEnable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);

				setupView();

				glm::vec3 cameraPosition = glm::inverse(m_View.GetMatrix()) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				glm::vec3 lightVector = { 0.0f, 1.0f, 0.0f };

				// Draw player
				{
					m_Model.Push();

					if (m_GameLogic->IsJumping())
						m_Model.Translate({ 0.0f, 0.0f, m_GameLogic->GetHeight() });

					m_Model.Rotate({ 0.0f, 0.0f, 1.0f }, m_GameLogic->GetRotationAngle());
					m_Model.Multiply(character->Matrix);

					m_pSkeletalPBR->Use();

					m_pSkeletalPBR->UniformMatrix4f("uProjection", m_Projection.GetMatrix());
					m_pSkeletalPBR->UniformMatrix4f("uView", m_View.GetMatrix());
					m_pSkeletalPBR->UniformMatrix4f("uModel", m_Model.GetMatrix());

					m_pSkeletalPBR->Uniform3fv("uCameraPos", 1, glm::value_ptr(cameraPosition));
					m_pSkeletalPBR->Uniform3fv("uLightPos", 1, glm::value_ptr(lightVector));

					m_pSkeletalPBR->Uniform1f("uEmission", { 0.0f });

					m_pSkeletalPBR->UniformTexture("uMetallic", texBlack);
					m_pSkeletalPBR->UniformTexture("uRoughness", texBlack);
					m_pSkeletalPBR->UniformTexture("uAo", texWhite);

					m_pSkeletalPBR->UniformTexture("uBRDFLut", assets.Get<Texture2D>(AssetName::TexBRDFLut));
					m_pSkeletalPBR->UniformTexture("uEnvironment", m_Sky->GetEnvironment());
					m_pSkeletalPBR->UniformTexture("uIrradiance", m_Sky->GetIrradiance());
					m_pSkeletalPBR->UniformTexture("uReflections", texBlack);
					m_pSkeletalPBR->UniformTexture("uReflectionsEmission", texBlack);

					m_pSkeletalPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::White));
					m_pSkeletalPBR->Uniform2f("uUvOffset", { 0, 0 });

					GLTFRenderConfig config;
					config.Program = m_pSkeletalPBR;
					character->Model.Render(time, config);

					m_Model.Pop();
				}

				// Draw ground
				
				m_pPBR->Use();
				
				m_pPBR->UniformMatrix4f("uProjection", m_Projection.GetMatrix());
				m_pPBR->UniformMatrix4f("uView", m_View.GetMatrix());
				m_pPBR->UniformMatrix4f("uModel", m_Model.GetMatrix());

				m_pPBR->Uniform3fv("uCameraPos", 1, glm::value_ptr(cameraPosition));
				m_pPBR->Uniform3fv("uLightPos", 1, glm::value_ptr(lightVector));

				m_pPBR->Uniform1f("uEmission", { 0.0f });

				m_pPBR->UniformTexture("uMap", m_txGroundMap);
				m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexGroundMetallic));
				m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexGroundRoughness));
				m_pPBR->UniformTexture("uAo", texWhite);

				m_pPBR->UniformTexture("uBRDFLut", assets.Get<Texture2D>(AssetName::TexBRDFLut));
				m_pPBR->UniformTexture("uEnvironment", m_Sky->GetEnvironment());
				m_pPBR->UniformTexture("uIrradiance", m_Sky->GetIrradiance());
				m_pPBR->UniformTexture("uReflections", m_fbGroundReflections->GetColorAttachment("color"));
				m_pPBR->UniformTexture("uReflectionsEmission", m_fbGroundReflections->GetColorAttachment("emission"));

				m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::White));
				m_pPBR->Uniform2f("uUvOffset", { (ix % 2) * 0.5f + fx * 0.5f, (iy % 2) * 0.5f + fy * 0.5f });

				assets.Get<VertexArray>(AssetName::ModGround)->DrawArrays(GL_TRIANGLES);

				// Draw spheres and rings
				m_pPBR->UniformTexture("uAo", texWhite);
				m_pPBR->UniformTexture("uReflections", texBlack);
				m_pPBR->UniformTexture("uReflectionsEmission", texBlack);
				m_pPBR->Uniform2f("uUvOffset", { 0.0f, 0.0f });


				for (int32_t x = -s_SightRadius; x <= s_SightRadius; x++)
					{
						for (int32_t y = -s_SightRadius; y <= s_SightRadius; y++)
						{
							auto value = m_Stage->GetValueAt(x + ix, y + iy);

							if (value == EStageObject::None || !isObjectVisible({ x - fx, y - fy }))
								continue;

							auto [p, tbn] = Project({ x - fx, y - fy, 0.15f + m_GameOverObjectsHeight });

							m_Model.Push();
							m_Model.Translate(p);
							m_Model.Multiply(tbn);

							if (value == EStageObject::Ring)
								m_Model.Rotate({ 0.0f, 0.0f, 1.0f }, glm::pi<float>() * time.Elapsed);

							m_pPBR->UniformMatrix4f("uModel", m_Model);

							m_pPBR->Uniform1f("uEmission", { value == EStageObject::Ring ? 0.75f : 0.0f });

							switch (value)
							{
							case EStageObject::Ring:

								m_pPBR->UniformTexture("uMap", texWhite);
								m_pPBR->UniformTexture("uMetallic", texRingMetallic);
								m_pPBR->UniformTexture("uRoughness", texRingRoughness);
								m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::Ring));
								modRing->GetMesh(0)->DrawArrays(GL_TRIANGLES);
								break;
							case EStageObject::RedSphere:
								m_pPBR->UniformTexture("uMap", texWhite);
								m_pPBR->UniformTexture("uMetallic", texSphereMetallic);
								m_pPBR->UniformTexture("uRoughness", texSphereRoughness);
								m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::RedSphere));
								modSphere->DrawArrays(GL_TRIANGLES);
								break;
							case EStageObject::BlueSphere:
								m_pPBR->UniformTexture("uMap", texWhite);
								m_pPBR->UniformTexture("uMetallic", texSphereMetallic);
								m_pPBR->UniformTexture("uRoughness", texSphereRoughness);
								m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::BlueSphere));
								modSphere->DrawArrays(GL_TRIANGLES);
								break;
							case EStageObject::YellowSphere:
								m_pPBR->UniformTexture("uMap", texWhite);
								m_pPBR->UniformTexture("uMetallic", texSphereMetallic);
								m_pPBR->UniformTexture("uRoughness", texSphereRoughness);
								m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::YellowSphere));
								modSphere->DrawArrays(GL_TRIANGLES);
								break;
							case EStageObject::Bumper:
								m_pPBR->UniformTexture("uMap", texBumper);
								m_pPBR->UniformTexture("uMetallic", texBumperMetallic);
								m_pPBR->UniformTexture("uRoughness", texBumperRoughness);
								m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(Colors::White));
								modSphere->DrawArrays(GL_TRIANGLES);

								break;
							}


							m_Model.Pop();
						}
					}

				// Draw Emerald if visible
				if(m_GameLogic->IsEmeraldVisible())
				{

					auto emeraldPos = glm::vec2(m_GameLogic->GetDirection()) * m_GameLogic->GetEmeraldDistance();
					auto [pos, tbn] = Project({ emeraldPos.x, emeraldPos.y, 0.8f });

					m_pPBR->Uniform1f("uEmission", { 0.75f });
					m_pPBR->UniformTexture("uMap", texWhite);
					m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexEmeraldMetallic));
					m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexEmeraldRoughness));
					m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(m_Stage->EmeraldColor));
					
					m_Model.Push();
					m_Model.Translate(pos);
					m_Model.Multiply(tbn);
					m_Model.Rotate({ 0.0f, 0.0f, 1.0f }, time.Elapsed * glm::pi<float>());
					m_pPBR->UniformMatrix4f("uModel", m_Model);
					assets.Get<Model>(AssetName::ModChaosEmerald)->GetMesh(0)->DrawArrays(GL_TRIANGLES);
					m_Model.Pop();


				}

			}
			
		}
		m_fbPBR->Unbind();


		// Apply post processing
		m_fBloom->Apply(2);
		// Draw to default frame buffer
		{
			GLEnableScope scope({ GL_FRAMEBUFFER_SRGB });

			glEnable(GL_FRAMEBUFFER_SRGB);
			glViewport(0, 0, windowSize.x, windowSize.y);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


			m_pDeferred->Use();
			m_pDeferred->UniformTexture("uColor", m_fbPBR->GetColorAttachment("color"));
			m_pDeferred->UniformTexture("uEmission", m_fbPBR->GetColorAttachment("emission"));
			m_pDeferred->Uniform1f("uExposure", { 1.0f });
			assets.Get<VertexArray>(AssetName::ModClipSpaceQuad)->DrawArrays(GL_TRIANGLES);

		}


	
		RenderGameUI(time);
		
	}
	
	void GameScene::OnDetach()
	{
	}

	void GameScene::OnResize(const WindowResizedEvent& evt)
	{
		glViewport(0, 0, evt.Width, evt.Height);
		m_fbPBR->Resize(evt.Width, evt.Height);
		m_fbGroundReflections->Resize(evt.Width, evt.Height);
	}

	void GameScene::RenderGameUI(const Time& time)
	{
		static constexpr float scale = 5.0f;

		auto windowSize = GetApplication().GetWindowSize();
		auto& renderer2d = GetApplication().GetRenderer2D();

		auto& assets = Assets::GetInstance();
		// In-game messages
		{
			float sw = scale * windowSize.x / windowSize.y;
			float sh = scale;

			renderer2d.Begin(glm::ortho(0.0f, sw, 0.0f, sh, -1.0f, 1.0f));
			renderer2d.Pivot({ 0.5f, 0.5f });


			for (auto it = m_GameMessages.begin(); it != m_GameMessages.end(); ++it)
			{
				it->Time += time.Delta;

				float x = 0.0f;

				if (it->Time < s_MsgSlideInTime)
					x = sw / 2.0f + sw * (1.0f - it->Time / s_MsgSlideDuration);
				else if (it->Time < s_MsgTime)
					x = sw / 2.0f;
				else
					x = sw / 2.0f - sw * (it->Time - s_MsgTime) / s_MsgSlideDuration;

				renderer2d.Push();
				renderer2d.Translate({ x, sh / 4.0f * 3.0f });

				renderer2d.TextShadowColor({ 0.0f, 0.0f, 0.0f, 1.0f });
				renderer2d.TextShadowOffset({ 0.02, -0.02 });
				renderer2d.Color({ 1.0f, 1.0f, 1.0f, 1.0f });

				renderer2d.DrawStringShadow(assets.Get<Font>(AssetName::FontMain), it->Message);

				renderer2d.Pop();
			}

			renderer2d.End();
			
			m_GameMessages.remove_if([](auto& x) { return x.Time >= s_MsgSlideOutTime; });

		}


		// Counters (spheres and rings)
		{
			constexpr float shadowOffset = 0.03f;
			constexpr float padding = 0.5f;
			constexpr float sw = 25.0f;
			float sh = sw / windowSize.x * windowSize.y;

			renderer2d.Begin(glm::ortho(0.0f, sw, 0.0f, sh, -1.0f, 1.0f));
			

			{
				auto blueSpheres = std::to_string(m_Stage->Count(EStageObject::BlueSphere));
				renderer2d.Push();
				renderer2d.Pivot(EPivot::TopLeft);
				renderer2d.Translate({ padding, sh - padding });

				renderer2d.Texture(assets.Get<Texture2D>(AssetName::TexUISphere));
				
				renderer2d.Color(Colors::Black);
				renderer2d.DrawQuad({ shadowOffset, -shadowOffset });

				renderer2d.Color(Colors::BlueSphere);
				renderer2d.DrawQuad({ 0.0f, 0.0f });
	
				renderer2d.Translate({ 1.0f + padding / 2.0f, 0.0f });

				renderer2d.Color({ 1.0f, 1.0f, 1.0f, 1.0f });
				renderer2d.TextShadowColor({ 0.0f, 0.0f, 0.0f, 1.0f });
				renderer2d.TextShadowOffset({ shadowOffset, -shadowOffset });
				renderer2d.DrawStringShadow(assets.Get<Font>(AssetName::FontMain), blueSpheres);
				
				renderer2d.Pop();
			}


			{
				auto rings = std::to_string(m_Stage->Rings);
				renderer2d.Push();
				renderer2d.Pivot(EPivot::TopRight);
				renderer2d.Translate({ sw - padding, sh - padding });

				renderer2d.Texture(assets.Get<Texture2D>(AssetName::TexUIRing));

				renderer2d.Color(Colors::Black);
				renderer2d.DrawQuad({ shadowOffset, -shadowOffset });

				renderer2d.Color(Colors::Ring);
				renderer2d.DrawQuad({ 0.0f, 0.0f });

				renderer2d.Translate({ -1.0f - padding / 2.0f, 0.0f });

				renderer2d.Color({ 1.0f, 1.0f, 1.0f, 1.0f });
				renderer2d.TextShadowColor({ 0.0f, 0.0f, 0.0f, 1.0f });
				renderer2d.TextShadowOffset({ shadowOffset, -shadowOffset });
				renderer2d.DrawStringShadow(assets.Get<Font>(AssetName::FontMain), rings);

				renderer2d.Pop();
			}


			renderer2d.End();
		}

		
	}

	void GameScene::OnGameStateChanged(const GameStateChangedEvent& evt)
	{
		auto& assets = Assets::GetInstance();
		auto character = assets.Get<Character>(AssetName::ChrSonic);

		if (evt.Current == EGameState::Starting)
		{
			character->Model.PlayAnimation("idle0");

			m_GameMessages.emplace_back("Get Blue Spheres!");
		}
		
		if (evt.Current == EGameState::Playing)
		{
			character->Model.FadeToAnimation("run", 0.5f, true, character->RunTimeWarp);
		}
		
		if (evt.Current == EGameState::GameOver)
		{
			auto task = MakeRef<FadeTask>(glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 2.0f);

			assets.Get<Audio>(AssetName::SfxGameOver)->Play();

			assets.Get<Audio>(AssetName::SfxMusic)->FadeOut(2.0f);

			character->Model.StopAllAnimations();

			task->SetDoneFunction([&](SceneTask& self) {

				if (m_Stage->Count(EStageObject::BlueSphere) == 0)
				{
					// Victory
					auto scene = MakeRef<StageClearScene>(m_GameInfo, m_Stage->GetCollectedRings(), m_Stage->IsPerfect());
					GetApplication().GotoScene(scene);
				}
				else
				{
					// Loss
					auto scene = MakeRef<SplashScene>();
					GetApplication().GotoScene(scene);
				}

			});

			ScheduleTask(ESceneTaskEvent::PostRender, task);
		}
		
		if (evt.Current == EGameState::Emerald)
		{

			assets.Get<Audio>(AssetName::SfxMusic)->SetVolume(0.5f);


			auto liftObjectsTask = MakeRef<SceneTask>();
			liftObjectsTask->SetUpdateFunction([&](SceneTask& self, const Time& time) {
				m_GameOverObjectsHeight += time.Delta * 10.0f;				
			});

			ScheduleTask(ESceneTaskEvent::PreRender, liftObjectsTask);

			auto playEmeraldSound = MakeRef<SceneTask>();

			playEmeraldSound->SetUpdateFunction([&, emeraldTime = 0.0f](SceneTask& self, const Time& time) mutable {

				if (emeraldTime > 2.0f)
				{
					assets.Get<Audio>(AssetName::SfxEmerald)->Play();
					self.SetDone();
				}

				emeraldTime += time.Delta;

			});

			assets.Get<Audio>(AssetName::SfxSplash)->Play();
			ScheduleTask(ESceneTaskEvent::PostRender, playEmeraldSound);
		}
	}

	void GameScene::OnGameAction(const GameActionEvent& evt)
	{
		auto& assets = Assets::GetInstance();
		auto character = assets.Get<Character>(AssetName::ChrSonic);

		switch (evt.Action)
		{
		case EGameAction::YellowSphereJumpStart:
			character->Model.PlayAnimation("ball", true, 0.5f);
			assets.Get<Audio>(AssetName::SfxYellowSphere)->Play();
			break;
		case EGameAction::NormalJumpStart:
			character->Model.PlayAnimation("ball", true, 0.5f);
			assets.Get<Audio>(AssetName::SfxJump)->Play();
			break;
		case EGameAction::JumpEnd:
			character->Model.PlayAnimation("run", true, character->RunTimeWarp);
			break;
		case EGameAction::GoForward:
			break;
		case EGameAction::GoBackward:
			break;
		case EGameAction::RingCollected:
			assets.Get<Audio>(AssetName::SfxRing)->Play();
			break;
		case EGameAction::Perfect:
			assets.Get<Audio>(AssetName::SfxPerfect)->Play();
			m_GameMessages.emplace_back("Perfect");
			break;
		case EGameAction::BlueSphereCollected:
			assets.Get<Audio>(AssetName::SfxBlueSphere)->Play();
			break;
		case EGameAction::HitBumper:
			assets.Get<Audio>(AssetName::SfxBumper)->Play();
			break;
		case EGameAction::GameSpeedUp:
			std::cout << m_GameLogic->GetVelocity() << std::endl;
			break;
		default:
			break;
		}

	}
	

	void GameScene::RotateSky(const glm::vec2& deltaPosition)
	{
		static constexpr float s_Scale = 32.0f;

		float du = deltaPosition.x / s_Scale;
		float dv = deltaPosition.y / s_Scale;
		
		glm::mat4 rotateZ = glm::rotate(glm::identity<glm::mat4>(), du * glm::pi<float>() * 2.0f, { 0.0f, 0.0f, 1.0f });
		glm::mat4 rotateX = glm::rotate(glm::identity<glm::mat4>(), dv * glm::pi<float>() * 2.0f, { 1.0f, 0.0f, 0.0f });
		glm::mat rotate = rotateZ * rotateX;

		m_Sky->ApplyMatrix(rotate);
	}

	
	void GameScene::RenderEmerald(const Ref<ShaderProgram>& currentProgram, const Time& time, MatrixStack& model)
	{
		auto emeraldPos = glm::vec2(m_GameLogic->GetDirection()) * m_GameLogic->GetEmeraldDistance();
		auto [pos, tbn] = Project({ emeraldPos.x, emeraldPos.y, 0.8f });

		model.Push();
		model.Translate(pos);
		model.Multiply(tbn);
		model.Rotate({ 0.0f, 0.0f, 1.0f }, time.Elapsed * glm::pi<float>());
		currentProgram->UniformMatrix4f("uModel", model);
		Assets::GetInstance().Get<Model>(AssetName::ModChaosEmerald)->GetMesh(0)->DrawArrays(GL_TRIANGLES);
		model.Pop();
	}
}