#pragma once

#include "Scene.h"
#include "Stage.h"


namespace bsf
{

	class Renderer2D;
	class GameLogic;

	class GameSceneTest : public Scene
	{
	public:
		void OnAttach(Application& app) override;
		void OnRender(Application& app, const Time& time) override;
		void OnDetach(Application& app) override;
	private:
		Ref<Stage> m_Stage;
		Ref<GameLogic> m_GameLogic;
		Ref<Renderer2D> m_Renderer2D;
	};
}

