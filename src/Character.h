#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "Asset.h"
#include "Ref.h"
#include "GLTF.h"

namespace bsf
{

	enum class CharacterAnimation
	{
		Run, Ball, Idle
	};

	class Character : public GLTF, public Asset
	{
	public:

		std::unordered_map<CharacterAnimation, std::string_view> AnimationMap;
		float RunTimeWarp = 1.0f;
		glm::mat4 Matrix = glm::identity<glm::mat4>();

		using GLTF::PlayAnimation;
		using GLTF::FadeToAnimation;

		void PlayAnimation(const CharacterAnimation anim, bool loop = true, float timeWarp = 1.0f);
		void FadeToAnimation(const CharacterAnimation anim, float fadeTime, bool loop = true, float timeWarp = 1.0f);

	};
}