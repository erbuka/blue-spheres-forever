#pragma once

#include "Ref.h"

namespace bsf
{
	class Texture2D;
	class VertexArray;
	class ShaderProgram;

	class Bloom
	{
	private:

		uint32_t m_Width, m_Height;

		uint32_t m_FramebufferId;
		Ref<Texture2D> m_txSource;

		std::vector<Ref<Texture2D>> m_vDownsample, m_vUpsample;

		Ref<VertexArray> m_vaQuad;
		Ref<ShaderProgram> m_pPrefilter, m_pDownsample, m_pUpsample;

		void UpdateTextures();

	public:
		Bloom(Ref<Texture2D> source);
		~Bloom();

		void Apply();

		const Ref<Texture2D>& GetResult();
	};
}