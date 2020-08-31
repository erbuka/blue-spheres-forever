#include "BsfPch.h"

#include "SkyGenerator.h"
#include "Common.h"
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
		m_pGenIrradiance = ShaderProgram::FromFile("assets/shaders/sky_generator/sky_gen_irradiance.vert", "assets/shaders/sky_generator/sky_gen_irradiance.frag");
	}

	Ref<Sky> SkyGenerator::Generate(const Options& options)
	{
		auto env = GenerateEnvironment(options);
		auto irr = GenerateIrradiance(env, 32);
		return Ref<Sky>(new Sky(env, irr));
	}


	Ref<TextureCube> SkyGenerator::GenerateEnvironment(const Options& options)
	{	

		GLEnableScope scope({ GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND });
		auto& assets = Assets::GetInstance();
		auto& starTex = assets.Get<Texture2D>(AssetName::TexWhite);
		auto bgPattern = Ref<TextureCube>(new TextureCube(
			1024,
			"assets/textures/noise_front5.png",
			"assets/textures/noise_back6.png",
			"assets/textures/noise_left2.png",
			"assets/textures/noise_right1.png",
			"assets/textures/noise_bottom4.png",
			"assets/textures/noise_top3.png"
		));

		auto starsPattern = Ref<TextureCube>(new TextureCube(
			1024,
			"assets/textures/stars_front5.png",
			"assets/textures/stars_back6.png",
			"assets/textures/stars_left2.png",
			"assets/textures/stars_right1.png",
			"assets/textures/stars_bottom4.png",
			"assets/textures/stars_top3.png"
		));

		CubeCamera camera(options.Size, GL_RGB16F, GL_RGB, GL_HALF_FLOAT);
		
		/*
		auto gradient = CreateGradient(16, {
			{ 0.0f, options.BaseColor0 },
			{ 0.3f, options.BaseColor0 },
			{ 0.45f, glm::mix(options.BaseColor0, glm::vec3(1.0f, 1.0f, 0.0f), 0.5f) },
			{ 0.6f, glm::mix(options.BaseColor1, glm::vec3(1.0f, 0.0f, 1.0f), 0.5f) },
			{ 0.75f, options.BaseColor1 },
			{ 1.0f, options.BaseColor1 }
		});
		*/
		auto gradient = CreateGradient(128, {
			{ 0.0f, options.BaseColor0 },
			{ 0.3f, glm::mix(options.BaseColor0, glm::vec3(1.0f, 1.0f, 0.0f), 0.1f) },
			{ 0.6f, glm::mix(options.BaseColor1, glm::vec3(1.0f, 0.0f, 1.0f), 0.1f) },
			{ 1.0f, options.BaseColor1 }
		});

		// Generate stars
		for (auto face : TextureCubeFaces)
		{
			camera.BindForRender(face);

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



			// Draw background
			{
				m_pGenBg->Use();
				m_pGenBg->UniformMatrix4f("uProjection", camera.GetProjectionMatrix());
				m_pGenBg->UniformMatrix4f("uView", camera.GetViewMatrix());
				m_pGenBg->UniformMatrix4f("uModel", glm::identity<glm::mat4>());
				//m_pGenBg->Uniform3fv("uColor0", 1, glm::value_ptr(options.BaseColor0));
				//m_pGenBg->Uniform3fv("uColor1", 1, glm::value_ptr(options.BaseColor1));
				m_pGenBg->UniformTexture("uBackgroundPattern", bgPattern);
				m_pGenBg->UniformTexture("uGradient", gradient);
				m_vaCube->Draw(GL_TRIANGLES);
			}


			// Draw stars
			{
				m_pGenStars->Use();
				m_pGenStars->UniformMatrix4f("uProjection", camera.GetProjectionMatrix());
				m_pGenStars->UniformMatrix4f("uView", camera.GetViewMatrix());
				m_pGenStars->UniformMatrix4f("uModel", glm::identity<glm::mat4>());
				m_pGenStars->UniformTexture("uStarsPattern", starsPattern);
				m_vaCube->Draw(GL_TRIANGLES);
			}
		}

		return camera.GetTexture();
	}

	Ref<TextureCube> SkyGenerator::GenerateIrradiance(const Ref<TextureCube>& sky, uint32_t size)
	{
		auto camera = MakeRef<CubeCamera>(size, GL_RGB16F, GL_RGB, GL_HALF_FLOAT);
		static std::array<TextureCubeFace, 6> faces = {
			TextureCubeFace::Right,
			TextureCubeFace::Left,
			TextureCubeFace::Top,
			TextureCubeFace::Bottom,
			TextureCubeFace::Front,
			TextureCubeFace::Back
		};

		GLEnableScope scope({ GL_DEPTH_TEST });
		glEnable(GL_DEPTH_TEST);

		auto modSkyBox = Assets::GetInstance().Get<VertexArray>(AssetName::ModSkyBox);

		for (auto face : faces)
		{
			camera->BindForRender(face);

			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			m_pGenIrradiance->Use();
			m_pGenIrradiance->UniformTexture("uEnvironment", sky);
			m_pGenIrradiance->UniformMatrix4f("uProjection", camera->GetProjectionMatrix());
			m_pGenIrradiance->UniformMatrix4f("uView", camera->GetViewMatrix());
			m_pGenIrradiance->UniformMatrix4f("uModel", glm::identity<glm::mat4>());
			modSkyBox->Draw(GL_TRIANGLES);

		}


		return camera->GetTexture();
	}

	Sky::Sky(const Ref<TextureCube>& env, const Ref<TextureCube>& irradiance) :
		m_SrcEnv(env), m_SrcIrr(irradiance)
	{
		m_Vertices = CreateCubeData();
		m_VertexArray = CreateCube();
		
		m_pSky = ShaderProgram::FromFile("assets/shaders/skybox.vert", "assets/shaders/skybox.frag");
		m_Env = Ref<CubeCamera>(new CubeCamera(env->GetSize(), GL_RGB16F, GL_RGB, GL_HALF_FLOAT));
		m_Irr = Ref<CubeCamera>(new CubeCamera(irradiance->GetSize(), GL_RGB16F, GL_RGB, GL_HALF_FLOAT));
		
		ApplyMatrix(glm::identity<glm::mat4>());

	}

	void Sky::ApplyMatrix(const glm::mat4& matrix)
	{
		for (auto& v : m_Vertices)
			v[0] = matrix * glm::vec4(v[0], 1.0);

		m_VertexArray->GetVertexBuffer(0)->SetSubData(m_Vertices.data(), 0, m_Vertices.size());

		Update(m_Env, m_SrcEnv);
		Update(m_Irr, m_SrcIrr);
	}

	const Ref<TextureCube>& Sky::GetEnvironment()
	{
		return m_Env->GetTexture();
	}

	const Ref<TextureCube>& Sky::GetIrradiance()
	{
		return m_Irr->GetTexture();
	}

	void Sky::Update(Ref<CubeCamera>& camera, Ref<TextureCube>& source)
	{

		auto& assets = Assets::GetInstance();

		static std::array<TextureCubeFace, 6> faces = {
			TextureCubeFace::Right,
			TextureCubeFace::Left,
			TextureCubeFace::Top,
			TextureCubeFace::Bottom,
			TextureCubeFace::Front,
			TextureCubeFace::Back
		};


		for (auto face : faces) {
			camera->BindForRender(face);

			glClearColor(0, 0, 0, 1);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

			m_pSky->Use();
			m_pSky->UniformMatrix4f("uProjection", camera->GetProjectionMatrix());
			m_pSky->UniformMatrix4f("uView", camera->GetViewMatrix());
			m_pSky->UniformMatrix4f("uModel", glm::identity<glm::mat4>());
			m_pSky->UniformTexture("uSkyBox", source);
			m_VertexArray->Draw(GL_TRIANGLES);

		}
	}

}