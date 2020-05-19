#pragma once

#include "Common.h"

#include <list>
#include <unordered_map>
#include <functional>

namespace bsf
{
	class Application;

	using SceneTaskDoneCallback = std::function<void()>;

	enum class ESceneTaskEvent
	{
		PreRender,
		PostRender
	};

	class SceneTask
	{
	public:
		SceneTask(SceneTaskDoneCallback done) : m_DoneCallback(done), m_IsDone(false), m_Application(nullptr) {}
		virtual ~SceneTask() {}
		virtual void Update(const Time& time) = 0;
		void Done() { m_IsDone = true; }
		bool IsDone() const { return m_IsDone; }
		Application& GetApplication();

	private:
		friend class Application;
		Application* m_Application = nullptr;
		SceneTaskDoneCallback m_DoneCallback;
		bool m_IsDone;
	};

	class FadeTask : public SceneTask
	{
	public:
		FadeTask(glm::vec4 fromColor, glm::vec4 toColor, float duration, SceneTaskDoneCallback done);
		void Update(const Time& time) override;
	private:
		float m_Time, m_Duration;
		glm::vec4 m_FromColor, m_ToColor;
	};

	class Scene
	{
	public:
		virtual ~Scene();
		virtual void OnAttach();
		virtual void OnRender(const Time& time);
		virtual void OnDetach();

		Application& GetApplication();

		template<typename T>
		void ScheduleTask(ESceneTaskEvent evt, const Ref<std::enable_if_t<std::is_base_of_v<SceneTask, T>, T>>& task) {
			m_ScheduledTasks[evt].push_back(task);
		}


	private:
		friend class Application;
		std::unordered_map<ESceneTaskEvent, std::list<Ref<SceneTask>>> m_ScheduledTasks;
		Application* m_App;
	};

}