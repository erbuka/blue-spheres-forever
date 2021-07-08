#include "BsfPch.h"

#include "BlurFilter.h"
#include "Common.h"

#include "Texture.h"
#include "Framebuffer.h"
#include "ShaderProgram.h"
#include "Assets.h"
#include "VertexArray.h"

namespace bsf
{
	BlurFilter::BlurFilter(const Ref<Texture2D>& source) :
		m_Source(source)
	{
		m_pHBlur = ShaderProgram::FromFile("assets/shaders/blur.vert", "assets/shaders/blur.frag");
		m_pVBlur = ShaderProgram::FromFile("assets/shaders/blur.vert", "assets/shaders/blur.frag", { "VERTICAL" });
		m_pCopy = ShaderProgram::FromFile("assets/shaders/copy.vert", "assets/shaders/copy.frag");
		
		m_Fb0 = MakeRef<Framebuffer>(source->GetWidth(), source->GetHeight(), false);
		m_Fb0->CreateColorAttachment("color", GL_RGB16F, GL_RGB, GL_HALF_FLOAT)->SetFilter(TextureFilter::Linear, TextureFilter::Linear);

		m_Fb1 = MakeRef<Framebuffer>(source->GetWidth(), source->GetHeight(), false);
		m_Fb1->CreateColorAttachment("color", GL_RGB16F, GL_RGB, GL_HALF_FLOAT)->SetFilter(TextureFilter::Linear, TextureFilter::Linear);
	}


	void BlurFilter::Apply(uint32_t iterations, uint32_t scale)
	{
		
		bool horizonal = true;
		uint32_t width = m_Source->GetWidth() / (1 << scale);
		uint32_t height = m_Source->GetHeight() / (1 << scale);

		
		GLEnableScope scope({ GL_DEPTH_TEST, GL_BLEND });

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glViewport(0, 0, width, height);

		m_Fb0->Resize(width, height);
		m_Fb1->Resize(width, height);

		Ref<Framebuffer> dst = m_Fb0;
		Ref<Framebuffer> src = m_Fb1;

		src->Bind();

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		m_pCopy->Use();
		m_pCopy->UniformTexture(HS("uSource"), m_Source);
		Assets::GetInstance().Get<VertexArray>(AssetName::ModClipSpaceQuad)->DrawArrays(GL_TRIANGLES);

		src->Unbind();


		for (uint32_t i = 0; i < iterations * 2; i++)
		{

			dst->Bind();

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			Ref<ShaderProgram>& prg = horizonal ? m_pHBlur : m_pVBlur;

			prg->Use();
			prg->UniformTexture(HS("uSource"), src->GetColorAttachment("color"));
			Assets::GetInstance().Get<VertexArray>(AssetName::ModClipSpaceQuad)->DrawArrays(GL_TRIANGLES);

			horizonal = !horizonal;
			std::swap(src, dst);
		}

		dst->Unbind();

	}
	Ref<Texture2D> BlurFilter::GetResult()
	{
		return m_Fb0->GetColorAttachment("color");
	}
}