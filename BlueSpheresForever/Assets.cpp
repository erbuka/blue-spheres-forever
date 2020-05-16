#include "BsfPch.h"

#include "Assets.h"
#include "Model.h"
#include "Texture.h"
#include "Log.h"
#include "Common.h"
#include "Font.h";
#include "WafefrontLoader.h"

namespace bsf
{

	Assets& Assets::GetInstance()
	{
		static Assets instance;
		return instance;
	}

	void Assets::Load()
	{

		// Fonts
		m_Assets[AssetName::FontMain] = Ref<Font>(new Font("assets/fonts/main.ttf"));

		// Textures
		m_Assets[AssetName::TexWhite] = Ref<Texture>(new Texture2D(0xffffffff));
		m_Assets[AssetName::TexBlack] = Ref<Texture>(new Texture2D(0xff000000));
		m_Assets[AssetName::TexNormalPosZ] = Ref<Texture>(new Texture2D(ToHexColor({ 0.5f, 0.5f, 1.0f })));

		m_Assets[AssetName::TexSphereMetallic] = CreateGray(0.1f);
		m_Assets[AssetName::TexSphereRoughness] = CreateGray(0.1f);

		m_Assets[AssetName::TexGroundNormal] = m_Assets[AssetName::TexNormalPosZ];
		m_Assets[AssetName::TexGroundMetallic] = CreateGray(0.5f);
		m_Assets[AssetName::TexGroundRoughness] = CreateGray(0.1f);

		m_Assets[AssetName::TexRingMetallic] = CreateGray(0.1f);
		m_Assets[AssetName::TexRingRoughness] = CreateGray(0.0f);

		{
			auto bumper = MakeRef<Texture2D>("assets/textures/star1.png");
			bumper->SetFilter(TextureFilter::LinearMipmapLinear, TextureFilter::Linear);
			m_Assets[AssetName::TexBumper] = bumper;
		}

		{
			auto star = MakeRef<Texture2D>("assets/textures/star.png");
			m_Assets[AssetName::TexStar] = star;
		}

		{
			auto brdf = MakeRef<Texture2D>("assets/textures/ibl_brdf_lut.png");
			brdf->SetFilter(TextureFilter::LinearMipmapLinear, TextureFilter::Linear);
			m_Assets[AssetName::TexBRDFLut] = brdf;
		}

		{
			m_Assets[AssetName::ModRing] = CreateModel(WavefrontLoader().Load("assets/models/ring.obj"), GL_STATIC_DRAW);
		}

		
		{ // Load sonic models
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

			auto sonicModel = MakeRef<AnimatedModel>();
			std::array<Ref<Model>, files.size()> models;

			for (uint32_t i = 0; i < files.size(); i++) {
				auto model = CreateModel(WavefrontLoader().Load(files[i]));
				sonicModel->AddFrame(model);
			}

			m_Assets[AssetName::ModSonic] = sonicModel;

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