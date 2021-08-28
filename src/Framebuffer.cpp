#include "BsfPch.h"

#include "Framebuffer.h"
#include "Texture.h"
#include "Log.h"


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
			m_DepthAttachment = MakeRef<Texture2D>(GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
			m_DepthAttachment->Bind(0);
			m_DepthAttachment->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
			m_DepthAttachment->SetPixels(nullptr, width, height);

			BSF_GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment->GetId(), 0));
		}

		Unbind();

	}

	Framebuffer::~Framebuffer()
	{
		BSF_GLCALL(glDeleteFramebuffers(1, &m_Id));
	}

	Ref<Texture2D> Framebuffer::SetColorAttachment(const std::string& name, const Ref<Texture2D>& att)
	{

		auto it = std::find_if(m_ColorAttachments.begin(), m_ColorAttachments.end(), [&](const auto& val) { return val.first == name; });

		if (it != m_ColorAttachments.end())
			it->second = att;
		else
			m_ColorAttachments.push_back({ name, att });

		return att;
	}

	Ref<Texture2D> Framebuffer::CreateColorAttachment(const std::string& name, GLenum internalFormat, GLenum format, GLenum type)
	{
		if (GetColorAttachment(name) != nullptr)
		{
			BSF_ERROR("Color attachment '{0}' is already present", name);
			return nullptr;
		}

		auto att = MakeRef<Texture2D>(internalFormat, format, type);
		att->Bind(0);
		att->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
		att->SetPixels(nullptr, m_Width, m_Height);
		uint32_t attIdx = m_ColorAttachments.size();
	
		return SetColorAttachment(name, att);
	}

	Ref<Texture2D> Framebuffer::GetColorAttachment(const std::string& name)
	{
		auto att = std::find_if(m_ColorAttachments.begin(), m_ColorAttachments.end(), [&](const auto& val) { return val.first == name; });
		return att == m_ColorAttachments.end() ? nullptr : att->second;
	}

	void Framebuffer::Resize(uint32_t width, uint32_t height)
	{

		m_Width = width;
		m_Height = height;

		for (auto& ca : m_ColorAttachments)
			ca.second->SetPixels(nullptr, width, height);

		if (m_DepthAttachment != nullptr)
			m_DepthAttachment->SetPixels(nullptr, width, height);
	}

	void Framebuffer::Bind()
	{
		static constexpr std::array<uint32_t, 16> s_ColorAttachments = {
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

		for(uint32_t i = 0; i < m_ColorAttachments.size(); i++)
			BSF_GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_ColorAttachments[i].second->GetId(), 0));


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