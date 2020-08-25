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
#include "CharacterAnimator.h"
#include "Audio.h"
#include "Stage.h"
#include "SkyGenerator.h"

namespace bsf
{


	Assets& Assets::GetInstance()
	{
		static Assets instance;
		return instance;
	}

	void Assets::Load()
	{
		const auto loadTex2D = [](const std::string& fileName) {
			auto result = MakeRef<Texture2D>(fileName);
			result->SetFilter(TextureFilter::LinearMipmapLinear, TextureFilter::Linear);
			return result;
		};


		// Stage generator
		m_Assets[AssetName::StageGenerator] = MakeRef<StageGenerator>();


		// Sky generator
		m_Assets[AssetName::SkyGenerator] = MakeRef<SkyGenerator>();

		// Fonts
		m_Assets[AssetName::FontMain] = Ref<Font>(new Font("assets/fonts/main.ttf", 128.0f));
		m_Assets[AssetName::FontText] = Ref<Font>(new Font("assets/fonts/arial.ttf", 72.0f));


		// Textures
		m_Assets[AssetName::TexWhite] = Ref<Texture>(new Texture2D(0xffffffff));
		m_Assets[AssetName::TexBlack] = Ref<Texture>(new Texture2D(0xff000000));
		m_Assets[AssetName::TexTransparent] = Ref<Texture>(new Texture2D(0));
		m_Assets[AssetName::TexNormalPosZ] = Ref<Texture>(new Texture2D(ToHexColor({ 0.5f, 0.5f, 1.0f })));

		m_Assets[AssetName::TexEmeraldMetallic] = CreateGray(0.1f);
		m_Assets[AssetName::TexEmeraldRoughness] = CreateGray(0.1f);

		m_Assets[AssetName::TexSphereMetallic] = CreateGray(0.1f);
		m_Assets[AssetName::TexSphereRoughness] = CreateGray(0.1f);
		
		m_Assets[AssetName::TexGroundNormal] = m_Assets[AssetName::TexNormalPosZ];
		m_Assets[AssetName::TexGroundMetallic] = CreateGray(0.8f);
		m_Assets[AssetName::TexGroundRoughness] = CreateGray(0.1f);

		m_Assets[AssetName::TexRingMetallic] = CreateGray(0.9f);
		m_Assets[AssetName::TexRingRoughness] = CreateGray(0.1f);

		m_Assets[AssetName::TexBumperMetallic] = CreateGray(0.1f);
		m_Assets[AssetName::TexBumperRoughness] = CreateGray(0.1f);
		m_Assets[AssetName::TexBumper] = loadTex2D("assets/textures/bumper.png");

		m_Assets[AssetName::TexStar] = loadTex2D("assets/textures/star.png");
		m_Assets[AssetName::TexBRDFLut] = loadTex2D("assets/textures/ibl_brdf_lut.png");
		m_Assets[AssetName::TexUISphere] = loadTex2D("assets/textures/sphere_ui.png");
		m_Assets[AssetName::TexUIRing] = loadTex2D("assets/textures/ring_ui.png");
		m_Assets[AssetName::TexUIAvoidSearch] = loadTex2D("assets/textures/avoid_search_ui.png");


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
		m_Assets[AssetName::SfxStageClear] = MakeRef<Audio>("assets/sound/stage-clear.mp3");
		m_Assets[AssetName::SfxMusic] = MakeRef<Audio>("assets/sound/music_techno.mp3");

		// Models
		
		m_Assets[AssetName::ModSphere] = CreateModel(WavefrontLoader().Load("assets/models/sphere.obj"))->GetMesh(0);
		m_Assets[AssetName::ModGround] = CreateGround(-10, 10, -10, 10, 10);
		m_Assets[AssetName::ModClipSpaceQuad] = CreateClipSpaceQuad();
		m_Assets[AssetName::ModSkyBox] = CreateCube();
		
		m_Assets[AssetName::ModRing] = 
			CreateModel(WavefrontLoader().Load("assets/models/ring.obj"), { 1.0f, 1.0f, 1.0f }, GL_STATIC_DRAW);

		m_Assets[AssetName::ModChaosEmerald] = 
			CreateModel(WavefrontLoader().Load("assets/models/chaos-emerald.obj"), { 1.0f, 1.0f, 1.0f }, GL_STATIC_DRAW);
		
		{ 
			std::array<std::string, 14> files = {
				"assets/models/sonic0.obj",
				"assets/models/sonic1.obj",
				"assets/models/sonic2.obj",
				"assets/models/sonic3.obj",
				"assets/models/sonic4.obj",
				"assets/models/sonic5.obj",
				"assets/models/sonic6.obj",
				"assets/models/sonic7.obj",
				"assets/models/sonic8.obj",
				"assets/models/sonic9.obj",
				"assets/models/sonic10.obj",
				"assets/models/sonic11.obj",
				"assets/models/sonicStand.obj",
				"assets/models/sonicJump.obj",
			};

			auto sonicAnimator = MakeRef<CharacterAnimator>();
			std::array<Ref<Model>, files.size()> models;

			for (uint32_t i = 0; i < files.size(); i++) {
				auto model = CreateModel(WavefrontLoader().Load(files[i]), { 0.05f, 0.05f, 0.05f }, GL_STATIC_DRAW);
				sonicAnimator->AddFrame(model);
			}

			sonicAnimator->RegisterAnimation("run", { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0 });
			sonicAnimator->RegisterAnimation("stand", { 12, 12 });
			sonicAnimator->RegisterAnimation("jump", { 13, 13 });

			m_Assets[AssetName::ModSonic] = sonicAnimator;
			
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