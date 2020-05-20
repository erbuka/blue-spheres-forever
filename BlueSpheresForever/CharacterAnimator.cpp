#include "BsfPch.h"

#include "CharacterAnimator.h"
#include "Model.h"
#include "VertexArray.h"

namespace bsf
{
	CharacterAnimator::CharacterAnimator() :
		m_MeshesCount(0),
		m_Model(nullptr),
		m_TimeMultiplier(1.0f),
		m_Running(true)
	{
	}
	void CharacterAnimator::AddFrame(const Ref<Model>& frame)
	{
		if (m_MeshesCount == 0)
			m_MeshesCount = frame->GetMeshCount();

		if (frame->GetMeshCount() != m_MeshesCount)
		{
			BSF_ERROR("Invalid meshes count. Expected {0}, got {1}", m_MeshesCount, frame->GetMeshCount());
			return;
		}

		m_Frames.push_back(frame);

	}

	void CharacterAnimator::Play(const std::string& animationName, float duration, bool loop)
	{
		const auto& animIt = m_Animations.find(animationName);

		if (animIt == m_Animations.end())
		{
			BSF_ERROR("Animation not found: {0}", animationName);
			return;
		}

		const auto frames = (*animIt).second;

		m_TransitionQueue.clear();

		float transitionDuration = duration / (frames.size() - 1);

		for (uint32_t i = 0; i < frames.size() - 1; i++)
			m_TransitionQueue.emplace_back(frames[i], frames[i + 1], transitionDuration);

		if (m_Reverse)
			Reverse();
	}

	void CharacterAnimator::Reverse()
	{
		if (!m_TransitionQueue.empty())
		{
			// Reverse the current transition time
			auto& current = m_TransitionQueue.front();
			current.CurrentTime = current.Duration - current.GetDelta() * current.Duration;

			// Reverse all the frame order
			for (auto& t : m_TransitionQueue)
				std::swap(t.FromFrame, t.ToFrame);

			// Reverse the list
			m_TransitionQueue.reverse();

			// Bring the last element in front
			m_TransitionQueue.push_front(m_TransitionQueue.back());
			m_TransitionQueue.pop_back();
		}
	}

	void CharacterAnimator::SetReverse(bool reverse)
	{
		if (reverse != m_Reverse)
		{
			m_Reverse = reverse;
			Reverse();
		}
	}


	void CharacterAnimator::Update(const Time& time)
	{
		if (!m_Running)
			return;

		float dt = time.Delta * m_TimeMultiplier;

		while (dt > 0.0f && !m_TransitionQueue.empty())
		{
			auto& current = m_TransitionQueue.front();
			current.Update(dt);
			dt = current.IsDone() ? current.GetExcessTime() : 0.0f;

			if (current.IsDone())
			{
				current.Reset();
				m_TransitionQueue.push_back(current);
				m_TransitionQueue.pop_front();
			}

		}

		UpdateModel();

	}

	void CharacterAnimator::RegisterAnimation(const std::string& name, const std::initializer_list<uint32_t>& frames)
	{

		assert(frames.size() > 1);

#ifdef DEBUG

		for (auto frame : frames)
		{
			assert(frame < m_Frames.size());
		}
#endif // DEBUG


		m_Animations[name] = frames;
	}


	void CharacterAnimator::UpdateModel()
	{
		InitializeModel();

		if (!m_TransitionQueue.empty())
		{
			const auto& current = m_TransitionQueue.front();

			for (uint32_t i = 0; i < m_MeshesCount; i++)
			{
				m_Model->GetMesh(i)->SetVertexBuffer(0, m_Frames[current.FromFrame]->GetMesh(i)->GetVertexBuffer(0));
				m_Model->GetMesh(i)->SetVertexBuffer(1, m_Frames[current.ToFrame]->GetMesh(i)->GetVertexBuffer(0));
			}
		}


	}

	void CharacterAnimator::InitializeModel()
	{
		assert(m_Frames.size() > 1);

		if (m_Model != nullptr)
			return;

		m_Model = MakeRef<Model>();

		uint32_t vertexCount = m_Frames[0]->GetMesh(0)->GetVertexCount();

		for (uint32_t i = 0; i < m_MeshesCount; i++)
		{
			auto va = Ref<VertexArray>(new VertexArray(vertexCount, 2));
			m_Model->AddMesh(va);
		}

	}

	FrameTransition::FrameTransition(uint32_t fromFrame, uint32_t toFrame, float duration) :
		FromFrame(fromFrame), 
		ToFrame(toFrame),
		Duration(duration),
		CurrentTime(0.0f)
	{
	}
	
	void FrameTransition::Reset()
	{
		CurrentTime = 0.0f;
	}
}