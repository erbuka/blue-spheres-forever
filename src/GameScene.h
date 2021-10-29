#pragma once

#include "Application.h"
#include "Common.h"
#include "Scene.h"
#include "MatrixStack.h"
#include "EventEmitter.h"

namespace bsf
{
	
	class BlurFilter;
	class ShaderProgram;
	class Stage;
	class GameLogic;
	class Texture2D;
	class TextureCube;
	class Framebuffer;
	class Sky;
	class Bloom;

	struct GameStateChangedEvent;
	struct GameActionEvent;

	struct GameMessage
	{
		
		GameMessage(const std::string& message) : Message(message), Time(0.0f) {}
		std::string Message;
		float Time ;
	};

	struct RingSparkle
	{
		glm::vec2 Position = { 0.0f, 0.0f };
		glm::vec2 Size = { 1.0f, 1.0f };
		glm::vec2 Velocity = { 0.0f, 0.0f };
		float Time = 0.0f;
		bool Alive = true;
	};


	class RingSparkleEmitter
	{
	private:
		std::vector<RingSparkle> m_Sparkles;
	public:
		void Clear();
		void Emit(const uint32_t count);
		void Update(const Time& time);
		const auto Empty() const { return m_Sparkles.empty(); }
		auto begin() const { return m_Sparkles.cbegin(); }
		auto end() const { return m_Sparkles.cend(); }
	};

	class GameScene : public Scene
	{
	public:
			
		GameScene(const Ref<Stage>& stage, const GameInfo& gameInfo);
		
		void OnAttach() override;
		void OnRender(const Time& time) override;
		void OnDetach() override;

		void OnResize(const WindowResizedEvent& evt);

	private:
		bool m_Paused = false;

		float m_GameOverObjectsHeight = 0.0f;

		MatrixStack m_Model, m_View, m_Projection;
		MatrixStack m_ShadowProjection, m_ShadowModel;

		Ref<Framebuffer> m_fbPBR, m_fbGroundReflections;

		Ref<ShaderProgram> m_pReflections, m_pSkeletalReflections, m_pPBR, m_pSkeletalPBR, m_pSkyGradient, m_pDeferred, m_pSkyBox;
		Ref<Bloom> m_fxBloom;
		
		Ref<Sky> m_Sky;
		Ref<Texture2D> m_txGroundMap;
		Ref<GameLogic> m_GameLogic;

		RingSparkleEmitter m_RingSparkles;
		
		Ref<Stage> m_Stage;

		std::list<GameMessage> m_GameMessages;

		const GameInfo m_GameInfo;

		void RenderGameUI(const Time& time);
		
		void OnGameStateChanged(const GameStateChangedEvent& evt);
		void OnGameAction(const GameActionEvent& action);

		void RotateSky(const glm::vec2& deltaPosition);

		void RenderEmerald(const Ref<ShaderProgram>& currentProgram, const Time& time, MatrixStack& model);

		void RenderRingSparkles(glm::vec4 projectedOrigin, const Time& time);

	};
}

