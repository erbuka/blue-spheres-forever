#include "BsfPch.h"

#include "SkyGenerator.h"
#include "Common.h"
#include "Texture.h"
#include "CubeCamera.h"
#include "ShaderProgram.h"
#include "VertexArray.h"
#include "Assets.h"
#include "Color.h"

namespace bsf
{

	SkyGenerator::SkyGenerator()
	{
		// Vertex arrays
		m_vaCube = CreateCube();

		// Shaders
		m_pGenBg = ShaderProgram::FromFile("assets/shaders/sky_generator/sky_gen_bg.vert", "assets/shaders/sky_generator/sky_gen_bg.frag");
		m_pGenStars = ShaderProgram::FromFile("assets/shaders/sky_generator/sky_gen_stars.vert", "assets/shaders/sky_generator/sky_gen_stars.frag");
		m_pGenIrradiance = ShaderProgram::FromFile("assets/shaders/sky_generator/sky_gen_irradiance.vert", "assets/shaders/sky_generator/sky_gen_irradiance.frag");
		
		// Images
		m_imNoise = LoadCubeImage(
			"assets/textures/noise_front5.png",
			"assets/textures/noise_back6.png",
			"assets/textures/noise_left2.png",
			"assets/textures/noise_right1.png",
			"assets/textures/noise_bottom4.png",
			"assets/textures/noise_top3.png");

		m_imStars = LoadCubeImage(
			"assets/textures/stars_front5.png",
			"assets/textures/stars_back6.png",
			"assets/textures/stars_left2.png",
			"assets/textures/stars_right1.png",
			"assets/textures/stars_bottom4.png",
			"assets/textures/stars_top3.png");
	}

	Ref<Sky> SkyGenerator::Generate(const Options& options)
	{
		auto env = GenerateEnvironment(options);
		auto irr = GenerateIrradiance(env, 32);
		return Ref<Sky>(new Sky(env, irr));
	}


	SkyGenerator::CubeImage SkyGenerator::LoadCubeImage(std::string_view front, std::string_view back, std::string_view left, std::string_view right, std::string_view bottom, std::string_view top)
	{
		CubeImage result;

		result[TextureCubeFace::Front] = std::move(std::get<0>(ImageLoad(front, false)));
		result[TextureCubeFace::Back] = std::move(std::get<0>(ImageLoad(back, false)));
		result[TextureCubeFace::Left] = std::move(std::get<0>(ImageLoad(left, false)));
		result[TextureCubeFace::Right] = std::move(std::get<0>(ImageLoad(right, false)));
		result[TextureCubeFace::Bottom] = std::move(std::get<0>(ImageLoad(bottom, false)));
		result[TextureCubeFace::Top] = std::move(std::get<0>(ImageLoad(top, false)));

		return result;
	}

	Ref<TextureCube> SkyGenerator::GenerateEnvironment(const Options& options)
	{	

		GLEnableScope scope({ GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND });
		auto& assets = Assets::GetInstance();
		auto& starTex = assets.Get<Texture2D>(AssetName::TexWhite);

		auto bgPattern = Ref<TextureCube>(new TextureCube(1024, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE));
		auto starsPattern = Ref<TextureCube>(new TextureCube(1024, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE));

		for (auto face : TextureCubeFaces)
		{
			bgPattern->SetPixels(face, m_imNoise[face].data());
			starsPattern->SetPixels(face, m_imStars[face].data());
		}

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
				m_pGenBg->UniformTexture("uBackgroundPattern", bgPattern);
				m_pGenBg->UniformTexture("uGradient", gradient);
				m_vaCube->DrawArrays(GL_TRIANGLES);
			}


			// Draw stars
			{
				m_pGenStars->Use();
				m_pGenStars->UniformMatrix4f("uProjection", camera.GetProjectionMatrix());
				m_pGenStars->UniformMatrix4f("uView", camera.GetViewMatrix());
				m_pGenStars->UniformMatrix4f("uModel", glm::identity<glm::mat4>());
				m_pGenStars->UniformTexture("uStarsPattern", starsPattern);
				m_vaCube->DrawArrays(GL_TRIANGLES);
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
			modSkyBox->DrawArrays(GL_TRIANGLES);

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
		for (auto face : TextureCubeFaces) {
			camera->BindForRender(face);

			glClearColor(0, 0, 0, 1);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

			m_pSky->Use();
			m_pSky->UniformMatrix4f("uProjection", camera->GetProjectionMatrix());
			m_pSky->UniformMatrix4f("uView", camera->GetViewMatrix());
			m_pSky->UniformMatrix4f("uModel", glm::identity<glm::mat4>());
			m_pSky->UniformTexture("uSkyBox", source);
			m_VertexArray->DrawArrays(GL_TRIANGLES);

		}
	}

	Ref<Sky> GenerateDefaultSky()
	{
		// Sky
		return Assets::GetInstance().Get<SkyGenerator>(AssetName::SkyGenerator)->Generate({
			1024,
			Colors::Blue,
			Darken(Colors::Blue, 0.5f)
		});
	}

}