#include "BsfPch.h"

#include "Scene.h"
#include "Application.h"

namespace bsf
{
	Scene::~Scene()
	{
	}

	void Scene::OnAttach(Application& app)
	{
	}

	void Scene::OnRender(Application& app, const Time& time)
	{
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void Scene::OnDetach(Application& app)
	{
	}
}
