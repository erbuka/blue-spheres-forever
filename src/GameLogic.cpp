#include "BsfPch.h"

#include "GameLogic.h"
#include "Stage.h"
#include "Log.h"
#include "Profiler.h"

namespace bsf
{

	// Game Pace
	static constexpr int32_t s_MinPace = 0;
	static constexpr int32_t s_MaxPace = 3;
	static constexpr float s_SpeedUpPeriod = 5.0f;

	// Game velocity defaults
	static constexpr float s_BaseVelocity = 3.5f;
	static constexpr float s_VelocityIncrease = 0.5f;
	static constexpr float s_MaxVelocity = s_BaseVelocity + s_MaxPace * s_VelocityIncrease;
	static constexpr float s_BaseAngularVelocity = glm::pi<float>() * 2.0f;
	static constexpr float s_AngularVelocityIncrease = glm::pi<float>() * 2.0f / 8.0f;
	static constexpr float s_EmeraldVelocityScale = 0.5f;

	// Jump
	static constexpr float s_JumpDistance = 2.0f;
	static constexpr float s_JumpHeight = 0.5f;

	// maximum sonic height (jump) for collision
	static constexpr float s_MaxCollisionHeight = 0.2f;


	// Yellow spheres
	static constexpr float s_YellowSphereDistance = 6.0f;
	static constexpr float s_YellowSphereHeight = .8f;

	// Game Over
	static constexpr float s_GameOverRotationAcceleration = 2.0f * glm::pi<float>();
	
	// Emerald
	static constexpr int32_t s_EmeraldDistanceHalf = 8;

	// Directions
	static constexpr glm::ivec2 s_dLeft = { -1, 0 };
	static constexpr glm::ivec2 s_dRight = { 1, 0 };
	static constexpr glm::ivec2 s_dTop = { 0, 1 };
	static constexpr glm::ivec2 s_dBottom = { 0, -1 };

	static constexpr glm::ivec2 s_dTopLeft = { -1, 1 };
	static constexpr glm::ivec2 s_dTopRight = { 1, 1 };
	static constexpr glm::ivec2 s_dBottomLeft = { -1, -1 };
	static constexpr glm::ivec2 s_dBottomRight = { 1, -1 };

	static constexpr std::array<glm::ivec2, 4> s_Directions = {
		s_dLeft, s_dRight, s_dTop, s_dBottom
	};


	static constexpr std::array<glm::ivec2, 8> s_AllDirections = {
		s_dLeft, s_dRight, s_dTop, s_dBottom,
		s_dTopLeft, s_dTopRight, s_dBottomLeft, s_dBottomRight
	};


	#pragma region Transform Rings Algorithm



	struct TransformRingState
	{
	public:

		glm::ivec2 ForbiddenTurn = { 0, 0 };
		glm::ivec2 CurrentDirection = { 0, 0 };
		std::vector<glm::ivec2> CurrentPath;

		TransformRingState(Stage* stage, const glm::ivec2& startingPoint) :
			m_Stage(stage),
			CurrentPath({ startingPoint })
		{
		}

		void GenerateChildren(std::vector<TransformRingState>& result)
		{
			static constexpr std::array<glm::ivec2, 4> s_NoForibiddenTurn = {
				glm::ivec2(0, 0),
				glm::ivec2(0, 0),
				glm::ivec2(0, 0),
				glm::ivec2(0, 0)
			};

			result.clear();

			const glm::ivec2& position = CurrentPath.back();
			
			// Check if this location is marked for avoid search. If that's the case, skip completely.
			if (m_Stage->GetAvoidSearchAt(position) == EAvoidSearch::Yes)
				return;
			
			// Let's check if all the surrounding spheres are red
			// If that's the case we should discard all the children of this state
			// since they would not lead to a solution
			// This is very big improvement on some stages (like sonic3 stage 6)
			if (!std::any_of(s_AllDirections.begin(), s_AllDirections.end(),
				[&](auto& dir) { return m_Stage->GetValueAt(position + dir) != EStageObject::RedSphere; }))
				return;

			const auto& forward = CurrentDirection;

			const std::array<glm::ivec2, 4> directions = {
				forward,
				glm::ivec2(-forward.y, forward.x),
				glm::ivec2(forward.y, -forward.x),
			};

			const std::array<glm::ivec2, 4> forbiddenTurn = {
				-forward,
				glm::ivec2(-directions[1].y, directions[1].x),
				glm::ivec2(directions[2].y, -directions[2].x),
			};

			const size_t dirCount = CurrentPath.size() > 1 ? directions.size() : s_Directions.size();
			const glm::ivec2* dirPtr = CurrentPath.size() > 1 ? directions.data() : s_Directions.data();
			const glm::ivec2* ftPtr = CurrentPath.size() > 1 ? forbiddenTurn.data() : s_NoForibiddenTurn.data();

			for (size_t i = 0; i < dirCount; i++)
			{
				// This turn is forbidden
				if (dirPtr[i] == ForbiddenTurn)
					continue;

				const auto pos = CurrentPath.back() + dirPtr[i];
				const auto wrappedPos = m_Stage->WrapCoordinates(pos);

				// Not a red sphere
				if (m_Stage->GetValueAt(pos) != EStageObject::RedSphere)
					continue;

				// Check if already in path, skipping the path first position which actually is allowed
				if (std::find_if(CurrentPath.begin() + 1, CurrentPath.end(), 
					[&](auto& v) { return m_Stage->WrapCoordinates(v) == wrappedPos; }) != CurrentPath.end())
					continue;


				TransformRingState newState(*this);
				newState.CurrentPath.push_back(pos);
				newState.CurrentDirection = dirPtr[i];
				newState.ForbiddenTurn = ftPtr[i];
				newState.ComputeScore();

				result.push_back(std::move(newState));

			}

		}

		void ComputeScore()
		{
			auto diff = glm::abs(CurrentPath.front() - CurrentPath.back());
			m_Score = CurrentPath.size() + diff.x + diff.y;
		}

		float Score() const { return m_Score; }

		bool operator==(const TransformRingState& other) const
		{
			return m_Stage->WrapCoordinates(CurrentPath.back()) == m_Stage->WrapCoordinates(other.CurrentPath.back());
		}

		bool operator<(const TransformRingState& other) const
		{
			return Score() < other.Score();
		}

		bool IsClosed() const
		{
			// Check if the path is closed and also valid (last turn)
			return CurrentPath.size() > 1 && CurrentPath.front() == CurrentPath.back() &&
				(CurrentPath[1] - CurrentPath[0] != ForbiddenTurn);
		}
		
		std::tuple<glm::ivec2, glm::ivec2> ComputeBounds() const
		{
			// Compute the bounding box of this path
			glm::ivec2 min = { std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max() };
			glm::ivec2 max = { std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min() };

			for (const auto& p : CurrentPath)
			{
				min.x = std::min(min.x, p.x);
				min.y = std::min(min.y, p.y);

				max.x = std::max(max.x, p.x);
				max.y = std::max(max.y, p.y);
			}

			return { min, max };
		}

		bool Contains(const std::tuple<glm::ivec2, glm::ivec2>& bounds, const glm::ivec2& pos) const
		{
			// Check if the given position is inside this path. The boundary is considered inside so be careful!
			const glm::ivec2& min = std::get<0>(bounds);
			const glm::ivec2& max = std::get<1>(bounds);

			bool inside = false;

			if (pos.x <= min.x || pos.x >= max.x || pos.y <= min.y || pos.y >= max.y)
				return false;

			for (int32_t x = min.x; x <= max.x; x++)
			{
				bool boundary = false;
				while (std::find(CurrentPath.begin(), CurrentPath.end(), glm::ivec2{ x, pos.y }) != CurrentPath.end())
				{
					x++;
					boundary = true;
				}

				if (boundary)
					inside = !inside;

				if (pos.x == x)
				{
					return inside;
				}

			}

			return false;
		}

	private:
		float m_Score = 0.0f;
		Stage* m_Stage;
	};

	

	class TransformRingAlgorithm
	{
	public:
		TransformRingAlgorithm(Stage& stage, const glm::ivec2& startingPoint) :
			m_Stage(stage),
			m_StartingPoint(startingPoint)
		{

		}

		void Calculate()
		{
			BSF_DIAGNOSTIC_FUNC();

			// First of all we check if there's a nearby blue sphere. If not, the algorithm
			// must not run
			if (std::none_of(s_AllDirections.begin(), s_AllDirections.end(),
				[&](const glm::ivec2& dir) { return m_Stage.GetValueAt(m_StartingPoint + dir) == EStageObject::BlueSphere; }))
			{
				return;
			}

			// Find a closed red spheres path with no sharp turns(no u turn or 2x2 turns)
			bool pathFound = false;
			glm::ivec2 floodFillPos;
			std::vector<glm::ivec2> path;
			std::vector<TransformRingState> openSet;
			std::vector<TransformRingState> children;

			openSet.emplace_back(&m_Stage, m_StartingPoint);
			
			while (!openSet.empty())
			{
				std::sort(openSet.begin(), openSet.end());

				auto current = openSet.front();
				openSet.erase(openSet.begin());

				if (current.IsClosed())
				{ // We are back at the starting location, so this is a possibile goal

					// Search for a blue sphere near the starting point that is also contained in the path
					const auto bounds = current.ComputeBounds();
					const auto floodFillDir = std::find_if(s_AllDirections.begin(), s_AllDirections.end(), [&](const glm::ivec2& dir) {
						const auto pos = m_StartingPoint + dir;
						return m_Stage.GetValueAt(pos) == EStageObject::BlueSphere && current.Contains(bounds, pos);
					});

					if (floodFillDir != s_AllDirections.end())
					{
						// If the search is successfull, this is a valid path and we have to convert into rings
						floodFillPos = *floodFillDir + m_StartingPoint;
						path = std::move(current.CurrentPath);
						pathFound = true;
						break;
					}
					else
					{
						// Otherwise this is a closed path with no blue sphere nearby the starting point,
						// so we don't convert to rings, and also we discard this path because it's closed
						// and so it's also self-intersecting
						continue;
					}



				}

				current.GenerateChildren(children);

				for (auto& state : children)
				{
					if (std::find(openSet.begin(), openSet.end(), state) == openSet.end())
						openSet.push_back(std::move(state));
					
				}
				
			}

			if (!pathFound)
				return;

			if (FloodFillRings(floodFillPos) > 0)
			{
				for (const auto& p : path)
					m_Stage.SetValueAt(p, EStageObject::Ring);
			}

		}

		int32_t FloodFillRings(const glm::ivec2& pos)
		{
			int32_t convertedBlueSpheres = 0;

			if (m_Stage.GetValueAt(pos) == EStageObject::BlueSphere)
			{
				m_Stage.SetValueAt(pos, EStageObject::Ring);
				
				convertedBlueSpheres += 1;

				for (const auto& dir : s_AllDirections)
				{
					convertedBlueSpheres += FloodFillRings(pos + dir);
				}
			}

			return convertedBlueSpheres;
		}


	private:
		glm::ivec2 m_StartingPoint;
		Stage& m_Stage;
	};


	#pragma endregion




	GameLogic::GameLogic(Stage& stage) :
		m_Stage(stage)
	{
		m_State = EGameState::Starting;
		
		m_RotateCommand = ERotate::None;
		m_JumpCommand = false;
		m_RunForwardCommand = false;

		m_State = EGameState::None;
		m_Position = stage.StartPoint;
		m_DeltaPosition = { 0, 0 };
		m_Direction = stage.StartDirection;

		m_Velocity = s_BaseVelocity;
		m_VelocityScale = 1.0f;
		m_JumpVelocityScale = 1.0f;
		m_AngularVelocity = s_BaseAngularVelocity;
		
		m_GameOverRotationSpeed = s_BaseAngularVelocity;

		m_IsEmeraldVisible = false;
		m_EmeraldDistance = 0.0f;

		m_IsRotating = false;
		m_IsJumping = false;
		m_IsGoingBackward = false;
		m_RotationAngle = m_TargetRotationAngle = std::atan2f(m_Direction.y, m_Direction.x);
		m_Height = 0.0f;
		m_LastBounceDistance = 1.0f;
		m_CurrentPace = s_MinPace;

		m_StateMap = {
			{ EGameState::None,		&GameLogic::StateFnNone		},
			{ EGameState::Starting, &GameLogic::StateFnStarting },
			{ EGameState::Playing,	&GameLogic::StateFnPlaying	},
			{ EGameState::GameOver, &GameLogic::StateFnGameOver	},
			{ EGameState::Emerald,	&GameLogic::StateFnEmerald	}
		};

	}


	glm::vec2 GameLogic::GetDeltaPosition()
	{
		auto result = m_DeltaPosition;
		m_DeltaPosition = { 0, 0 };
		return result;
	}

	glm::vec2 GameLogic::GetViewDirection() const
	{
		return { std::cos(m_RotationAngle), std::sin(m_RotationAngle) };
	}

	float GameLogic::GetNormalizedVelocity() const { return m_Velocity * m_VelocityScale / s_BaseVelocity; }

	float GameLogic::GetMaxVelocity() const { return s_MaxVelocity; }

	glm::vec2 GameLogic::GetPosition() const
	{
		return WrapPosition(m_Position);
	}

	void GameLogic::Advance(const Time& time)
	{
		(this->*m_StateMap[m_State])(time);
		// Wrap position inside boundary
		m_Position = WrapPosition(m_Position);
	}

	void GameLogic::Rotate(ERotate r)
	{
		m_RotateCommand = r;
	}

	void GameLogic::Jump()
	{
		if (!m_IsJumping)
		{
			m_JumpCommand = true;
		}
	}

	void GameLogic::RunForward()
	{
		if (m_IsGoingBackward && !m_IsJumping)
		{
			m_RunForwardCommand = true;
		}
	}

	void GameLogic::ChangeGameState(EGameState newState)
	{
		GameStateChanged.Emit({ m_State, newState });
		m_State = newState;
	}

	void GameLogic::StateFnNone(const Time& time)
	{
		ChangeGameState(EGameState::Starting);
	}

	void GameLogic::StateFnStarting(const Time& time)
	{
		// In the starting state, we can rotate in place
		// until the game starts
		PullRotateCommand();
		DoRotation(time);

		if (time.Elapsed >= 3.0f)
			ChangeGameState(EGameState::Playing);

	}

	void GameLogic::StateFnPlaying(const Time& time)
	{
		// Update the current rotation, if any
		DoRotation(time);

		// Update the game pace
		m_SpeedUpTimer += time;

		while (m_SpeedUpTimer.Elapsed >= s_SpeedUpPeriod && m_CurrentPace < s_MaxPace)
		{
			m_SpeedUpTimer -= s_SpeedUpPeriod;
			m_CurrentPace += 1;
			GameAction.Emit({ EGameAction::GameSpeedUp });
		}

		// Update the current velocity(ies) based on the game pace

		m_Velocity = s_BaseVelocity + m_CurrentPace * s_VelocityIncrease;
		m_AngularVelocity = s_BaseAngularVelocity + m_CurrentPace * s_AngularVelocityIncrease;


		// If not rotating, we move forward
		if (!m_IsRotating)
		{

			bool crossed, crossedHorizontal, crossedVertical;


			// Getting the next position position;
			glm::vec2 prevPos = m_Position;
			float step = CalculateStep(time);

			m_Position += glm::vec2(m_Direction) * step;
			m_DeltaPosition += glm::vec2(m_Direction) * step;
			glm::ivec2 roundedPosition = glm::round(m_Position);

			// Update last bounce distance
			m_LastBounceDistance = std::min(1.0f, m_LastBounceDistance + step);


			// Check if we crossed an edge
			crossed = CrossedEdge(prevPos, m_Position, crossedHorizontal, crossedVertical);


			if (m_Height <= s_MaxCollisionHeight && crossed)
			{
				auto object = m_Stage.GetValueAt(roundedPosition);

				if (object == EStageObject::BlueSphere)
				{
					m_Stage.SetValueAt(roundedPosition, EStageObject::RedSphere);

					// Fire event
					GameAction.Emit({ EGameAction::BlueSphereCollected });

					// Run the ring conversion algorithm
					TransformRingAlgorithm(m_Stage, roundedPosition).Calculate();

					if (m_Stage.GetValueAt(roundedPosition) == EStageObject::Ring)
					{
						m_Stage.CollectRing(roundedPosition);
						GameAction.Emit({ EGameAction::RingCollected });
						if (m_Stage.IsPerfect())
							GameAction.Emit({ EGameAction::Perfect });
					}

					// If there are no more blue spheres the game is over
					if (m_Stage.Count(EStageObject::BlueSphere) == 0)
					{
						// Reset the direction if going backward
						if (m_IsGoingBackward)
						{
							m_Direction *= -1.0f;
							m_IsGoingBackward = false;
						}

						// Set a low velocity like in the original game
						m_Velocity = s_BaseVelocity;
						m_VelocityScale = 0.5f;
						m_AngularVelocity = s_BaseAngularVelocity;

						// Spawn emerald
						m_IsEmeraldVisible = true;
						m_EmeraldDistance = s_EmeraldDistanceHalf * 2.0f;

						// Snap to current position for correct animations
						m_Position = roundedPosition;

						// Change game state
						ChangeGameState(EGameState::Emerald);
					}

				}
				else if (object == EStageObject::Bumper)
				{
					m_LastBounceDistance = 0.0f;
					m_IsGoingBackward = !m_IsGoingBackward;
					m_Direction *= -1;
					m_Position = roundedPosition; // Snap to star sphere
					GameAction.Emit({ EGameAction::HitBumper });
					GameAction.Emit({ m_IsGoingBackward ? EGameAction::GoBackward : EGameAction::GoForward });

				}
				else if (object == EStageObject::YellowSphere)
				{
					m_TotalJumpDistance = s_YellowSphereDistance;
					m_RemainingJumpDistance = s_YellowSphereDistance;
					m_JumpHeight = s_YellowSphereHeight;
					m_JumpVelocityScale = 2.0f;
					m_IsJumping = true;
					GameAction.Emit({ EGameAction::YellowSphereJumpStart });
				}
				else if (object == EStageObject::RedSphere)
				{
					ChangeGameState(EGameState::GameOver);
				}
				else if (object == EStageObject::Ring)
				{
					m_Stage.CollectRing(roundedPosition);
					GameAction.Emit({ EGameAction::RingCollected });
					if (m_Stage.IsPerfect())
						GameAction.Emit({ EGameAction::Perfect });
				}

			}

			// Check jump requests
			if (!m_IsJumping && m_JumpCommand)
			{
				m_TotalJumpDistance = s_JumpDistance;
				m_RemainingJumpDistance = s_JumpDistance;
				m_JumpHeight = s_JumpHeight;
				m_IsJumping = true;
				m_JumpCommand = false;
				m_JumpVelocityScale = 1.0f;
				GameAction.Emit({ EGameAction::NormalJumpStart });
			}

			HandleJump(step);

			// Check run forward requests
			if (m_IsGoingBackward && m_RunForwardCommand && m_LastBounceDistance == 1.0f)
			{
				m_RunForwardCommand = false;
				m_IsGoingBackward = false;
				m_Direction *= -1;
				GameAction.Emit({ EGameAction::GoForward });
			}

			// If we crossed and edge and there's a rotation request,
			// we want snap to the edge and rotate in place.
			
			// Must be on the ground to rotate, and can't rotate in the
			// same spot more than once
			
			// We also have to make sure that the state is still "Playing"
			// We don't want to pull rotate commands if it's "GameOver" or "Emerald"
			if (m_State == EGameState::Playing)
			{
				if (crossed && m_LastBounceDistance == 1.0f && !m_IsJumping && PullRotateCommand())
				{
					m_Position = glm::round(m_Position);
				}
			}

		}
	}

	void GameLogic::StateFnGameOver(const Time& time)
	{
		m_RotationAngle += time.Delta * m_GameOverRotationSpeed;
		m_GameOverRotationSpeed += s_GameOverRotationAcceleration * time.Delta;
	}

	void GameLogic::StateFnEmerald(const Time& time)
	{
		float step = CalculateStep(time);

		HandleJump(step);

		m_Position += glm::vec2(m_Direction) * step;
		m_DeltaPosition += glm::vec2(m_Direction) * step;
		m_EmeraldDistance = std::max(0.0f, m_EmeraldDistance - 2.0f * step);

		if (m_EmeraldDistance == 0.0f)
			ChangeGameState({ EGameState::GameOver });

	}

	void GameLogic::HandleJump(float step)
	{
		if (m_IsJumping)
		{

			m_RemainingJumpDistance -= step;
			float deltaJump = (m_TotalJumpDistance - m_RemainingJumpDistance) / m_TotalJumpDistance;
			m_Height = std::max(0.0f, (1.0f - std::pow(deltaJump * 2.0f - 1.0f, 2.0f)) * this->m_JumpHeight);


			if (m_RemainingJumpDistance <= 0.0f)
			{
				m_IsJumping = false;
				m_JumpVelocityScale = 1.0f;
				GameAction.Emit({ EGameAction::JumpEnd });
			}

		}
	}

	float GameLogic::CalculateStep(const Time& time) const
	{
		return m_Velocity * m_VelocityScale * m_JumpVelocityScale * time.Delta;
	}

	uint32_t GameLogic::GetCurrentPace() const { return m_CurrentPace; }
	uint32_t GameLogic::GetMinPace() const { return s_MinPace; }
	uint32_t GameLogic::GetMaxPace() const { return s_MaxPace; }

	glm::vec2 GameLogic::WrapPosition(const glm::vec2& p) const
	{
		glm::vec2 pos = p;

		while (pos.x < 0.0f)
			pos.x += m_Stage.GetSize();

		while (pos.y <= 0.0f)
			pos.y += m_Stage.GetSize();

		if (pos.x > m_Stage.GetSize())
			pos.x = std::fmodf(pos.x, m_Stage.GetSize());

		if (pos.y > m_Stage.GetSize())
			pos.y = std::fmodf(pos.y, m_Stage.GetSize());

		return pos;

	}

	bool GameLogic::PullRotateCommand()
	{
		if (!m_IsRotating && m_RotateCommand != ERotate::None)
		{
			m_IsRotating = true;
			m_TargetRotationAngle = m_RotationAngle + int32_t(m_RotateCommand) * glm::pi<float>() / 2.0f;

			if (m_RotateCommand == ERotate::Left)
			{
				m_Direction = { -m_Direction.y, m_Direction.x };
			}
			else
			{
				m_Direction = { m_Direction.y, -m_Direction.x };
			}

			m_RotateCommand = ERotate::None;

			return true;
		}

		return false;
	}

	void GameLogic::DoRotation(const Time& time)
	{
		if (m_IsRotating)
		{
			float dist = m_TargetRotationAngle - m_RotationAngle;
			float step = Sign(dist) * m_AngularVelocity * time.Delta;

			if (std::abs(dist) < std::abs(step))
			{
				m_RotationAngle = m_TargetRotationAngle;
				m_IsRotating = false;
			}
			else
			{
				m_RotationAngle += step;
			}
		}
	}

	bool GameLogic::CrossedEdge(const glm::vec2 currentPos, const glm::vec2 nextPosition, bool& horizontal, bool& vertical)
	{
		if (m_Stage.WrapCoordinates(glm::round(currentPos)) == m_LastCrossedPosition)
		{
			horizontal = false;
			vertical = false;
			return false;
		}


		int32_t 
			x0 = glm::floor(currentPos.x), 
			y0 = glm::floor(currentPos.y), 
			x1 = glm::floor(nextPosition.x), 
			y1 = glm::floor(nextPosition.y);

		horizontal = x1 - x0 != 0;
		vertical = y1 - y0 != 0;
		bool crossed = horizontal || vertical;

		if (crossed)
		{
			m_LastCrossedPosition = m_Stage.WrapCoordinates(glm::round(currentPos));
		}

		return crossed;
	}




}

