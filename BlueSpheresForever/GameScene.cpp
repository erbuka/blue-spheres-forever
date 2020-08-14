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
#include "CharacterAnimator.h"
#include "Audio.h"
#include "MenuScene.h"
#include "SkyGenerator.h"
#include "SplashScene.h"

#pragma region Shaders

bool paused = false;


static const std::string s_StarsVertex = R"Vertex(
	#version 330 core

	uniform mat4 uProjection;
	uniform mat4 uView;
	uniform mat4 uModel;

	uniform vec2 uOffset;

	const float PI = 3.141592;

	layout(location = 0) in vec2 aUv;
	layout(location = 1) in vec3 aColor;
	layout(location = 2) in float aSize;
	
	flat out vec4 gPosition;
	flat out vec3 gColor;
	flat out float gSize;
	
	vec3 dome(in vec2 uv) {
		uv = uv * 2.0 - 1.0;

		float x = uv.x;
		float y = uv.y;		
		//float z = -(x*x + y*y) + 1.0;			
		float z = sqrt(1.0 - x*x - y*y);
		return vec3(x, y, z - 0.9);
			
	}	

	void main() {
	
		vec2 coords = fract(aUv + uOffset);
		vec4 position = uProjection * uView * uModel * vec4(dome(coords) * 10.0, 1.0);
		
		gPosition = position;
		gSize = aSize;
		gColor = aColor;

		gl_Position =  position;
	}	

)Vertex";

static const std::string s_StarsGeometry = R"Geometry(
	#version 330
	
	uniform vec2 uUnitSize;

	layout(points) in;
	layout(triangle_strip, max_vertices = 4) out;	

	flat in vec4 gPosition[1];
	flat in vec3 gColor[1];
	flat in float gSize[1];		
	
	flat out vec3 fColor;
	out vec2 fUv;

	void main() {
		vec4 position = gPosition[0] / gPosition[0].w;
		float sx = uUnitSize.x * gSize[0] * 2.0f;
		float sy = uUnitSize.y * gSize[0] * 2.0f;
			
		fColor = gColor[0];

		gl_Position = position + vec4(+sx, -sy, position.z, position.w); fUv = vec2(1.0f, 0.0f); EmitVertex();
		gl_Position = position + vec4(+sx, +sy, position.z, position.w); fUv = vec2(1.0f, 1.0f); EmitVertex();
		gl_Position = position + vec4(-sx, -sy, position.z, position.w); fUv = vec2(0.0f, 0.0f); EmitVertex();
		gl_Position = position + vec4(-sx, +sy, position.z, position.w); fUv = vec2(0.0f, 1.0f); EmitVertex();
		
		
		EndPrimitive();
		
	}

)Geometry";



static const std::string s_StarsFragment = R"Fragment(
	#version 330 core
	
	uniform sampler2D uMap;

	flat in vec3 fColor;
	in vec2 fUv;
	
	out vec4 oColor;

	void main() {
		oColor = texture(uMap, fUv) * vec4(fColor, 1.0);
	}


)Fragment";

static const std::string s_ShadowVertex = R"Vertex(
	#version 330 core
	
	uniform mat4 uProjection;
	uniform mat4 uView;
	uniform mat4 uModel;
	
	layout(location = 0) in vec3 aPosition;
	
	out vec3 fPosition;
	
	void main() {
		vec3 position = (uView * uModel * vec4(aPosition, 1.0)).xyz;
		fPosition = position;
		gl_Position = uProjection * vec4(position, 1.0);
	}	
	
)Vertex";

static const std::string s_ShadowFragment = R"Fragment(
	#version 330 core

	in vec3 fPosition;

	layout(location = 0) out float oDepth;

	void main() {
		oDepth = fPosition.z;
	}

)Fragment";

static const std::string s_SkyGradientVertex = R"Vertex(
	#version 330 core

	uniform mat4 uProjection;
	uniform mat4 uView;
	uniform mat4 uModel;
	
	layout(location = 0) in vec3 aPosition;
	layout(location = 1) in vec3 aUv;

	out vec3 fPosition;
	out vec3 fUv;

	void main() {
		fPosition = aPosition;
		fUv = aUv;
		gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
	}
)Vertex";

static const std::string s_SkyGradientFragment = R"Vertex(
	#version 330 core

	uniform vec3 uColor0;
	uniform vec3 uColor1;
	uniform vec2 uOffset;

	in vec3 fPosition;
	in vec3 fUv;
	
	out vec4 oColor;

	void main() {
		oColor = vec4(mix(uColor1, uColor0, fUv.y * 0.5 + 0.5), 1.0);
	}

)Vertex";

static const std::string s_DeferredFragment = R"Fragment(
	#version 330 core

	uniform mat4 uProjection;
	uniform mat4 uProjectionInv;

	uniform sampler2D uColor;
	uniform sampler2D uPosition;

	in vec2 fUv;

	out vec4 oColor;	


	void main() {
		vec3 color = texture(uColor, fUv).rgb;
		color = color / (color + vec3(1.0));
		oColor = vec4(color, 1.0);
	}
	
)Fragment";


#pragma endregion

namespace bsf
{


	struct StarVertex
	{
		glm::vec2 UV;
		glm::vec3 Color;
		float Size;
	};




#pragma region Utilities



#pragma endregion



	GameScene::GameScene(const Ref<Stage>& stage) :
		m_Stage(stage)
	{
	}

	void GameScene::OnAttach()
	{
		auto& app = GetApplication();
		auto& assets = Assets::GetInstance();

		auto windowSize = app.GetWindowSize();

		m_GameLogic = MakeRef<GameLogic>(*m_Stage);

		// Framebuffers
		m_fbDeferred = MakeRef<Framebuffer>(windowSize.x, windowSize.y, true);
		m_fbDeferred->AddColorAttachment("color", GL_RGB16F, GL_RGB, GL_HALF_FLOAT);

		m_fbGroundReflections = MakeRef<Framebuffer>(windowSize.x, windowSize.y, true);
		m_fbGroundReflections->AddColorAttachment("color", GL_RGB16F, GL_RGB, GL_HALF_FLOAT);

		m_fbShadow = MakeRef<Framebuffer>(2048, 2048, true);
		m_fbShadow->AddColorAttachment("depth", GL_R16F, GL_RED, GL_FLOAT);
		m_fbShadow->GetColorAttachment("depth")->Bind(0);


		// Programs
		m_pPBR = ShaderProgram::FromFile("assets/shaders/pbr.vert", "assets/shaders/pbr.frag");
		m_pMorphPBR = ShaderProgram::FromFile("assets/shaders/pbr.vert", "assets/shaders/pbr.frag", { "MORPH" });
		m_pDeferred = ShaderProgram::FromFile("assets/shaders/deferred.vert", "assets/shaders/deferred.frag");
		m_pSkyBox = ShaderProgram::FromFile("assets/shaders/skybox.vert", "assets/shaders/skybox.frag");
		m_pSkyGradient = MakeRef<ShaderProgram>(s_SkyGradientVertex, s_SkyGradientFragment);
		m_pShadow = MakeRef<ShaderProgram>(s_ShadowVertex, s_ShadowFragment);
		
		// Textures
		if (m_Stage->FloorRenderingMode == EFloorRenderingMode::CheckerBoard)
		{
			m_txGroundMap = CreateCheckerBoard({ ToHexColor(m_Stage->CheckerColors[0]), ToHexColor(m_Stage->CheckerColors[1]) });
			m_txGroundMap->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);;
		}
		else
		{
			m_txGroundMap = Ref<Texture2D>(new Texture2D("assets/textures/metalgrid2_basecolor.png"));
			m_txGroundMap->SetFilter(TextureFilter::LinearMipmapLinear, TextureFilter::Linear);
			m_txGroundMap->SetAnisotropy(16.0f);
		} 


		// Skybox
		auto& skyGenerator = assets.Get<SkyGenerator>(AssetName::SkyGenerator);
		m_Sky = skyGenerator->Generate({ 1024, m_Stage->SkyColors[0], m_Stage->SkyColors[1] });

		// Event hanlders
		m_Subscriptions.push_back(app.WindowResized.Subscribe(this, &GameScene::OnResize));
		m_Subscriptions.push_back(m_GameLogic->GameStateChanged.Subscribe(this, &GameScene::OnGameStateChanged));
		m_Subscriptions.push_back(m_GameLogic->GameAction.Subscribe(this, &GameScene::OnGameAction));


		m_Subscriptions.push_back(app.KeyPressed.Subscribe([&](const KeyPressedEvent& evt) {
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
				paused = !paused;
			}
			else if (evt.KeyCode == GLFW_KEY_A)
			{
				m_GameMessages.emplace_back("Message Test");
			}

		}));

		auto fadeIn = MakeRef<FadeTask>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), 0.5f);
		ScheduleTask<FadeTask>(ESceneTaskEvent::PostRender, fadeIn);

	}
	
	void GameScene::OnRender(const Time& time)
	{
		auto windowSize = GetApplication().GetWindowSize();
		float aspect = windowSize.x / windowSize.y;
		auto& assets = Assets::GetInstance();

		glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 blue(0.0f, 0.0f, 1.0f, 1.0f);
		glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
		glm::vec4 yellow(1.0f, 1.0f, 0.0f, 1.0f);
		glm::vec4 gold(0.83f, 0.69f, 0.22f, 1.0f);

		auto modSphere = assets.Get<VertexArray>(AssetName::ModSphere);
		auto modRing = assets.Get<Model>(AssetName::ModRing);
		auto modSonic = assets.Get<CharacterAnimator>(AssetName::ModSonic);

		if (!paused)
		{
			const int steps = 5;
			for (int i = 0; i < steps; i++)
				m_GameLogic->Advance({ time.Delta / steps, time.Elapsed });
		}

		glCullFace(GL_BACK);

		glm::vec2 pos = m_GameLogic->GetPosition();
		glm::vec2 deltaPos = m_GameLogic->GetDeltaPosition();
		int32_t ix = pos.x, iy = pos.y;
		float fx = pos.x - ix, fy = pos.y - iy;

		auto setupView = [&]() {
			// Setup the player view
			m_View.Reset();
			m_Model.Reset();

			m_View.LoadIdentity();
			m_Model.LoadIdentity();

			m_View.LookAt({ -1.5f, 2.5f, 0.0f }, { 1.0f, 0.0, 0.0f }, { 0.0f, 1.0f, 0.0f });
			m_View.Rotate({ 0.0f, 1.0f, 0.0f }, -m_GameLogic->GetRotationAngle());
			m_Model.Rotate({ 1.0f, 0.0f, 0.0f }, -glm::pi<float>() / 2.0f);
		};

		m_Projection.Reset();
		m_Projection.Perspective(glm::pi<float>() / 4.0f, aspect, 0.1f, 30.0f);

		// Update skybox
		{
			m_Model.Reset();
			m_View.Reset();
			RotateSky(pos, deltaPos, windowSize);
		}

		// Render shadow map
		RenderShadowMap(time);

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

			m_pPBR->Use();


			m_pPBR->UniformMatrix4f("uProjection", m_Projection.GetMatrix());
			m_pPBR->UniformMatrix4f("uView", m_View.GetMatrix());
			m_pPBR->UniformMatrix4f("uModel", m_Model.GetMatrix());

			m_pMorphPBR->Uniform3fv("uCameraPos", 1, glm::value_ptr(cameraPosition));
			m_pMorphPBR->Uniform3fv("uLightPos", 1, glm::value_ptr(lightVector));


			m_pPBR->UniformTexture("uAo", assets.Get<Texture2D>(AssetName::TexWhite), 3);

			m_pPBR->UniformTexture("uBRDFLut", assets.Get<Texture2D>(AssetName::TexBRDFLut), 4);
			m_pPBR->UniformTexture("uEnvironment", m_Sky->GetEnvironment(), 5);
			m_pPBR->UniformTexture("uIrradiance", m_Sky->GetIrradiance(), 6);
			m_pPBR->UniformTexture("uShadowMap", m_fbShadow->GetColorAttachment("depth"), 7);
			m_pPBR->UniformTexture("uReflections", assets.Get<Texture2D>(AssetName::TexBlack), 8);

			m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(white));
			m_pPBR->Uniform2f("uUvOffset", { 0.0f, 0.0f });

			auto hd = [](const glm::vec3& obs) -> float {
				auto v = obs - s_GroundCenter;
				float h = glm::length(v) - s_GroundRadius;
				return glm::sqrt(h * (2.0f * s_GroundRadius + h));
			};

			auto hd2 = [](float h) -> float {
				return glm::sqrt(h * (2.0f * s_GroundRadius + h));
			};

			float horizon = hd(cameraWorldPosition);

			for (int32_t x = -12; x <= 12; x++)
			{
				for (int32_t y = -12; y <= 12; y++)
				{
					auto value = m_Stage->GetValueAt(x + ix, y + iy);

					if (value == EStageObject::None)
						continue;
					/*
					auto top = std::get<0>(Project({ x - fx, y - fy, 0.3f + m_GameOverObjectsHeight }));
					float maxTopDist = hd(top) + horizon;
					float topDist = glm::length(top - cameraWorldPosition);
					float factor = glm::clamp((topDist - horizon) / (maxTopDist - horizon), 0.0f, 1.0f);
					

					if (factor == 1.0f)
						continue;

					//auto [p, tbn] = Project({ x - fx, y - fy, 0.30f * factor - 0.15f - m_GameOverObjectsHeight });
					auto [p, tbn] = Project({ x - fx, y - fy, glm::lerp(-0.15f - m_GameOverObjectsHeight, 0.45f + m_GameOverObjectsHeight,  factor * factor) });
					*/

					auto [visible, p, tbn] = Reflect(cameraWorldPosition, { x - fx, y - fy, 0.15f + m_GameOverObjectsHeight }, 0.15f);

					if (!visible)
						continue;

					m_Model.Push();
					m_Model.Translate(p);
					m_Model.Multiply(tbn);
					
					if (value == EStageObject::Ring)
						m_Model.Rotate({ 0.0f, 0.0f, 1.0f }, glm::pi<float>() * time.Elapsed);

					m_pPBR->UniformMatrix4f("uModel", m_Model);

					switch (value)
					{
					case EStageObject::Ring:

						m_pPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexWhite), 0);
						m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexRingMetallic), 1); // Sphere metallic
						m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexRingRoughness), 2); // Sphere roughness
						m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(gold));
						modRing->GetMesh(0)->Draw(GL_TRIANGLES);
						break;
					case EStageObject::RedSphere:
						m_pPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexWhite), 0);
						m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexSphereMetallic), 1);
						m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexSphereRoughness), 2);
						m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(red));
						modSphere->Draw(GL_TRIANGLES);
						break;
					case EStageObject::BlueSphere:
						m_pPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexWhite), 0);
						m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexSphereMetallic), 1);
						m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexSphereRoughness), 2);
						m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(blue));
						modSphere->Draw(GL_TRIANGLES);
						break;
					case EStageObject::YellowSphere:
						m_pPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexWhite), 0);
						m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexSphereMetallic), 1); // Sphere metallic
						m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexSphereRoughness), 2); // Sphere roughness
						m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(yellow));
						modSphere->Draw(GL_TRIANGLES);
						break;
					case EStageObject::Bumper:
						m_pPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexBumper), 0);
						m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexBumperMetallic), 1); 
						m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexBumperRoughness), 2);
						m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(white));
						modSphere->Draw(GL_TRIANGLES);

						break;
					}

					m_Model.Pop();
				}
			}


		}

		// Draw to deferred frame buffer
		m_fbDeferred->Bind();
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
				m_pSkyBox->UniformTexture("uSkyBox", m_Sky->GetEnvironment(), 0);
				assets.Get<VertexArray>(AssetName::ModSkyBox)->Draw(GL_TRIANGLES);
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
				modSonic->SetTimeMultiplier(m_GameLogic->GetNormalizedVelocity());
				modSonic->Update(time);

				m_Model.Push();
				m_Model.Rotate({ 1.0f, 0.0f, 0.0f }, glm::pi<float>() / 2.0f);
				m_Model.Translate({ 0.0f, m_GameLogic->GetHeight() + (m_GameLogic->IsJumping() ? 0.3f : 0.0f), 0.0f });
				m_Model.Rotate({ 0.0f, 1.0f, 0.0f }, m_GameLogic->GetRotationAngle());
				if (m_GameLogic->IsJumping())
				{
					// 1 revolution per unit
					float angle = (m_GameLogic->GetTotalJumpDistance() - m_GameLogic->GetRemainingJumpDistance()) * glm::pi<float>() * 2.0f;

					// if we're going backward, we rotate in the opposite direction
					m_Model.Rotate({ 0.0f, 0.0f, 1.0f }, angle * (m_GameLogic->IsGoindBackward() ? 1.0f : -1.0f));
				}

				m_pMorphPBR->Use();

				m_pMorphPBR->UniformMatrix4f("uProjection", m_Projection.GetMatrix());
				m_pMorphPBR->UniformMatrix4f("uView", m_View.GetMatrix());
				m_pMorphPBR->UniformMatrix4f("uModel", m_Model.GetMatrix());

				m_pMorphPBR->Uniform3fv("uCameraPos", 1, glm::value_ptr(cameraPosition));
				m_pMorphPBR->Uniform3fv("uLightPos", 1, glm::value_ptr(lightVector));

				m_pMorphPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexWhite), 0);
				m_pMorphPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexBlack), 1);
				m_pMorphPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexWhite), 2);
				m_pMorphPBR->UniformTexture("uAo", assets.Get<Texture2D>(AssetName::TexWhite), 3);

				m_pMorphPBR->UniformTexture("uBRDFLut", assets.Get<Texture2D>(AssetName::TexBRDFLut), 4);
				m_pMorphPBR->UniformTexture("uEnvironment", m_Sky->GetEnvironment(), 5);
				m_pMorphPBR->UniformTexture("uIrradiance", m_Sky->GetIrradiance(), 6);
				m_pMorphPBR->UniformTexture("uShadowMap", m_fbShadow->GetColorAttachment("depth"), 7);
				m_pMorphPBR->UniformTexture("uReflections", assets.Get<Texture2D>(AssetName::TexBlack), 8);


				m_pMorphPBR->Uniform4fv("uColor", 1, glm::value_ptr(white));
				m_pMorphPBR->Uniform2f("uUvOffset", { 0, 0 });
				m_pMorphPBR->Uniform1f("uMorphDelta", { modSonic->GetDelta() });

				
				for (auto va : modSonic->GetModel()->GetMeshes())
					va->Draw(GL_TRIANGLES);

				m_Model.Pop();

				// Draw ground
				
				m_pPBR->Use();

				m_pPBR->UniformMatrix4f("uProjection", m_Projection.GetMatrix());
				m_pPBR->UniformMatrix4f("uView", m_View.GetMatrix());
				m_pPBR->UniformMatrix4f("uModel", m_Model.GetMatrix());

				m_pPBR->Uniform2fv("uResolution", 1, glm::value_ptr(windowSize));

				m_pPBR->UniformMatrix4f("uShadowProjection", m_ShadowProjection.GetMatrix());
				m_pPBR->UniformMatrix4f("uShadowView", m_ShadowView.GetMatrix());
				m_pPBR->Uniform2f("uShadowMapTexelSize", { 1.0f / m_fbShadow->GetWidth(), 1.0f / m_fbShadow->GetHeight() });

				m_pPBR->Uniform3fv("uCameraPos", 1, glm::value_ptr(cameraPosition));
				m_pPBR->Uniform3fv("uLightPos", 1, glm::value_ptr(lightVector));

				m_pPBR->UniformTexture("uMap", m_txGroundMap, 0);
				m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexGroundMetallic), 1);
				m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexGroundRoughness), 2);
				m_pPBR->UniformTexture("uAo", assets.Get<Texture2D>(AssetName::TexWhite), 3);

				m_pPBR->UniformTexture("uBRDFLut", assets.Get<Texture2D>(AssetName::TexBRDFLut), 4);
				m_pPBR->UniformTexture("uEnvironment", m_Sky->GetEnvironment(), 5);
				m_pPBR->UniformTexture("uIrradiance", m_Sky->GetIrradiance(), 6);
				m_pPBR->UniformTexture("uShadowMap", m_fbShadow->GetColorAttachment("depth"), 7);
				m_pPBR->UniformTexture("uReflections", m_fbGroundReflections->GetColorAttachment("color"), 8);

				m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(white));
				m_pPBR->Uniform2f("uUvOffset", { (ix % 2) * 0.5f + fx * 0.5f, (iy % 2) * 0.5f + fy * 0.5f });

				assets.Get<VertexArray>(AssetName::ModGround)->Draw(GL_TRIANGLES);

				// Draw spheres and rings
				m_pPBR->UniformTexture("uAo", assets.Get<Texture2D>(AssetName::TexWhite), 3);
				m_pPBR->UniformTexture("uReflections", assets.Get<Texture2D>(AssetName::TexBlack), 8);
				m_pPBR->Uniform2f("uUvOffset", { 0.0f, 0.0f });

				for (int32_t x = -12; x <= 12; x++)
				{
					for (int32_t y = -12; y <= 12; y++)
					{
						auto value = m_Stage->GetValueAt(x + ix, y + iy);
						
						if (value == EStageObject::None)
							continue;

						auto [p, tbn] = Project({ x - fx, y - fy, 0.15f + m_GameOverObjectsHeight });

						m_Model.Push();
						m_Model.Translate(p);
						m_Model.Multiply(tbn); 
					
						if (value == EStageObject::Ring)
							m_Model.Rotate({ 0.0f, 0.0f, 1.0f }, glm::pi<float>() * time.Elapsed);

						m_pPBR->UniformMatrix4f("uModel", m_Model);

						switch (value)
						{
						case EStageObject::Ring:

							m_pPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexWhite), 0);
							m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexRingMetallic), 1); // Sphere metallic
							m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexRingRoughness), 2); // Sphere roughness
							m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(gold));
							modRing->GetMesh(0)->Draw(GL_TRIANGLES);
							break;
						case EStageObject::RedSphere:
							m_pPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexWhite), 0);
							m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexSphereMetallic), 1);
							m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexSphereRoughness), 2);
							m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(red));
							modSphere->Draw(GL_TRIANGLES);
							break;
						case EStageObject::BlueSphere:
							m_pPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexWhite), 0);
							m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexSphereMetallic), 1);
							m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexSphereRoughness), 2);
							m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(blue));
							modSphere->Draw(GL_TRIANGLES);
							break;
						case EStageObject::YellowSphere:
							m_pPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexWhite), 0);
							m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexSphereMetallic), 1);
							m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexSphereRoughness), 2);
							m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(yellow));
							modSphere->Draw(GL_TRIANGLES);
							break;
						case EStageObject::Bumper:
							m_pPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexBumper), 0);
							m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexBumperMetallic), 1);
							m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexBumperRoughness), 2);
							m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(white));
							modSphere->Draw(GL_TRIANGLES);

							break;
						}

	
						m_Model.Pop();
					}
				}

				// Draw Emerald if visible
				if(m_GameLogic->IsEmeraldVisible()){

					m_pPBR->UniformTexture("uMap", assets.Get<Texture2D>(AssetName::TexWhite), 0);
					m_pPBR->UniformTexture("uMetallic", assets.Get<Texture2D>(AssetName::TexEmeraldMetallic), 1);
					m_pPBR->UniformTexture("uRoughness", assets.Get<Texture2D>(AssetName::TexEmeraldRoughness), 2);
					m_pPBR->Uniform4fv("uColor", 1, glm::value_ptr(m_Stage->EmeraldColor));
					RenderEmerald(m_pPBR, time, m_Model);

				}

			}
			
		}
		m_fbDeferred->Unbind();

		// Draw to default frame buffer
		{
			GLEnableScope scope({ GL_FRAMEBUFFER_SRGB });
			
			glEnable(GL_FRAMEBUFFER_SRGB);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			m_pDeferred->Use();
			//m_pDeferred->UniformTexture("uColor", m_fbDeferred->GetColorAttachment("color"), 0);
			m_pDeferred->UniformTexture("uColor", m_fbDeferred->GetColorAttachment("color"), 0);
			m_pDeferred->Uniform1f("uExposure", { 1.0f });
			assets.Get<VertexArray>(AssetName::ModClipSpaceQuad)->Draw(GL_TRIANGLES);


		}

	
		RenderGameUI(time);
		
	}
	
	void GameScene::OnDetach()
	{
		for (auto& unsubscribe : m_Subscriptions)
			unsubscribe();
	}

	void GameScene::OnResize(const WindowResizedEvent& evt)
	{
		glViewport(0, 0, evt.Width, evt.Height);
		m_fbDeferred->Resize(evt.Width, evt.Height);
		m_fbGroundReflections->Resize(evt.Width, evt.Height);
	}

	void GameScene::RenderShadowMap(const Time& time)
	{
		GLEnableScope scope({ GL_DEPTH_TEST });

		auto& assets = Assets::GetInstance();
		auto modSphere = assets.Get<VertexArray>(AssetName::ModSphere);
		auto modRing = assets.Get<Model>(AssetName::ModRing);

		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, m_fbShadow->GetWidth(), m_fbShadow->GetHeight());

		m_ShadowProjection.Reset();
		m_ShadowProjection.Orthographic(-6.0f, 6.0f, -6.0f, 6.0f, 0.0f, 100.0f);
		
		m_ShadowView.Reset();
		m_ShadowView.LookAt({ 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f });

		m_ShadowModel.Reset();
		m_ShadowModel.Rotate({ 1.0f, 0.0f, 0.0f }, -glm::pi<float>() / 2.0f);
	
		m_fbShadow->Bind();

		glClearColor(-100.0f, 0.0f, 0.0, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		m_pShadow->Use();

		m_pShadow->UniformMatrix4f("uProjection", m_ShadowProjection);
		m_pShadow->UniformMatrix4f("uView", m_ShadowView);

		glm::vec2 pos = m_GameLogic->GetPosition();
		int32_t ix = pos.x, iy = pos.y;
		float fx = pos.x - ix, fy = pos.y - iy;

		// Draw objects
		for (int32_t x = -6; x <= 6; x++)
		{
			for (int32_t y = -6; y <= 6; y++)
			{
				auto value = m_Stage->GetValueAt(x + ix, y + iy);

				if (value == EStageObject::None)
					continue;

				m_ShadowModel.Push();
				m_ShadowModel.Translate(std::get<0>(Project({ x - fx, y - fy, 0.15f + m_GameOverObjectsHeight })));

				if (value == EStageObject::Ring)
					m_ShadowModel.Rotate({ 0.0f, 0.0f, 1.0f }, glm::pi<float>() * time.Elapsed);

				m_pShadow->UniformMatrix4f("uModel", m_ShadowModel);

				switch (value)
				{
				case EStageObject::Ring:
					modRing->GetMesh(0)->Draw(GL_TRIANGLES);
					break;
				case EStageObject::RedSphere:
				case EStageObject::BlueSphere:
				case EStageObject::YellowSphere:
				case EStageObject::Bumper:
					modSphere->Draw(GL_TRIANGLES);
					break;
				}

				m_ShadowModel.Pop();
			}
		}

		// Draw emerald
		if (m_GameLogic->IsEmeraldVisible())
		{
			RenderEmerald(m_pShadow, time, m_ShadowModel);
		}


	}

	void GameScene::RenderGameUI(const Time& time)
	{
		auto windowSize = GetApplication().GetWindowSize();
		auto& renderer2d = GetApplication().GetRenderer2D();

		auto& assets = Assets::GetInstance();
		// In-game messages
		{
			float sw = 5.0f * windowSize.x / windowSize.y;
			float sh = 5.0f;

			renderer2d.Begin(glm::ortho(0.0f, sw, 0.0f, sh, -1.0f, 1.0f));
			renderer2d.Pivot({ 0.5f, 0.5f });

			for (auto it = m_GameMessages.begin(); it != m_GameMessages.end(); ++it)
			{
				it->Time += time.Delta;

				float x = 0.0f;

				if (it->Time < GameMessage::s_SlideInTime)
					x = sw / 2.0f + sw * (1.0f - it->Time / GameMessage::s_SlideDuration);
				else if (it->Time < GameMessage::s_MessageTime)
					x = sw / 2.0f;
				else
					x = sw / 2.0f - sw * (it->Time - GameMessage::s_MessageTime) / GameMessage::s_SlideDuration;

				renderer2d.Push();
				renderer2d.Translate({ x, sh / 4.0f * 3.0f });

				renderer2d.Color({ 0.0f, 0.0f, 0.0f, 1.0f });
				renderer2d.DrawString(assets.Get<Font>(AssetName::FontMain), it->Message);

				renderer2d.Translate({ -0.02, 0.02 });
				renderer2d.Color({ 1.0f, 1.0f, 1.0f, 1.0f });
				renderer2d.DrawString(assets.Get<Font>(AssetName::FontMain), it->Message);

				renderer2d.Pop();
			}

			renderer2d.End();
			
			m_GameMessages.remove_if([](auto& x) { return x.Time >= GameMessage::s_SlideOutTime; });

		}


		// Counters (spheres and rings)
		{
			constexpr float textOffset = 0.2f;
			constexpr float shadowOffset = 0.03f;
			constexpr float padding = 0.5f;
			constexpr float sw = 25.0f;
			float sh = sw / windowSize.x * windowSize.y;

			renderer2d.Begin(glm::ortho(0.0f, sw, 0.0f, sh, -1.0f, 1.0f));
			
			{
				auto blueSpheres = Format("%d", m_Stage->Count(EStageObject::BlueSphere));
				renderer2d.Push();
				renderer2d.Pivot(EPivot::TopLeft);
				renderer2d.Translate({ padding, sh - padding });

				renderer2d.Texture(assets.Get<Texture2D>(AssetName::TexSphereUI));
				
				renderer2d.Color({ 0.0f, 0.0f, 0.0f, 1.0f });
				renderer2d.DrawQuad({ shadowOffset, -shadowOffset });

				renderer2d.Color({ 0.0f, 0.0f, 1.0f, 1.0f });
				renderer2d.DrawQuad({ 0.0f, 0.0f });
	
				renderer2d.Translate({ 1.0f + padding / 2.0f, textOffset });

				renderer2d.Color({ 0.0f, 0.0f, 0.0f, 1.0f });
				renderer2d.DrawString(assets.Get<Font>(AssetName::FontMain), blueSpheres, { shadowOffset, -shadowOffset });
				
				renderer2d.Color({ 1.0f, 1.0f, 1.0f, 1.0f });
				renderer2d.DrawString(assets.Get<Font>(AssetName::FontMain), blueSpheres);
				
				renderer2d.Pop();
			}


			{
				auto rings = Format("%d", m_Stage->Rings);
				renderer2d.Push();
				renderer2d.Pivot(EPivot::TopRight);
				renderer2d.Translate({ sw - padding, sh - padding });

				renderer2d.Texture(assets.Get<Texture2D>(AssetName::TexRingUI));

				renderer2d.Color({ 0.0f, 0.0f, 0.0f, 1.0f });
				renderer2d.DrawQuad({ shadowOffset, -shadowOffset });

				renderer2d.Color({ 1.0f, 1.0f, 0.0f, 1.0f });
				renderer2d.DrawQuad({ 0.0f, 0.0f });

				renderer2d.Translate({ -1.0f - padding / 2.0f, textOffset });

				renderer2d.Color({ 0.0f, 0.0f, 0.0f, 1.0f });
				renderer2d.DrawString(assets.Get<Font>(AssetName::FontMain), rings, { shadowOffset, -shadowOffset });

				renderer2d.Color({ 1.0f, 1.0f, 1.0f, 1.0f });
				renderer2d.DrawString(assets.Get<Font>(AssetName::FontMain), rings);

				renderer2d.Pop();
			}



			renderer2d.End();
		}

	}

	void GameScene::OnGameStateChanged(const GameStateChangedEvent& evt)
	{
		auto& assets = Assets::GetInstance();
		auto& animator = assets.Get<CharacterAnimator>(AssetName::ModSonic);

		if (evt.Current == EGameState::Starting)
		{
			animator->SetRunning(true);
			animator->Play("stand", 1.0f);
			m_GameMessages.emplace_back("Get Blue Spheres!");
		}
		
		if (evt.Current == EGameState::Playing)
		{
			animator->Play("run", 0.5f);
		}
		
		if (evt.Current == EGameState::GameOver)
		{
			auto task = MakeRef<FadeTask>(glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 2.0f);

			assets.Get<Audio>(AssetName::SfxGameOver)->Play();

			animator->SetRunning(false);

			task->SetDoneFunction([&](SceneTask& self) {
				auto scene = MakeRef<SplashScene>();
				GetApplication().GotoScene(scene);
			});

			ScheduleTask<FadeTask>(ESceneTaskEvent::PostRender, task);
		}
		
		if (evt.Current == EGameState::Emerald)
		{

			auto liftObjectsTask = MakeRef<SceneTask>();
			liftObjectsTask->SetUpdateFunction([this](SceneTask& self, const Time& time) {
				m_GameOverObjectsHeight += time.Delta * 10.0f;				
			});

			ScheduleTask<SceneTask>(ESceneTaskEvent::PreRender, liftObjectsTask);

			auto playEmeraldSound = MakeRef<SceneTask>();

			playEmeraldSound->SetUpdateFunction([&, emeraldTime = MakeRef<float>()](SceneTask& self, const Time& time) {
				
				if ((*emeraldTime) > 2.0f)
				{
					assets.Get<Audio>(AssetName::SfxEmerald)->Play();
					self.SetDone();
				}

				(*emeraldTime) += time.Delta;

			});

			assets.Get<Audio>(AssetName::SfxSplash)->Play();
			ScheduleTask<SceneTask>(ESceneTaskEvent::PostRender, playEmeraldSound);
		}
	}

	void GameScene::OnGameAction(const GameActionEvent& evt)
	{
		auto& assets = Assets::GetInstance();
		auto animator = Assets::GetInstance().Get<CharacterAnimator>(AssetName::ModSonic);

		switch (evt.Action)
		{
		case EGameAction::YellowSphereJumpStart:
			animator->Play("jump", 0.5f);
			assets.Get<Audio>(AssetName::SfxYellowSphere)->Play();
			break;
		case EGameAction::NormalJumpStart:
			animator->Play("jump", 0.5f);
			assets.Get<Audio>(AssetName::SfxJump)->Play();
			break;
		case EGameAction::JumpEnd:
			animator->Play("run", 0.5f);
			break;
		case EGameAction::GoForward:
			animator->SetReverse(false);
			break;
		case EGameAction::GoBackward:
			animator->SetReverse(true);
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
		default:
			break;
		}

	}
	

	void GameScene::RotateSky(const glm::vec2& position, const glm::vec2& deltaPosition, const glm::vec2& windowSize)
	{
		float du = deltaPosition.x / m_Stage->GetWidth();
		float dv = deltaPosition.y / m_Stage->GetHeight();
		float u = 1.0f - position.x / m_Stage->GetWidth();
		float v = 1.0f - position.y / m_Stage->GetHeight();

		uint32_t ix = position.x;
		uint32_t iy = position.y;
		float fx = position.x - ix;
		float fy = position.y - iy;

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
		Assets::GetInstance().Get<Model>(AssetName::ModChaosEmerald)->GetMesh(0)->Draw(GL_TRIANGLES);
		model.Pop();
	}
}