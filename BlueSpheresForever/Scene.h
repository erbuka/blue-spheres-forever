#pragma once

#include "Common.h"
namespace bsf
{
	class Application;

	class Scene
	{
	public:
		virtual ~Scene();
		virtual void OnAttach(Application& app);
		virtual void OnRender(Application& app, const Time& time);
		virtual void OnDetach(Application& app);
	};

}