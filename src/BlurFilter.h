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
		BlurFilter(const Ref<Texture2D>& source);
		void Apply(uint32_t iterations, uint32_t scale);
		Ref<Texture2D> GetResult();
	private:
		Ref<ShaderProgram> m_pHBlur, m_pVBlur, m_pCopy;
		Ref<Framebuffer> m_Fb0, m_Fb1;
		Ref<Texture2D> m_txSource;
	};
}

