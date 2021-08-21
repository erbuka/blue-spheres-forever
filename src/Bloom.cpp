#include "BsfPch.h"

#include "Bloom.h"
#include "Log.h"
#include "Texture.h"
#include "VertexArray.h"
#include "Common.h"
#include "ShaderProgram.h"
#include "Config.h"

namespace bsf
{

	static constexpr std::array<GLenum, 1> s_DrawBuffers = { GL_COLOR_ATTACHMENT0 };

	void Bloom::UpdateTextures()
	{
		auto sw = m_txSource->GetWidth();
		auto sh = m_txSource->GetHeight();

		if (sw != m_Width || sh != m_Height)
		{
			m_Width = sw;
			m_Height = sh;

			m_vDownsample.clear();
			m_vUpsample.clear();

			while (sw >= 1 && sh >= 1)
			{
				auto ds = MakeRef<Texture2D>(GL_RGB16F, GL_RGB, GL_HALF_FLOAT);
				ds->SetPixels(nullptr, sw, sh);
				ds->SetFilter(TextureFilter::Linear, TextureFilter::Linear);
				ds->SetWrap(TextureWrap::ClampToEdge);
				m_vDownsample.push_back(std::move(ds));

				auto us = MakeRef<Texture2D>(GL_RGB16F, GL_RGB, GL_HALF_FLOAT);
				us->SetPixels(nullptr, sw, sh);
				us->SetFilter(TextureFilter::Linear, TextureFilter::Linear);
				us->SetWrap(TextureWrap::ClampToEdge);
				m_vUpsample.push_back(std::move(us));

				sw /= 2;
				sh /= 2;

			}

			m_vUpsample.back() = m_vDownsample.back();

		}

	}

	Bloom::Bloom(Ref<Texture2D> source) :
		m_txSource(std::move(source)),
		m_FramebufferId(0),
		m_Width(0),
		m_Height(0)
	{
		m_pPrefilter = ShaderProgram::FromFile("assets/shaders/bloom/prefilter.vert", "assets/shaders/bloom/prefilter.frag");
		m_pDownsample = ShaderProgram::FromFile("assets/shaders/bloom/downsample.vert", "assets/shaders/bloom/downsample.frag");
		m_pUpsample = ShaderProgram::FromFile("assets/shaders/bloom/upsample.vert", "assets/shaders/bloom/upsample.frag");

		m_vaQuad = CreateClipSpaceQuad();

		glGenFramebuffers(1, &m_FramebufferId);
	}

	Bloom::~Bloom()
	{
		glDeleteFramebuffers(1, &m_FramebufferId);
	}
	void Bloom::Apply()
	{
		const auto w = m_txSource->GetWidth();
		const auto h = m_txSource->GetHeight();

		UpdateTextures();

		GLEnableScope scope({ GL_DEPTH_TEST });

		glDisable(GL_DEPTH_TEST);

		// Prefilter
		glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferId);
		glDrawBuffers(1, s_DrawBuffers.data());
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_vDownsample[0]->GetId(), 0);

		glViewport(0, 0, m_vDownsample[0]->GetWidth(), m_vDownsample[0]->GetHeight());
		m_pPrefilter->Use();
		m_pPrefilter->UniformTexture(HS("uSource"), m_txSource);
		m_pPrefilter->Uniform1f(HS("uThreshold"), { GlobalShadingConfig::BloomThreshold });
		m_pPrefilter->Uniform1f(HS("uKick"), { GlobalShadingConfig::BloomKnee });
		//m_pPrefilter->Uniform1f(HS("uRange"), { GlobalShadingConfig::BloomRange });
		
		m_vaQuad->DrawArrays(GL_TRIANGLES);

		// Downsample and filter
		for (size_t i = 1; i < m_vDownsample.size(); ++i)
		{
			const auto& current = m_vDownsample[i];
			const auto w = current->GetWidth();
			const auto h = current->GetHeight();

			BSF_GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, current->GetId(), 0));
			glViewport(0, 0, w, h);
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);

			m_pDownsample->Use();
			m_pDownsample->UniformTexture(HS("uSource"), m_vDownsample[i - 1]);

			m_vaQuad->DrawArrays(GL_TRIANGLES);

		}

		// Upsample and filter
		for (int32_t i = m_vUpsample.size() - 2; i >= 0; --i)
		{
			const auto& current = m_vUpsample[i];
			const auto w = current->GetWidth();
			const auto h = current->GetHeight();

			BSF_GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, current->GetId(), 0));
			glViewport(0, 0, w, h);
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);


			m_pUpsample->Use();
			m_pUpsample->UniformTexture(HS("uPrevious"), m_vUpsample[i + 1]);
			m_pUpsample->UniformTexture(HS("uPrefilter"), m_vDownsample[i]);

			m_vaQuad->DrawArrays(GL_TRIANGLES);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}
	const Ref<Texture2D>& Bloom::GetResult()
	{
		return m_vUpsample[0];
	}
}