#pragma once

#include "Common.h"
#include "Application.h"
#include "Scene.h"
#include "MatrixStack.h"
#include "EventEmitter.h"
#include "Stage.h"

namespace bsf
{

	class ShaderProgram;
	class VertexArray;
	class Stage;
	class GameLogic;
	class Texture2D;
	class TextureCube;

	class GameScene : public Scene
	{
	public:
		void OnAttach(Application& app) override;
		void OnRender(Application& app, const Time& time) override;
		void OnDetach(Application& app) override;

		void OnResize(const WindowResizedEvent& evt);

	private:
		
		std::vector<Unsubscribe> m_Subscriptions;
		MatrixStack m_ModelViewMatrix;

		Ref<VertexArray> m_World, m_Sphere, m_SkyBox;
		Ref<ShaderProgram> m_Program, m_SkyBoxProgram;
		
		Ref<TextureCube> m_CubeMap;
		Ref<Texture2D> m_Map, m_White;
		Ref<GameLogic> m_GameLogic;
		
		Stage m_Stage;

	};
}

