#include "BsfPch.h"

#include "Scene.h"
#include "Application.h"

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
}
