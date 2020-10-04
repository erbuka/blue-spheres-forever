#include "BsfPch.h"

#include "SkyGenerator.h"
#include "Common.h"
#include "Texture.h"
#include "CubeCamera.h"
#include "ShaderProgram.h"
#include "VertexArray.h"
#include "Assets.h"
#include "Color.h"
#include "Diagnostic.h"

namespace bsf
{

	SkyGenerator::SkyGenerator()
	{
		// Vertex arrays
		m_vaCube = CreateCube();

		// Shaders
		m_pGenEnv = ShaderProgram::FromFile("assets/shaders/sky_generator/sky_gen_bg.vert", "assets/shaders/sky_generator/sky_gen.frag");
		m_pGenIrr = ShaderProgram::FromFile("assets/shaders/sky_generator/sky_gen_irradiance.vert", "assets/shaders/sky_generator/sky_gen_irradiance.frag");
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

		CubeCamera camera(options.Size, GL_RGB16F, GL_RGB, GL_HALF_FLOAT);

		// Generate stars
		for (auto face : TextureCubeFaces)
		{
			camera.BindForRender(face);

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			m_pGenEnv->Use();
			m_pGenEnv->UniformMatrix4f("uProjection", camera.GetProjectionMatrix());
			m_pGenEnv->UniformMatrix4f("uView", camera.GetViewMatrix());
			m_pGenEnv->UniformMatrix4f("uModel", glm::identity<glm::mat4>());

			m_pGenEnv->Uniform3fv("uBackColor", 1, glm::value_ptr(options.BaseColor));

			m_pGenEnv->Uniform3fv("uStarColor", 1, glm::value_ptr(options.StarsColor));

			m_pGenEnv->Uniform1f("uBackgroundScale", { 1.0f });
			m_pGenEnv->Uniform1f("uStarScale", { 180.0f });
			m_pGenEnv->Uniform1f("uStarBrightnessScale", { 200.0f });
			m_pGenEnv->Uniform1f("uStarPower", { 12.0f });
			m_pGenEnv->Uniform1f("uStarMultipler", { 24.0f });

			m_pGenEnv->Uniform1f("uCloudScale", { 0.75f });

			m_vaCube->DrawArrays(GL_TRIANGLES);
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

			m_pGenIrr->Use();
			m_pGenIrr->UniformTexture("uEnvironment", sky);
			m_pGenIrr->UniformMatrix4f("uProjection", camera->GetProjectionMatrix());
			m_pGenIrr->UniformMatrix4f("uView", camera->GetViewMatrix());
			m_pGenIrr->UniformMatrix4f("uModel", glm::identity<glm::mat4>());
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
		BSF_DIAGNOSTIC_FUNC();
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
			Colors::White
		});
	}

}