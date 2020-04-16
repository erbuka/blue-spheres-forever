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
	class Framebuffer;
	class Renderer2D;

	class GameScene : public Scene
	{
	public:
			
		GameScene(const Ref<Stage>& stage);

		void OnAttach(Application& app) override;
		void OnRender(Application& app, const Time& time) override;
		void OnDetach(Application& app) override;

		void OnResize(const WindowResizedEvent& evt);

	private:
		


		std::vector<Unsubscribe> m_Subscriptions;
		MatrixStack m_Model, m_View, m_Projection;

		Ref<Renderer2D> m_Renderer2D;

		Ref<Framebuffer> m_fbReflections;

		Ref<VertexArray> m_vaWorld, m_vaSphere, m_vaSky;
		Ref<ShaderProgram> m_pPBR, m_pSky;
		
		Ref<Texture2D> m_txGroundMap, m_txGroundNormalMap, m_txGroundMetallic, m_txGroundRoughness, m_txGroundAo;
		Ref<GameLogic> m_GameLogic;
		
		Ref<Stage> m_Stage;

	};
}

