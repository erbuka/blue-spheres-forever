#pragma once

#include "Common.h"

#include <unordered_map>

namespace bsf
{

	class Texture;

	enum class AssetName
	{
		TexWhite,
		TexBlack,
		TexNormalPosZ,
		TexBumper,
		TexSphereMetallic,
		TexSphereRoughness,
		TexStar,
		TexBRDFLut,
	};


	class Assets
	{
	public:
		static Assets& Get();

		Assets(const Assets&) = delete;
		Assets& operator=(const Assets&) = delete;

		void Load();
		void Dispose();
			
		const Ref<Texture>& GetTexture(AssetName n);

		~Assets();
	private:

		Assets() {}

		std::unordered_map<AssetName, Ref<Texture>> m_Textures;
	};
}

