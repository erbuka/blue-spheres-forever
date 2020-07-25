#pragma once


#include <list>

#include "Common.h"
#include "Scene.h"
#include "MatrixStack.h"
#include "EventEmitter.h"

namespace bsf
{

	class Framebuffer;
	class ShaderProgram;

	class SplashScene : public Scene
	{
	public:
		void OnAttach() override;
		void OnRender(const Time& time) override;
		void OnDetach() override;

		void OnResize(const WindowResizedEvent& evt);

	private:
		Ref<Framebuffer> m_fbDeferred;
		Ref<ShaderProgram> m_pPBR, m_pDeferred;

		MatrixStack m_Projection, m_View, m_Model;

		std::list<Unsubscribe> m_Subscriptions;

	};
}