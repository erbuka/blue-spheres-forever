#pragma once

#include <vector>

#include "Scene.h"
#include "MatrixStack.h"
#include "Common.h"

namespace bsf
{
	class ShaderProgram;
	class Framebuffer;
	class VertexArray;

	class RTXTestScene : public Scene
	{
	public:
		void OnAttach() override;
		void OnRender(const Time& time) override;
		void OnDetach() override;

		void OnResize(const WindowResizedEvent& evt);

	private:

		std::vector<glm::vec3> m_SpherePositions;
		Ref<VertexArray> m_vaQuad;
		Ref<ShaderProgram> m_pQuad, m_pRtx;
		Ref<Framebuffer> m_fbRTX;
		MatrixStack m_Projection, m_View, m_Model;
	};
}

