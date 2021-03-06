#pragma once

#include <inttypes.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <unordered_map>

#include "Ref.h"
#include "Texture.h"


namespace bsf
{
	class ShaderProgram;
	class Framebuffer;

	class CubeCamera
	{
	public:
		CubeCamera(uint32_t size, GLenum internalFormat, GLenum format, GLenum type);
		~CubeCamera();

		const glm::mat4& GetProjectionMatrix() const { return m_Projection; } 
		const glm::mat4& GetViewMatrix() const { return m_ActiveView; }

		const Ref<TextureCube>& GetTexture() const { return m_TextureCube; }

		void BindForRender(TextureCubeFace face);

	private:

		glm::mat4 m_Projection, m_ActiveView;
		std::unordered_map<TextureCubeFace, glm::mat4> m_Views;

		Ref<Framebuffer> m_Fb;
		Ref<TextureCube> m_TextureCube;
		
		uint32_t m_Size;
	};
}

