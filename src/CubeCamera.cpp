#include "BsfPch.h"

#include "CubeCamera.h"
#include "Log.h"
#include "ShaderProgram.h"
#include "Framebuffer.h"

namespace bsf 
{
	CubeCamera::CubeCamera(uint32_t size, GLenum internalFormat, GLenum format, GLenum type) : m_Size(size)
	{
		m_Fb = MakeRef<Framebuffer>(size, size, true);
		
		m_TextureCube = MakeRef<TextureCube>(size, internalFormat, format, type);

		m_Projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);

		m_Views = {
		   { TextureCubeFace::Right, glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)) }, // +X
		   { TextureCubeFace::Left, glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)) }, // -X
		   { TextureCubeFace::Top, glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)) }, // +Y
		   { TextureCubeFace::Bottom, glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)) }, // -Y
		   { TextureCubeFace::Front, glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)) }, // +Z
		   { TextureCubeFace::Back, glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)) } // -Z
		};


	}

	CubeCamera::~CubeCamera()
	{
	}

	void CubeCamera::BindForRender(TextureCubeFace face)
	{

		static constexpr std::array<GLenum, 1> s_DrawBuffers = { GL_COLOR_ATTACHMENT0 };

		glViewport(0, 0, m_Size, m_Size);

		m_ActiveView = m_Views[face];

		m_TextureCube->Bind(0);

		m_Fb->Bind();

		BSF_GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + uint32_t(face), m_TextureCube->GetId(), 0));
		BSF_GLCALL(glDrawBuffers(s_DrawBuffers.size(), s_DrawBuffers.data()));

	}

}