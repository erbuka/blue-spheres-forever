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

	Ref<Texture2D> Framebuffer::AddColorAttachment(const std::string& name, GLenum internalFormat, GLenum format, GLenum type)
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
		BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format, type, 0));

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
			BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0));
		}

	}

	void Framebuffer::Bind()
	{
		static std::array<uint32_t, 16>  s_ColorAttachments = {
			GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3,
			GL_COLOR_ATTACHMENT4,
			GL_COLOR_ATTACHMENT5,
			GL_COLOR_ATTACHMENT6,
			GL_COLOR_ATTACHMENT7,
			GL_COLOR_ATTACHMENT8,
			GL_COLOR_ATTACHMENT9,
			GL_COLOR_ATTACHMENT10,
			GL_COLOR_ATTACHMENT11,
			GL_COLOR_ATTACHMENT12,
			GL_COLOR_ATTACHMENT13,
			GL_COLOR_ATTACHMENT14,
			GL_COLOR_ATTACHMENT15
		};

		BSF_GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, m_Id));
		BSF_GLCALL(glDrawBuffers(m_ColorAttachments.size(), s_ColorAttachments.data()));
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