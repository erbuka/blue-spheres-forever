#include "BsfPch.h"

#include "SkyGenerator.h"
#include "Texture.h"
#include "CubeCamera.h"
#include "ShaderProgram.h"
#include "VertexArray.h"
#include "Assets.h"

namespace bsf
{
	static constexpr std::array<float, 6 * 5> s_Billboad = {
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,

		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f
	};

	SkyGenerator::SkyGenerator()
	{
		// Vertex arrays
		m_vaCube = CreateCube();

		auto vbBillboard = Ref<VertexBuffer>(new VertexBuffer({
			{ "aPosition", AttributeType::Float3 },
			{ "aUv", AttributeType::Float2}
		}, s_Billboad.data(), 6));
		m_vaBillboard = Ref<VertexArray>(new VertexArray(6, { vbBillboard }));

		// Shaders
		m_pGenBg = ShaderProgram::FromFile("assets/shaders/sky_generator/sky_gen_bg.vert", "assets/shaders/sky_generator/sky_gen_bg.frag");
		m_pGenStars = ShaderProgram::FromFile("assets/shaders/sky_generator/sky_gen_stars.vert", "assets/shaders/sky_generator/sky_gen_stars.frag");
	}

	Ref<TextureCube> SkyGenerator::Generate(const Options& options)
	{	

		return Ref<TextureCube>(new TextureCube(256, "assets/textures/skybox.png"));

		GLEnableScope scope({ GL_DEPTH_TEST, GL_CULL_FACE });
		auto& assets = Assets::GetInstance();
		auto& starTex = assets.Get<Texture2D>(AssetName::TexWhite);
		auto bgPattern = Ref<TextureCube>(new TextureCube(1024, "assets/textures/skyback.png"));

		CubeCamera camera(options.Size, GL_RGB16F, GL_RGB, GL_HALF_FLOAT);

		// Generate stars
		std::vector<glm::vec3> starPos(1000);

		for (auto& pos : starPos)
			pos = glm::ballRand(100.0f);
		

		for (auto face : TextureCubeFaces)
		{
			camera.BindForRender(face);

			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);


			// Draw background
			{
				glDepthMask(GL_FALSE);
				m_pGenBg->Use();
				m_pGenBg->UniformMatrix4f("uProjection", camera.GetProjectionMatrix());
				m_pGenBg->UniformMatrix4f("uView", camera.GetViewMatrix());
				m_pGenBg->UniformMatrix4f("uModel", glm::identity<glm::mat4>());
				m_pGenBg->Uniform3fv("uColor0", 1, glm::value_ptr(options.BaseColor0));
				m_pGenBg->Uniform3fv("uColor1", 1, glm::value_ptr(options.BaseColor1));
				m_pGenBg->UniformTexture("uBackgroundPattern", bgPattern, 1);
				m_vaCube->Draw(GL_TRIANGLES);
				glDepthMask(GL_TRUE);
			}
			// Draw stars
			m_pGenStars->Use();
			m_pGenStars->UniformMatrix4f("uProjection", camera.GetProjectionMatrix());
			m_pGenStars->UniformMatrix4f("uView", camera.GetViewMatrix());
			m_pGenStars->Uniform3f("uColor", { 20.0f, 20.0f, 20.0f });
			m_pGenStars->UniformTexture("uTexture", starTex, 0);
			
			for (const auto& pos : starPos)
			{
				m_pGenStars->UniformMatrix4f("uModel", glm::translate(pos));
				//assets.Get<VertexArray>(AssetName::ModSphere)->Draw(GL_TRIANGLES);
				//m_vaBillboard->Draw(GL_TRIANGLES);
			}


		}

		return camera.GetTexture();
	}

}