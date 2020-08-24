#pragma once

#include <unordered_map>
#include <list>
#include <algorithm>

#include "Ref.h"
#include "Time.h"
#include "Asset.h"

namespace bsf
{
	class Model;


	struct FrameTransition
	{
		uint32_t FromFrame, ToFrame;
		float Duration;
		float CurrentTime;

		FrameTransition(uint32_t fromFrame, uint32_t toFrame, float duration);

		float GetDelta() const { return std::min(1.0f, CurrentTime / Duration); }
		void Update(float dt) { CurrentTime += dt; }
		bool IsDone() const { return CurrentTime >= Duration; }
		float GetExcessTime() const { return std::max(CurrentTime - Duration, 0.0f); }
		void Reset();


	};
	
		
	class CharacterAnimator : public Asset
	{
	public:
		CharacterAnimator();
		void AddFrame(const Ref<Model>& frame);

		void Update(const Time& time);

		const Ref<Model>& GetModel() const { return m_Model; }
		float GetDelta() const { return m_TransitionQueue.empty() ? 0.0f : m_TransitionQueue.front().GetDelta(); }

		void RegisterAnimation(const std::string& name, const std::initializer_list<uint32_t>& frames);
		void Play(const std::string& animationName, float duration, bool loop = true);

		void SetTimeMultiplier(float timeMultipliter) { m_TimeMultiplier = timeMultipliter; }

		void SetReverse(bool reverse);

		void SetRunning(bool running) { m_Running = running; }

	private:

		void Reverse();
		void UpdateModel();
		void InitializeModel();

		bool m_Running;
		bool m_Reverse;
		float m_TimeMultiplier;
		Ref<Model> m_Model;
		uint32_t m_MeshesCount;
		std::vector<Ref<Model>> m_Frames;
		std::list<FrameTransition> m_TransitionQueue;
		std::unordered_map<std::string, std::vector<uint32_t>> m_Animations;
	};
}

