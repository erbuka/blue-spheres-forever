#pragma once

#include "Application.h"
#include "Common.h"
#include "Scene.h"
#include "MatrixStack.h"
#include "EventEmitter.h"

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

	struct GameStateChangedEvent;
	struct GameActionEvent;

	struct BoxVertex {
		glm::vec3 Position, Uv;
		BoxVertex(const glm::vec3 v) : Position(v), Uv(v) {}
	};

	struct GameMessage
	{
		static constexpr float s_SlideDuration = 0.5f;
		static constexpr float s_MessageDuration = 2.0f;
		static constexpr float s_SlideInTime = s_SlideDuration;
		static constexpr float s_MessageTime = s_SlideDuration + s_MessageDuration;
		static constexpr float s_SlideOutTime = s_SlideDuration * 2.0f + s_MessageDuration;
		
		GameMessage(const std::string& message) : Message(message), Time(0.0f) {}
		std::string Message;
		float Time ;
	};

	class GameScene : public Scene
	{
	public:
			
		GameScene(const Ref<Stage>& stage);
		
		void OnAttach() override;
		void OnRender(const Time& time) override;
		void OnDetach() override;

		void OnResize(const WindowResizedEvent& evt);

	private:

		std::vector<Unsubscribe> m_Subscriptions;
		MatrixStack m_Model, m_View, m_Projection;
		MatrixStack m_ShadowView, m_ShadowProjection, m_ShadowModel;

		Ref<Renderer2D> m_Renderer2D;
		Ref<Framebuffer> m_fbDeferred, m_fbShadow;

		Ref<VertexArray> m_vaWorld, m_vaSphere, m_vaQuad, m_vaSkyBox, m_vaDynSkyBox;
		Ref<ShaderProgram> m_pPBR, m_pMorphPBR, m_pSkyGradient, m_pDeferred, m_pSkyBox, m_pIrradiance, m_pShadow;
		
		Ref<TextureCube> m_txBaseSkyBox, m_txBaseIrradiance;
		Ref<CubeCamera> m_ccSkyBox, m_ccIrradiance;
		Ref<Texture2D> m_txGroundMap;
		Ref<GameLogic> m_GameLogic;
		
		Ref<Stage> m_Stage;

		std::vector<BoxVertex> m_vDynSkyBoxVertices;

		std::list<GameMessage> m_GameMessages;


		void RenderShadowMap(const Time& time);

		void RenderGameUI(const Time& time);
		
		void OnGameStateChanged(const GameStateChangedEvent& evt);
		void OnGameAction(const GameActionEvent& action);

		Ref<TextureCube> CreateBaseSkyBox();
		Ref<TextureCube> CreateBaseIrradianceMap(const Ref<TextureCube>& source, uint32_t size);

		void RotateDynamicCubeMap(const glm::vec2& position, const glm::vec2& deltaPosition, const glm::vec2& windowSize);
		void GenerateDynamicCubeMap(Ref<CubeCamera>& camera, Ref<TextureCube> source);


	};
}

