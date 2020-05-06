#include "Assets.h"
#include "Texture.h"
#include "Log.h"
#include "Common.h"

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
		m_Textures[AssetName::TexWhite] = Ref<Texture>(new Texture2D(0xffffffff));
		m_Textures[AssetName::TexBlack] = Ref<Texture>(new Texture2D(0xff000000));
		m_Textures[AssetName::TexNormalPosZ] = Ref<Texture>(new Texture2D(ToHexColor({ 0.5f, 0.5f, 1.0f })));

		m_Textures[AssetName::TexSphereMetallic] = CreateGray(0.95f);
		m_Textures[AssetName::TexSphereRoughness] = CreateGray(0.1f);


		{
			
			auto groundMetallic = MakeRef<Texture2D>("assets/textures/titanium-metal.png");
			groundMetallic->Filter(TextureFilter::MinFilter, TextureFilterMode::LinearMipmapLinear);
			groundMetallic->Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
			m_Textures[AssetName::TexGroundMetallic] = groundMetallic;
			
			m_Textures[AssetName::TexGroundMetallic] = CreateGray(0.95f);
		}

		{
			auto groundRoughness = MakeRef<Texture2D>("assets/textures/titanium-rough.png");
			groundRoughness->Filter(TextureFilter::MinFilter, TextureFilterMode::LinearMipmapLinear);
			groundRoughness->Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
			m_Textures[AssetName::TexGroundRoughness] = groundRoughness;
			

			m_Textures[AssetName::TexGroundRoughness] = CreateGray(0.2f);
		}

		{
			auto bumper = MakeRef<Texture2D>("assets/textures/star1.png");
			bumper->Filter(TextureFilter::MinFilter, TextureFilterMode::LinearMipmapLinear);
			bumper->Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
			m_Textures[AssetName::TexBumper] = bumper;
		}



		{
			auto star = MakeRef<Texture2D>("assets/textures/star.png");
			star->Filter(TextureFilter::MinFilter, TextureFilterMode::LinearMipmapLinear);
			star->Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
			m_Textures[AssetName::TexStar] = star;
		}

		{
			auto brdf = MakeRef<Texture2D>("assets/textures/ibl_brdf_lut.png");
			brdf->Filter(TextureFilter::MinFilter, TextureFilterMode::LinearMipmapLinear);
			brdf->Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
			m_Textures[AssetName::TexBRDFLut] = brdf;
		}



	}

	void Assets::Dispose()
	{
		m_Textures.clear();
	}

	const Ref<Texture>& Assets::GetTexture(AssetName n)
	{
		return GetAssetStatic(n, m_Textures);
	}

	Assets::~Assets()
	{
		Dispose();
	}
}