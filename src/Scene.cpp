#include "BsfPch.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "Common.h";
#include "Scene.h"
#include "Application.h"
#include "Renderer2D.h"

namespace bsf
{
	Scene::~Scene()
	{
	}

	void Scene::OnAttach()
	{
	}

	void Scene::OnRender(const Time& time)
	{
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void Scene::OnDetach()
	{
	}

	Application& Scene::GetApplication()
	{
		assert(m_App != nullptr);
		return *m_App;
	}

	FadeTask::FadeTask(glm::vec4 fromColor, glm::vec4 toColor, float duration) :
		m_FromColor(fromColor),
		m_ToColor(toColor),
		m_Duration(duration),
		m_Time(0.0f)
	{
		assert(duration > 0.0f);

		SetUpdateFunction([this](SceneTask& self, const Time& time) {
			float delta = m_Time / m_Duration;
			auto& renderer2d = GetApplication().GetRenderer2D();

			renderer2d.Begin(glm::ortho(0.0f, 1.0f, 0.0f, 1.0f));
			renderer2d.Pivot(EPivot::BottomLeft);
			renderer2d.Color(glm::mix(m_FromColor, m_ToColor, delta));
			renderer2d.DrawQuad({ 0, 0 });
			renderer2d.End();

			m_Time = std::min(m_Time + time.Delta, m_Duration);

			if (m_Time == m_Duration)
				SetDone();
		});

	}


	void SceneTask::CallUpdateFn(const Time& time)
	{
		if (!m_IsStarted)
		{
			m_IsStarted = true;
			m_StartTime = time;
		}
		m_UpdateFn != nullptr ? m_UpdateFn(*this, time) : SetDone();
	}

	void SceneTask::CallDoneFn()
	{
		if (m_DoneFn)
			m_DoneFn(*this);
	}

	Application& SceneTask::GetApplication()
	{
		if (m_Application == nullptr)
		{
			BSF_ERROR("Application is null");
		}
		return *m_Application;
	}

	WaitForTask::WaitForTask(float seconds) :
		m_Duration(seconds),
		m_Time(0.0f)
	{
		SetUpdateFunction([&](SceneTask& self, const Time& time) {
			if (m_Time >= m_Duration)
				SetDone();

			m_Time += time.Delta;
		});
	}

}
