#include "Assets.h"
#include "Texture.h"
#include "Log.h"

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

		{
			auto bumper = MakeRef<Texture2D>("assets/textures/star1.png");
			bumper->Filter(TextureFilter::MinFilter, TextureFilterMode::LinearMipmapLinear);
			bumper->Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
			m_Textures[AssetName::TexBumper] = bumper;
		}

		{
			auto sphereMetallic = MakeRef<Texture2D>("assets/textures/sphere-metallic.png");
			sphereMetallic->Filter(TextureFilter::MinFilter, TextureFilterMode::LinearMipmapLinear);
			sphereMetallic->Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
			m_Textures[AssetName::TexSphereMetallic] = sphereMetallic;
		}

		{
			auto sphereRoughness = MakeRef<Texture2D>("assets/textures/sphere-roughness.png");
			sphereRoughness->Filter(TextureFilter::MinFilter, TextureFilterMode::LinearMipmapLinear);
			sphereRoughness->Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
			m_Textures[AssetName::TexSphereRoughness] = sphereRoughness;
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