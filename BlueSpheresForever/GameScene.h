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
	class Sky;

	struct GameStateChangedEvent;
	struct GameActionEvent;

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


		float m_GameOverObjectsHeight = 0.0f;

		std::list<Unsubscribe> m_Subscriptions;

		MatrixStack m_Model, m_View, m_Projection;
		MatrixStack m_ShadowView, m_ShadowProjection, m_ShadowModel;

		Ref<Framebuffer> m_fbDeferred, m_fbShadow, m_fbGroundReflections;

		Ref<ShaderProgram> m_pPBR, m_pMorphPBR, m_pSkyGradient, m_pDeferred, m_pSkyBox, m_pShadow;
		
		Ref<Sky> m_Sky;
		Ref<Texture2D> m_txGroundMap;
		Ref<GameLogic> m_GameLogic;
		
		Ref<Stage> m_Stage;

		std::list<GameMessage> m_GameMessages;


		void RenderShadowMap(const Time& time);

		void RenderGameUI(const Time& time);
		
		void OnGameStateChanged(const GameStateChangedEvent& evt);
		void OnGameAction(const GameActionEvent& action);

		void RotateSky(const glm::vec2& deltaPosition);

		void RenderEmerald(const Ref<ShaderProgram>& currentProgram, const Time& time, MatrixStack& model);

	};
}

