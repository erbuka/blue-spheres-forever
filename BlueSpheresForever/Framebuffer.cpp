#include "Framebuffer.h"
#include "Texture.h"
#include "Log.h"


#include <glad/glad.h>

namespace bsf
{
	Framebuffer::Framebuffer(uint32_t width, uint32_t height, bool hasDepth) :
		m_Width(width),
		m_Height(height),
		m_DepthAttachment(nullptr)
	{
		BSF_GLCALL(glGenFramebuffers(1, &m_Id));
		Bind();

		if (hasDepth)
		{
			m_DepthAttachment = MakeRef<Texture2D>();
			m_DepthAttachment->Filter(TextureFilter::MinFilter, TextureFilterMode::Nearest);
			m_DepthAttachment->Filter(TextureFilter::MagFilter, TextureFilterMode::Nearest);
			m_DepthAttachment->Bind(0);

			BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0));
			BSF_GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment->GetId(), 0));
		}

		Unbind();

	}

	Framebuffer::~Framebuffer()
	{
		BSF_GLCALL(glDeleteFramebuffers(1, &m_Id));
	}

	Ref<Texture2D> Framebuffer::AddColorAttachment(const std::string& name)
	{
		if (m_ColorAttachments.find(name) != m_ColorAttachments.end())
		{
			BSF_ERROR("Color attachment '{0}' is already present", name);
			return nullptr;
		}

		auto att = MakeRef<Texture2D>();
		
		att->Filter(TextureFilter::MinFilter, TextureFilterMode::Nearest);
		att->Filter(TextureFilter::MagFilter, TextureFilterMode::Nearest);
		att->Bind(0);
		BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0));

		uint32_t attIdx = m_ColorAttachments.size();
		
		Bind();
		BSF_GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attIdx, GL_TEXTURE_2D, att->GetId(), 0));
		Unbind();

		m_ColorAttachments[name] = att;

		return att;
	}

	Ref<Texture2D> Framebuffer::GetColorAttachment(const std::string& name)
	{
		auto att = m_ColorAttachments.find(name);
		return att == m_ColorAttachments.end() ? nullptr : m_ColorAttachments[name];
	}

	void Framebuffer::Resize(uint32_t width, uint32_t height)
	{

		m_Width = width;
		m_Height = height;

		for (auto& ca : m_ColorAttachments)
		{
			ca.second->Bind(0);
			BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0));
		}

		if (m_DepthAttachment != nullptr)
		{
			m_DepthAttachment->Bind(0);
			BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0));
		}

	}

	void Framebuffer::Bind()
	{
		BSF_GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, m_Id));
	}

	void Framebuffer::Unbind()
	{
		BSF_GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}
	bool Framebuffer::Check()
	{
		Bind();
		auto status = BSF_GLCALL(glCheckFramebufferStatus(GL_FRAMEBUFFER));
		Unbind();
		return status == GL_FRAMEBUFFER_COMPLETE;
	}
}