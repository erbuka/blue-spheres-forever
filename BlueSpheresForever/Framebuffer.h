#pragma once

#include "Common.h"

#include <cinttypes>
#include <unordered_map>
#include <glad/glad.h>

namespace bsf
{
	class Texture2D;

	class Framebuffer
	{
	public:


		Framebuffer(uint32_t width, uint32_t height, bool hasDepth);
		~Framebuffer();

		Ref<Texture2D> AddColorAttachment(const std::string& name, GLenum internalFormat = GL_RGBA8, GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE);

		Ref<Texture2D> GetColorAttachment(const std::string& name);

		Ref<Texture2D> GetDepthAttachment() { return m_DepthAttachment; }

		void Resize(uint32_t width, uint32_t height);

		void Bind();
		void Unbind();

		bool Check();

	private:

		struct ColorAttachment
		{
			GLenum InternalFormat, Format, Type;
			Ref<Texture2D> Texture;
		};

		Ref<Texture2D> m_DepthAttachment;
		std::unordered_map<std::string, ColorAttachment> m_ColorAttachments;

		uint32_t m_Id;
		uint32_t m_Width, m_Height;
	};
}

