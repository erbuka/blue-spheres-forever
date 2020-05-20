#pragma once

#include "Common.h"
#include "EventEmitter.h"

#include <glm/glm.hpp>


namespace bsf
{
	class Stage;

	enum class EGameState : uint8_t
	{
		None,
		Starting,
		Playing,
		Emerald,
		GameOver,
		Finished
	};

	enum class EGameAction
	{
		JumpStart,
		JumpEnd,
		GoBackward,
		GoForward,
		RingCollected,
		Perfect,
		GameSpeedUp
	};

	struct GameActionEvent
	{
		EGameAction Action;
	};

	struct GameStateChangedEvent
	{
		EGameState Old, Current;
	};


	class GameLogic
	{
	public:

		enum class ERotate : int8_t 
		{
			None = 0,
			Left = 1,
			Right = -1
		};

		EventEmitter<GameActionEvent> GameAction;
		EventEmitter<GameStateChangedEvent> GameStateChanged;

		GameLogic(Stage& stage);

		void Advance(const Time& time);

		float GetHeight() const { return m_Height; }
		
		glm::vec2 GetPosition() const;
		glm::vec2 GetDeltaPosition();
		
		glm::ivec2 GetDirection() const { return m_Direction; }
		float GetRotationAngle() const { return m_RotationAngle; }

		float GetNormalizedVelocity() const;
		float GetMaxVelocity() const;
		float GetVelocity() const { return m_Velocity; }
		
		bool IsRotating() const { return m_IsRotating; }
		
		bool IsGoindBackward() const { return m_IsGoingBackward; }
		
		bool IsJumping() const { return m_IsJumping; }
		float GetTotalJumpDistance() const { return m_TotalJumpDistance; }
		float GetRemainingJumpDistance() const { return m_RemainingJumpDistance; }

		uint32_t GetCurrentPace() const;
		uint32_t GetMinPace() const;
		uint32_t GetMaxPace() const;


		glm::vec2 WrapPosition(const glm::vec2& pos) const;

		void Rotate(ERotate r);
		void Jump();
		void RunForward();

	private:

		using StateFn = std::function<void(const Time&)>;

		Time m_SpeedUpTimer;

		void ChangeGameState(EGameState newState);

		float m_Velocity, m_VelocityScale;
		float m_AngularVelocity;

		glm::ivec2 m_Direction;
		glm::vec2 m_Position, m_DeltaPosition;
		glm::ivec2 m_LastCrossedPosition;
		float m_Height;
		float m_LastBounceDistance;
		float m_RemainingJumpDistance, m_TotalJumpDistance;
		float m_JumpHeight;

		int32_t m_CurrentPace;

		bool m_IsGoingBackward;
		bool m_IsJumping;
		bool m_IsRotating;

		ERotate m_RotateCommand;
		bool m_RunForwardCommand;
		bool m_JumpCommand;

		float m_RotationAngle, m_TargetRotationAngle;
		float m_GameOverRotationSpeed;

		EGameState m_State;
		Stage& m_Stage;

		bool PullRotateCommand();
		void DoRotation(const Time& time);

		bool CrossedEdge(const glm::vec2 pos0, const glm::vec2 pos1, bool& horizontal, bool& vertical);

	};
}


