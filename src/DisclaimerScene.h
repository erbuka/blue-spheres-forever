#pragma once

#include "Scene.h"

namespace bsf
{
	class DisclaimerScene : public Scene
	{
	public:
		void OnAttach() override;
		void OnRender(const Time& time) override;
	};
}

