#pragma once

#include <list>

#include "Ref.h"
#include "Scene.h"
#include "MatrixStack.h"
#include "EventEmitter.h"

namespace bsf
{

	class Framebuffer;
	class ShaderProgram;
	class Sky;
	class BlurFilter;
	class Renderer2D;

	class SplashScene : public Scene
	{
	public:
		void OnAttach() override;
		void OnRender(const Time& time) override;
		void OnDetach() override;

		void OnResize(const WindowResizedEvent& evt);

	private:
			
		void DrawEmeralds(const Time& time);
		void DrawTitle(Renderer2D& r2, const Time& time);

		bool m_DisplayTitle = false;

		Ref<BlurFilter> m_fBlur;
		Ref<Framebuffer> m_fbPBR;
		Ref<ShaderProgram> m_pPBR, m_pDeferred, m_pSky;
		Ref<Sky> m_Sky;

		MatrixStack m_Projection, m_View, m_Model;

		
	};
}