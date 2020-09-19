#pragma once

#include "Log.h"
#include "Ref.h"
#include "Profiler.h"
#include <unordered_map>

namespace bsf
{

	class Asset;

	enum class AssetName
	{
		TexWhite,
		TexBlack,
		TexTransparent,

		TexBumper,
		TexBumperMetallic,
		TexBumperRoughness,

		TexEmeraldMetallic,
		TexEmeraldRoughness,

		TexSphereMetallic,
		TexSphereRoughness,

		TexGroundMetallic,
		TexGroundRoughness,

		TexRingMetallic,
		TexRingRoughness,

		TexBRDFLut,

		TexUISphere,
		TexUIRing,
		TexUIAvoidSearch,
		TexUIPosition,
		TexUINew,
		TexUIOpen,
		TexUISave,
		TexUIDelete,
		TexUIBack,

		FontMain,
		FontText,

		ModRing,
		ModSonic,
		ModChaosEmerald,
		ModSphere,
		ModGround,
		ModClipSpaceQuad,
		ModSkyBox,

		ChrSonic,

		SfxBlueSphere,
		SfxYellowSphere,
		SfxGameOver,
		SfxEmerald,
		SfxBumper,
		SfxRing,
		SfxPerfect,
		SfxJump,
		SfxSplash,
		SfxIntro,
		SfxCodeOk,
		SfxCodeWrong,
		SfxTally,
		SfxStageClear,
		SfxMusic,

		StageGenerator,
		SkyGenerator
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
			return std::dynamic_pointer_cast<T>(m_Assets[name]);
		}

		~Assets();
	private:

		Assets() {}

		std::unordered_map<AssetName, Ref<Asset>> m_Assets;

	};
}

