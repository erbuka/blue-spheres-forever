#include "BsfPch.h"

#include "GameLogic.h"
#include "Stage.h"
#include "Log.h"

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
		glm::ivec2 StartingPoint = { 0, 0 };
		std::vector<glm::ivec2> CurrentPath;

		TransformRingState(Stage* stage, const glm::ivec2& startingPoint) :
			m_Stage(stage),
			StartingPoint(startingPoint)
		{
		}

		bool GoalTest() const
		{
			return CurrentPath.size() > 0 && CurrentPath.back() == StartingPoint;
		}

		std::vector<TransformRingState> GenerateChildren()
		{

			std::vector<TransformRingState> result;
			result.reserve(s_Directions.size());

			// First let's check if all the surrounding spheres are red
			// If that's the case we should discard all the children of this state
			// since they would not lead to a solution
			// This is very big improvement on some stages (like sonic3 stage 6)

			const glm::ivec2& position = CurrentPath.size() == 0 ? StartingPoint : CurrentPath.back();
			bool allRed = true;
			for (const auto& dir : s_AllDirections)
			{
				if (m_Stage->GetValueAt(position + dir) != EStageObject::RedSphere)
				{
					allRed = false;
					break;
				}
			}

			if (allRed)
				return result;


			// This is the root state. There's no previous
			// direction so we can go to any close red sphere
			if (CurrentDirection == glm::ivec2(0, 0))
			{

				for (const auto& dir : s_Directions)
				{
					auto pos = StartingPoint + dir;

					// We're only interested in red spheres
					if (m_Stage->GetValueAt(pos) != EStageObject::RedSphere)
						continue;

					TransformRingState newState(*this);
					newState.CurrentPath.push_back(pos);
					newState.CurrentDirection = dir;

					result.push_back(std::move(newState));


				}
			}
			else
			{
				// Not a root state

				const auto& forward = CurrentDirection;

				const std::array<glm::ivec2, 3> directions = {
					forward,
					glm::ivec2(-forward.y, forward.x),
					glm::ivec2(forward.y, -forward.x)
				};

				const std::array<glm::ivec2, 3> forbiddenTurn = {
					glm::ivec2(0, 0),
					glm::ivec2(-directions[1].y, directions[1].x),
					glm::ivec2(directions[2].y, -directions[2].x)
				};

				for (size_t i = 0; i < 3; i++)
				{
					// This turn is forbidden
					if (directions[i] == ForbiddenTurn)
						continue;

					const auto pos = CurrentPath.back() + directions[i];

					// Not a red sphere
					if (m_Stage->GetValueAt(pos) != EStageObject::RedSphere)
						continue;

					// Already in path
					if (std::find(CurrentPath.begin(), CurrentPath.end(), pos) != CurrentPath.end())
						continue;

					TransformRingState newState(*this);
					newState.CurrentPath.push_back(pos);
					newState.CurrentDirection = directions[i];
					newState.ForbiddenTurn = forbiddenTurn[i];
					result.push_back(std::move(newState));

				}

			}

			return result;
		}

		float Score() const
		{

			float h = CurrentPath.size();

			if (CurrentPath.size() > 0)
			{
				h += glm::distance(glm::vec2(StartingPoint), glm::vec2(CurrentPath.back()));
			}
			return h;
		}



	private:
		Stage* m_Stage;
	};


	bool operator<(const TransformRingState& a, const TransformRingState& b)
	{
		return a.Score() < b.Score();
	}

	
	bool operator==(const TransformRingState& a, const TransformRingState& b)
	{
		glm::ivec2 posA = a.CurrentPath.size() == 0 ? a.StartingPoint : a.CurrentPath.back();
		glm::ivec2 posB = b.CurrentPath.size() == 0 ? b.StartingPoint : b.CurrentPath.back();
		return posA == posB;
	}

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

			// Find a closed red spheres path with no sharp turns(no u turn or 2x2 turns)

			bool pathFound = false;
			std::vector<glm::ivec2> path;
			std::vector<TransformRingState> openSet, closedSet;
			
			openSet.emplace_back(&m_Stage, m_StartingPoint);
			
			while (!openSet.empty())
			{
				std::sort(openSet.begin(), openSet.end());

				auto current = openSet.front();
				openSet.erase(openSet.begin());

				closedSet.push_back(current);
				
				if (current.GoalTest()) // Done
				{
					path = std::move(current.CurrentPath);
					pathFound = true;
					break;
				}

				for (auto& state : current.GenerateChildren())
				{
					
					if (std::find(openSet.begin(), openSet.end(), state) == openSet.end())
					{
						openSet.push_back(std::move(state));
					}
					
				}
				
			}

			if (!pathFound)
				return;

			// First we get the bounding box of the path
			glm::ivec2 min = { std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max() };
			glm::ivec2 max = { std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min() };

			for (const auto& p : path)
			{
				min.x = std::min(min.x, p.x);
				min.y = std::min(min.y, p.y);

				max.x = std::max(max.x, p.x);
				max.y = std::max(max.y, p.y);

			}
			
			// This lamda will return true if the given position is inside the path with a
			// scanline approach
			// NB: The boundary is not considered inside.
			// TODO: maybe should be optimized (n^2 for now) -> could split the path
			// into scanlines before so we can access the desired line in constant time maybe
			auto isInsidePath = [&path, &min, &max](const glm::ivec2& pos) -> bool {

				bool inside = false;

				if (pos.x <= min.x || pos.x >= max.x || pos.y <= min.y || pos.y >= max.y)
					return false;


				for (int32_t x = min.x; x <= max.x; x++)
				{
					bool boundary = false;
					while (std::find(path.begin(), path.end(), glm::ivec2{ x, pos.y }) != path.end())
					{
						x++;
						boundary = true;
					}

					if(boundary)
						inside = !inside;

					if (pos.x == x)
					{
						return inside;
					}

				}

				return false;
			};


			// If and only if there's a connected blue sphere inside the path (even diagonal are considered connected
			// in this case), then convert into rings(floodfill)

			int32_t convertedBlueSpheres = 0;

			for (const auto& p : path)
			{
				for (const auto& dir : s_AllDirections)
				{
					auto pos = p + dir;

					if (isInsidePath(pos))
					{
						convertedBlueSpheres += FloodFillRings(pos);
					}

				}
			}


			if (convertedBlueSpheres > 0)
			{
				for (const auto& p : path)
				{
					m_Stage.SetValueAt(p, EStageObject::Ring);
				}
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
			{ EGameState::None, std::bind(&GameLogic::StateFnNone, this, std::placeholders::_1)},
			{ EGameState::Starting, std::bind(&GameLogic::StateFnStarting, this, std::placeholders::_1)},
			{ EGameState::Playing, std::bind(&GameLogic::StateFnPlaying, this, std::placeholders::_1)},
			{ EGameState::GameOver, std::bind(&GameLogic::StateFnGameOver, this, std::placeholders::_1)},
			{ EGameState::Emerald, std::bind(&GameLogic::StateFnEmerald, this, std::placeholders::_1)}
		};

	}


	glm::vec2 GameLogic::GetDeltaPosition()
	{
		auto result = m_DeltaPosition;
		m_DeltaPosition = { 0, 0 };
		return result;
	}

	float GameLogic::GetNormalizedVelocity() const { return m_Velocity * m_VelocityScale / s_BaseVelocity; }

	float GameLogic::GetMaxVelocity() const { return s_MaxVelocity; }

	glm::vec2 GameLogic::GetPosition() const
	{
		return WrapPosition(m_Position);
	}

	void GameLogic::Advance(const Time& time)
	{
		m_StateMap[m_State](time);

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
		if (m_IsGoingBackward)
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

					GameAction.Emit({ m_IsGoingBackward ? EGameAction::GoBackward : EGameAction::GoForward });

				}
				else if (object == EStageObject::YellowSphere)
				{
					m_TotalJumpDistance = s_YellowSphereDistance;
					m_RemainingJumpDistance = s_YellowSphereDistance;
					m_JumpHeight = s_YellowSphereHeight;
					m_JumpVelocityScale = 2.0f;
					m_IsJumping = true;
					GameAction.Emit({ EGameAction::JumpStart });
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
				GameAction.Emit({ EGameAction::JumpStart });
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
			pos.x += m_Stage.GetWidth();

		while (pos.y <= 0.0f)
			pos.y += m_Stage.GetHeight();

		if (pos.x > m_Stage.GetWidth())
			pos.x = std::fmodf(pos.x, m_Stage.GetWidth());

		if (pos.y > m_Stage.GetHeight())
			pos.y = std::fmodf(pos.y, m_Stage.GetHeight());

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
		if (glm::ivec2(glm::round(currentPos)) == m_LastCrossedPosition)
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
			m_LastCrossedPosition = glm::round(currentPos);
		}

		return crossed;
	}




}

