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
	class CubeCamera;

	class GameScene : public Scene
	{
	public:
			
		GameScene(const Ref<Stage>& stage);

		void OnAttach(Application& app) override;
		void OnRender(Application& app, const Time& time) override;
		void OnDetach(Application& app) override;

		void OnResize(const WindowResizedEvent& evt);

	private:

		void GenerateSkyBox(const glm::vec2& position, const glm::vec2& windowSize);

		std::vector<Unsubscribe> m_Subscriptions;
		MatrixStack m_Model, m_View, m_Projection;

		Ref<Renderer2D> m_Renderer2D;
		Ref<Framebuffer> m_fbDeferred;

		Ref<VertexArray> m_vaWorld, m_vaSphere, m_vaStars, m_vaQuad, m_vaSkyBox, m_vaSkyPlane;
		Ref<ShaderProgram> m_pPBR, m_pStars, m_pSkyGradient, m_pDeferred, m_pSkyBox, m_pSkyPlane;
		
		Ref<TextureCube> m_txEnv;
		Ref<CubeCamera> m_ccSkyBox;
		Ref<Texture2D> m_txGroundMap, m_txClouds;
		Ref<GameLogic> m_GameLogic;
		
		Ref<Stage> m_Stage;

	};
}

