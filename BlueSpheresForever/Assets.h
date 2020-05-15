#pragma once

#include "Common.h"

#include <unordered_map>

namespace bsf
{

	class Texture;
	class Font;

	enum class AssetName
	{
		TexWhite,
		TexBlack,
		TexNormalPosZ,
		
		TexBumper,

		TexSphereMetallic,
		TexSphereRoughness,
		
		TexGroundNormal,
		TexGroundMetallic,
		TexGroundRoughness,

		TexRingMetallic,
		TexRingRoughness,

		TexStar,
		TexBRDFLut,

		FontMain,

		ModRing

	};

	class Assets
	{
	public:
		static Assets& Get();

		Assets(Assets&) = delete;
		Assets(Assets&&) = delete;
		Assets& operator=(Assets&) = delete;

		void Load();
		void Dispose();

		const Ref<Texture>& GetTexture(AssetName n);
		const Ref<Font>& GetFont(AssetName n);
		const Ref<Model>& GetModel(AssetName n);

		~Assets();
	private:

		Assets() {}

		std::unordered_map<AssetName, Ref<Texture>> m_Textures;
		std::unordered_map<AssetName, Ref<Font>> m_Fonts;
		std::unordered_map<AssetName, Ref<Model>> m_Models;
	};
}

