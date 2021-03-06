#pragma once

#include "Ref.h"

namespace bsf
{
	class Texture2D;
	class Framebuffer;
	class ShaderProgram;

	class BlurFilter
	{
	public:
		BlurFilter(const Ref<Texture2D>& target);
		void Apply(uint32_t iterations);
	private:
		Ref<ShaderProgram> m_pBlur;
		Ref<Framebuffer> m_Fb;
		Ref<Texture2D> m_Target, m_PingPong;
	};
}

