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
	BlurFilter::BlurFilter(const Ref<Texture2D>& target) :
		m_Target(target)
	{
		m_pBlur = ShaderProgram::FromFile("assets/shaders/blur.vert", "assets/shaders/blur.frag");
		m_Fb = MakeRef<Framebuffer>(target->GetWidth(), target->GetHeight(), false);
		m_PingPong = MakeRef<Texture2D>(target->GetInternalFormat(), target->GetFormat(), target->GetType());
	}

	void BlurFilter::Apply(uint32_t iterations)
	{
		
		bool horizonal = true;
		uint32_t width = m_Target->GetWidth();
		uint32_t height = m_Target->GetHeight();

		Ref<Texture2D> src = m_Target;
		Ref<Texture2D> dst = m_PingPong;

		GLEnableScope scope({ GL_DEPTH_TEST, GL_BLEND });

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glViewport(0, 0, width, height);

		for (uint32_t i = 0; i < iterations * 2; i++)
		{
			m_Fb->SetColorAttachment("color", dst);
			if (dst->GetWidth() != width || dst->GetHeight() != height)
				m_Fb->Resize(width, height);
			m_Fb->Bind();

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			m_pBlur->Use();
			m_pBlur->UniformTexture("uSource", src);
			m_pBlur->Uniform1i("uHorizontal", { (int32_t)horizonal });
			Assets::GetInstance().Get<VertexArray>(AssetName::ModClipSpaceQuad)->DrawArrays(GL_TRIANGLES);

			horizonal = !horizonal;
			std::swap(src, dst);
		}

		m_Fb->Unbind();

	}
}