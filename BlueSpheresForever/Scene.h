#pragma once

#include "Common.h"

namespace bsf
{
	class Application;

	class Scene
	{
	public:
		virtual ~Scene();
		virtual void OnAttach();
		virtual void OnRender(const Time& time);
		virtual void OnDetach();

		Application& GetApplication();

	private:
		friend class Application;
		Application* m_App;
	};

}