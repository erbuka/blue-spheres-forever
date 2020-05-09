#pragma once

#include "Application.h"
#include "Common.h"
#include "Scene.h"
#include "MatrixStack.h"
#include "EventEmitter.h"

namespace bsf
{

	class ShaderProgram;
	class VertexArray;
	class Stage;
	class GameLogic;
	class Texture2D;
	class TextureCube;
	class Framebuffer;
	class Renderer2D;
	class CubeCamera;

	struct BoxVertex {
		glm::vec3 Position, Uv;
		BoxVertex(const glm::vec3 v) : Position(v), Uv(v) {}
	};

	class GameScene : public Scene
	{
	public:
			
		GameScene(const Ref<Stage>& stage);

		void OnAttach(Application& app) override;
		void OnRender(Application& app, const Time& time) override;
		void OnDetach(Application& app) override;

		void OnResize(const WindowResizedEvent& evt);

	private:

		Ref<TextureCube> CreateBaseSkyBox();
		Ref<TextureCube> CreateBaseIrradianceMap(const Ref<TextureCube>& source, uint32_t size);


		void RotateDynamicCubeMap(const glm::vec2& position, const glm::vec2& deltaPosition, const glm::vec2& windowSize);
		void GenerateDynamicCubeMap(Ref<CubeCamera>& camera, Ref<TextureCube> source);

		std::vector<Unsubscribe> m_Subscriptions;
		MatrixStack m_Model, m_View, m_Projection;

		Ref<Renderer2D> m_Renderer2D;
		Ref<Framebuffer> m_fbDeferred;

		Ref<VertexArray> m_vaWorld, m_vaSphere, m_vaStars, m_vaQuad, m_vaSkyBox, m_vaDynSkyBox;
		Ref<ShaderProgram> m_pPBR, m_pStars, m_pSkyGradient, m_pDeferred, m_pSkyBox, m_pIrradiance;
		
		Ref<TextureCube> m_txBaseSkyBox, m_txBaseIrradiance;
		Ref<CubeCamera> m_ccSkyBox, m_ccIrradiance;
		Ref<Texture2D> m_txGroundMap;
		Ref<GameLogic> m_GameLogic;
		
		Ref<Stage> m_Stage;

		std::vector<BoxVertex> m_vDynSkyBoxVertices;

	};
}

