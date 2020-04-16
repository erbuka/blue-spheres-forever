#pragma once

#include "Common.h"

#include <cinttypes>
#include <vector>

namespace bsf
{
	class Texture2D;

	class Framebuffer
	{
	public:

		Framebuffer(uint32_t width, uint32_t height, bool hasDepth);
		~Framebuffer();

		Ref<Texture2D> AddColorAttachment();

		const std::vector<Ref<Texture2D>>& GetColorAttachments() const;

		void Resize(uint32_t width, uint32_t height);

		void Bind();
		void Unbind();

		bool Check();

	private:

		Ref<Texture2D> m_DepthAttachment;
		std::vector<Ref<Texture2D>> m_ColorAttachments;

		uint32_t m_Id;
		uint32_t m_Width, m_Height;
	};
}

