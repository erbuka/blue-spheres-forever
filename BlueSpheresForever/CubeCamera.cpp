#include "CubeCamera.h"

#include <glm/ext.hpp>

#include "Log.h"
#include "ShaderProgram.h"

namespace bsf 
{
	CubeCamera::CubeCamera(uint32_t size, GLenum internalFormat, GLenum format, GLenum type) : m_Size(size)
	{
		BSF_GLCALL(glGenFramebuffers(1, &m_fbId));
		BSF_GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, m_fbId));

		m_fbDepth = MakeRef<Texture2D>();
		m_fbDepth->Bind(0);
		BSF_GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, size, size, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 0));
		BSF_GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fbDepth->GetId(), 0));

		auto status = BSF_GLCALL(glCheckFramebufferStatus(GL_FRAMEBUFFER));
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			BSF_ERROR("Could not initialize cube camera");
		}

		m_TextureCube = MakeRef<TextureCube>(size, size);

		m_Projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

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
		BSF_GLCALL(glDeleteFramebuffers(1, &m_fbId));
	}

	void CubeCamera::BindForRender(TextureCubeFace face)
	{

		glViewport(0, 0, m_Size, m_Size);

		m_ActiveView = m_Views[face];

		m_TextureCube->Bind(0);

		BSF_GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, m_fbId));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + uint32_t(face), m_TextureCube->GetId(), 0);

	}

}