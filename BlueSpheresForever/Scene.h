#pragma once

#include "Common.h"

#include <list>
#include <unordered_map>
#include <functional>

namespace bsf
{
	class Application;


	enum class ESceneTaskEvent
	{
		PreRender,
		PostRender
	};

	class SceneTask
	{
	public:
		using DoneFn = std::function<void(SceneTask&)>;
		using UpdateFn = std::function<void(SceneTask&, const Time&)>;

		SceneTask() : m_DoneFn(nullptr), m_UpdateFn(nullptr), m_IsDone(false), m_Application(nullptr) {}
		virtual ~SceneTask() {}

		void SetUpdateFunction(UpdateFn fn) { m_UpdateFn = fn; }
		void SetDoneFunction(DoneFn fn) { m_DoneFn = fn; }

		void SetDone() { m_IsDone = true; }
		bool IsDone() const { return m_IsDone; }
		Application& GetApplication();

	private:

		void CallUpdateFn(const Time& time);
		void CallDoneFn();

		friend class Application;
		Application* m_Application = nullptr;
		DoneFn m_DoneFn;
		UpdateFn m_UpdateFn;
		bool m_IsDone;
	};

	class FadeTask : public SceneTask
	{
	public:
		FadeTask(glm::vec4 fromColor, glm::vec4 toColor, float duration);
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