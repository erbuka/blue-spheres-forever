#include "BsfPch.h"

#include "Assets.h"
#include "ShaderProgram.h"
#include "Model.h"
#include "VertexArray.h"
#include "Texture.h"
#include "Log.h"
#include "Common.h"
#include "Font.h";
#include "WafefrontLoader.h"
#include "Audio.h"
#include "Stage.h"
#include "SkyGenerator.h"
#include "Character.h"

namespace bsf
{


	Assets& Assets::GetInstance()
	{
		static Assets instance;
		return instance;
	}

	void Assets::Load()
	{
		const auto loadTex2D = [](std::string_view fileName) {
			auto result = MakeRef<Texture2D>(fileName);
			result->SetFilter(TextureFilter::LinearMipmapLinear, TextureFilter::Linear);
			return result;
		};


		// Stage generator
		m_Assets[AssetName::StageGenerator] = MakeRef<StageGenerator>();


		// Sky generator
		m_Assets[AssetName::SkyGenerator] = MakeRef<SkyGenerator>();

		// Fonts
		m_Assets[AssetName::FontMain] = MakeRef<Font>("assets/fonts/main.ttf", 128.0f);
		m_Assets[AssetName::FontText] = MakeRef<Font>("assets/fonts/arial.ttf", 72.0f);

		// Textures
		m_Assets[AssetName::TexLogo] = loadTex2D("assets/textures/bs.png");

		m_Assets[AssetName::TexWhite] = MakeRef<Texture2D>(0xffffffff);
		m_Assets[AssetName::TexBlack] = MakeRef<Texture2D>(0xff000000);
		m_Assets[AssetName::TexTransparent] = MakeRef<Texture2D>(0);

		m_Assets[AssetName::TexEmeraldMetallic] = CreateGray(0.1f);
		m_Assets[AssetName::TexEmeraldRoughness] = CreateGray(0.3f);

		m_Assets[AssetName::TexSphereMetallic] = CreateGray(0.1f);
		m_Assets[AssetName::TexSphereRoughness] = CreateGray(0.3f);
		
		m_Assets[AssetName::TexGroundMetallic] = CreateGray(0.5f);
		m_Assets[AssetName::TexGroundRoughness] = CreateGray(0.1f);

		m_Assets[AssetName::TexRingMetallic] = CreateGray(0.1f);
		m_Assets[AssetName::TexRingRoughness] = CreateGray(0.1f);

		m_Assets[AssetName::TexBumperMetallic] = CreateGray(0.1f);
		m_Assets[AssetName::TexBumperRoughness] = CreateGray(0.3f);
		m_Assets[AssetName::TexBumper] = loadTex2D("assets/textures/bumper.png");

		m_Assets[AssetName::TexBRDFLut] = loadTex2D("assets/textures/ibl_brdf_lut.png");
		
		m_Assets[AssetName::TexUISphere] = loadTex2D("assets/textures/sphere_ui.png");
		m_Assets[AssetName::TexUIRing] = loadTex2D("assets/textures/ring_ui.png");
		m_Assets[AssetName::TexUIAvoidSearch] = loadTex2D("assets/textures/avoid_search_ui.png");
		m_Assets[AssetName::TexUIPosition] = loadTex2D("assets/textures/position_ui.png");
		m_Assets[AssetName::TexUINew] = loadTex2D("assets/textures/new_ui.png");
		m_Assets[AssetName::TexUIOpen] = loadTex2D("assets/textures/open_ui.png");
		m_Assets[AssetName::TexUIDelete] = loadTex2D("assets/textures/delete_ui.png");
		m_Assets[AssetName::TexUISave] = loadTex2D("assets/textures/save_ui.png");
		m_Assets[AssetName::TexUIBack] = loadTex2D("assets/textures/back_ui.png");

		// Sound
		m_Assets[AssetName::SfxBlueSphere] = MakeRef<Audio>("assets/sound/bluesphere.wav");
		m_Assets[AssetName::SfxGameOver] = MakeRef<Audio>("assets/sound/redsphere.wav");
		m_Assets[AssetName::SfxBumper] = MakeRef<Audio>("assets/sound/bounce.wav");
		m_Assets[AssetName::SfxEmerald] = MakeRef<Audio>("assets/sound/emerald.wav");
		m_Assets[AssetName::SfxYellowSphere] = MakeRef<Audio>("assets/sound/tong.wav");
		m_Assets[AssetName::SfxRing] = MakeRef<Audio>("assets/sound/ring.wav");
		m_Assets[AssetName::SfxPerfect] = MakeRef<Audio>("assets/sound/perfect.mp3");
		m_Assets[AssetName::SfxJump] = MakeRef<Audio>("assets/sound/jump.wav");
		m_Assets[AssetName::SfxSplash] = MakeRef<Audio>("assets/sound/splash.wav");
		m_Assets[AssetName::SfxIntro] = MakeRef<Audio>("assets/sound/intro.mp3");
		m_Assets[AssetName::SfxCodeOk] = MakeRef<Audio>("assets/sound/ok.wav");
		m_Assets[AssetName::SfxCodeWrong] = MakeRef<Audio>("assets/sound/wrong.wav");
		m_Assets[AssetName::SfxTally] = MakeRef<Audio>("assets/sound/tally.mp3");
		m_Assets[AssetName::SfxStageClear] = MakeRef<Audio>("assets/sound/stage_clear.mp3");
		m_Assets[AssetName::SfxMenu] = MakeRef<Audio>("assets/sound/menu.wav");

		// Music
		{
			constexpr float loopPoint = 5.7f;
			auto music = MakeRef<Audio>("assets/sound/music_techno_loop.mp3");
			music->SetLoop(loopPoint);
			m_Assets[AssetName::SfxMusic] = music;
		}

		// Models
		
		m_Assets[AssetName::ModSphere] = CreateModel(WavefrontLoader().Load("assets/models/sphere.obj"))->GetMesh(0);
		m_Assets[AssetName::ModGround] = CreateGround(-10, 10, -10, 10, 10);
		m_Assets[AssetName::ModClipSpaceQuad] = CreateClipSpaceQuad();
		m_Assets[AssetName::ModSkyBox] = CreateCube();
		
		m_Assets[AssetName::ModRing] = 
			CreateModel(WavefrontLoader().Load("assets/models/ring.obj"), { 1.0f, 1.0f, 1.0f }, GL_STATIC_DRAW);

		m_Assets[AssetName::ModChaosEmerald] = 
			CreateModel(WavefrontLoader().Load("assets/models/chaos-emerald.obj"), { 1.0f, 1.0f, 1.0f }, GL_STATIC_DRAW);

		// Sonic character
		{
			auto sonic = MakeRef<Character>();

			sonic->RunTimeWarp = 2.0f;


			sonic->Matrix =
				glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) *
				glm::rotate(glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
				glm::scale(glm::vec3{ 0.5f, 0.5f, 0.5f });

			sonic->Load("assets/models/sonic.gltf", {
				GLTFAttributes::Position,
				GLTFAttributes::Normal,
				GLTFAttributes::Uv,
				GLTFAttributes::Joints_0,
				GLTFAttributes::Weights_0
				});

			sonic->AnimationMap = {
				{ CharacterAnimation::Idle, "idle0"},
				{ CharacterAnimation::Run, "run"},
				{ CharacterAnimation::Ball, "ball"},
			};

			m_Assets[AssetName::ChrSonic] = sonic;

		}
		

	}

	void Assets::Dispose()
	{
		m_Assets.clear();
	}

	Assets::~Assets()
	{
		Dispose();
	}
}