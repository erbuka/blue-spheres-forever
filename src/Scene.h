#pragma once

#include "Ref.h"
#include "Time.h"
#include "EventEmitter.h"

#include <list>
#include <unordered_map>
#include <functional>
#include <vector>

namespace bsf
{
	class Application;
	class Scene;

	enum class ESceneTaskEvent
	{
		PreRender,
		PostRender
	};

	class SceneTask
	{
	public:
		using DoneFn = std::function<void(SceneTask &)>;
		using UpdateFn = std::function<void(SceneTask &, const Time &)>;

		SceneTask() : m_DoneFn(nullptr), m_UpdateFn(nullptr), m_Application(nullptr),
					  m_IsDone(false), m_IsStarted(false) {}

		SceneTask(const UpdateFn &update) : SceneTask()
		{
			m_UpdateFn = update;
		}

		SceneTask(UpdateFn&& update) : SceneTask()
		{
			m_UpdateFn = std::move(update);
		}

		virtual ~SceneTask() {}

		void SetUpdateFunction(const UpdateFn &fn) { m_UpdateFn = fn; }
		void SetUpdateFunction(UpdateFn &&fn) { m_UpdateFn = std::move(fn); }

		void SetDoneFunction(const DoneFn &fn) { m_DoneFn = fn; }
		void SetDoneFunction(DoneFn &&fn) { m_DoneFn = std::move(fn); }

		Ref<SceneTask> Chain(Ref<SceneTask> next);

		const Time &GetStartTime() const { return m_StartTime; }

		void SetDone() { m_IsDone = true; }
		bool IsStarted() const { return m_IsStarted; }
		bool IsDone() const { return m_IsDone; }

		Application &GetApplication();
		Scene &GetScene();
		ESceneTaskEvent GetEvent() const { return m_Event; }

	private:
		void CallUpdateFn(const Time &time);
		void CallDoneFn();

		friend class Application;
		Application *m_Application = nullptr;
		Scene *m_Scene = nullptr;
		ESceneTaskEvent m_Event = ESceneTaskEvent::PreRender;
		DoneFn m_DoneFn;
		UpdateFn m_UpdateFn;
		Time m_StartTime;
		bool m_IsStarted;
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

	class WaitForTask : public SceneTask
	{
	public:
		WaitForTask(float seconds);

	private:
		float m_Duration, m_Time;
	};

	class Scene : public EventReceiver
	{
	public:
		Scene() = default;
		Scene(Scene &&) = delete;

		virtual ~Scene();
		virtual void OnAttach();
		virtual void OnRender(const Time &time);
		virtual void OnDetach();

		Application &GetApplication();

		void ScheduleTask(ESceneTaskEvent evt, const Ref<SceneTask> &task);

		template <typename T, typename... Args>
		Ref<std::enable_if_t<std::is_base_of_v<SceneTask, T>, T>> ScheduleTask(ESceneTaskEvent evt, Args &&... args)
		{
			auto task = MakeRef<T>(std::forward<Args>(args)...);
			ScheduleTask(evt, task);
			return task;
		}

	private:
		friend class Application;
		std::unordered_map<ESceneTaskEvent, std::list<Ref<SceneTask>>> m_ScheduledTasks;
		Application *m_App = nullptr;
	};

} // namespace bsf