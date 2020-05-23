#pragma once

#include "Common.h"
#include <unordered_map>

namespace bsf
{

	class Asset;

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

		ModRing,
		ModSonic,
		ModChaosEmerald,

		SfxBlueSphere,
		SfxYellowSphere,
		SfxGameOver,
		SfxEmerald,
		SfxBumper,
		SfxRing,
		SfxPerfect,
		SfxJump,
	};

	class Assets
	{
	public:
		static Assets& GetInstance();

		Assets(Assets&) = delete;
		Assets(Assets&&) = delete;
		Assets& operator=(Assets&) = delete;

		void Load();
		void Dispose();


		template<typename T>
		Ref<typename std::enable_if_t<std::is_base_of_v<Asset, T>, T>> Get(AssetName name)
		{

			auto r = m_Assets.find(name);
			if (r == m_Assets.end())
			{
				BSF_ERROR("Asset not found: {0}", name);
				return nullptr;
			}

			auto ptr = std::dynamic_pointer_cast<T>(r->second);

			if (ptr == nullptr)
			{

				BSF_ERROR("Invalid type for asset {0}", name);
				return nullptr;
			}

			return ptr;
		}

		~Assets();
	private:

		Assets() {}

		std::unordered_map<AssetName, Ref<Asset>> m_Assets;

	};
}

