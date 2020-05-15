#include "BsfPch.h"

#include "Assets.h"
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
			m_Assets[AssetName::ModRing] = WavefrontLoader().Load("assets/models/ring.obj")->CreateModel();
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