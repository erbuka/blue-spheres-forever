#pragma once

#include "Common.h"

#include <glm/glm.hpp>


namespace bsf
{
	class Stage;

	class GameLogic
	{
	public:

		enum class EState : uint8_t
		{
			Starting,
			Playing,
			Emerald,
			GameOver,
			Finished
		};

		enum class ERotate : int8_t 
		{
			None = 0,
			Left = 1,
			Right = -1
		};


		GameLogic(Stage& stage);

		float GetHeight() const { return m_Height; }
		glm::ivec2 GetDirection() const { return m_Direction; }
		glm::vec2 GetPosition() const;
		float GetRotationAngle() const { return m_RotationAngle; }
		float GetVelocity() const { return m_Velocity; }
		void Advance(const Time& time);
		bool IsRotating() const { return m_IsRotating; }

		glm::vec2 WrapPosition(const glm::vec2& pos) const;

		void Rotate(ERotate r);
		void Jump();
		void RunForward();

	private:


		float m_Velocity, m_VelocityScale;

		glm::ivec2 m_Direction;
		glm::vec2 m_Position;
		glm::ivec2 m_LastCrossedPosition;
		float m_Height;
		float m_LastBounceDistance;
		float m_RemainingJumpDistance, m_TotalJumpDistance;
		float m_JumpHeight;


		bool m_IsGoingBackward;
		bool m_IsJumping;
		bool m_IsRotating;

		ERotate m_RotateCommand;
		bool m_RunForwardCommand;
		bool m_JumpCommand;

		float m_RotationAngle, m_TargetRotationAngle;

		EState m_State;
		Stage& m_Stage;

		bool PullRotateCommand();
		void DoRotation(const Time& time);

		bool CrossedEdge(const glm::vec2 pos0, const glm::vec2 pos1, bool& horizontal, bool& vertical);

	};
}


