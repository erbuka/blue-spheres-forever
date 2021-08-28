#include "BsfPch.h"

#include "Character.h"


namespace bsf
{

	void Character::PlayAnimation(const CharacterAnimation anim, bool loop, float timeWarp)
	{
		PlayAnimation(AnimationMap[anim], loop, timeWarp);
	}

	void Character::FadeToAnimation(const CharacterAnimation anim, float fadeTime, bool loop, float timeWarp)
	{
		FadeToAnimation(AnimationMap[anim], fadeTime, loop, timeWarp);
	}
}