#include "BsfPch.h"

#include "Assets.h"
#include "Texture.h"
#include "Log.h"
#include "Common.h"
#include "Font.h";
#include "WafefrontLoader.h"

namespace bsf
{
	template<typename T>
	static Ref<T>& GetAssetStatic(AssetName name, std::unordered_map<AssetName, Ref<T>>& map)
	{
		static Ref<T> nullVal = nullptr;

		auto r = map.find(name);
		if (r == map.end())
		{
			BSF_ERROR("Asset not found: {0}", name);
			return nullVal;
		}
		return r->second;
	}

	Assets& Assets::Get()
	{
		static Assets instance;
		return instance;
	}

	void Assets::Load()
	{

		// Fonts
		m_Fonts[AssetName::FontMain] = Ref<Font>(new Font("assets/fonts/main.ttf"));

		// Textures
		m_Textures[AssetName::TexWhite] = Ref<Texture>(new Texture2D(0xffffffff));
		m_Textures[AssetName::TexBlack] = Ref<Texture>(new Texture2D(0xff000000));
		m_Textures[AssetName::TexNormalPosZ] = Ref<Texture>(new Texture2D(ToHexColor({ 0.5f, 0.5f, 1.0f })));

		m_Textures[AssetName::TexSphereMetallic] = CreateGray(0.1f);
		m_Textures[AssetName::TexSphereRoughness] = CreateGray(0.1f);

		m_Textures[AssetName::TexGroundNormal] = m_Textures[AssetName::TexNormalPosZ];
		m_Textures[AssetName::TexGroundMetallic] = CreateGray(0.1f);
		m_Textures[AssetName::TexGroundRoughness] = CreateGray(0.1f);

		m_Textures[AssetName::TexRingMetallic] = CreateGray(0.1f);
		m_Textures[AssetName::TexRingRoughness] = CreateGray(0.0f);

		{
			auto bumper = MakeRef<Texture2D>("assets/textures/star1.png");
			bumper->SetFilter(TextureFilter::LinearMipmapLinear, TextureFilter::Linear);
			m_Textures[AssetName::TexBumper] = bumper;
		}



		{
			auto star = MakeRef<Texture2D>("assets/textures/star.png");
			m_Textures[AssetName::TexStar] = star;
		}

		{
			auto brdf = MakeRef<Texture2D>("assets/textures/ibl_brdf_lut.png");
			brdf->SetFilter(TextureFilter::LinearMipmapLinear, TextureFilter::Linear);
			m_Textures[AssetName::TexBRDFLut] = brdf;
		}

		{
			m_Models[AssetName::ModRing] = WavefrontLoader().Load("assets/models/ring.obj")->CreateModel();
		}

	}

	void Assets::Dispose()
	{
		m_Textures.clear();
		m_Fonts.clear();
	}

	const Ref<Texture>& Assets::GetTexture(AssetName n)
	{
		return GetAssetStatic(n, m_Textures);
	}

	const Ref<Font>& Assets::GetFont(AssetName n)
	{
		return GetAssetStatic(n, m_Fonts);
	}

	const Ref<Model>& Assets::GetModel(AssetName n)
	{
		return GetAssetStatic(n, m_Models);
	}

	Assets::~Assets()
	{
		Dispose();
	}
}