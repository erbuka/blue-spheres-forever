#include "GameScene.h"
#include "VertexArray.h"
#include "Common.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "GameLogic.h"
#include "Log.h"

#include <array>
#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

static const std::string s_SkyBoxVertex = R"Vertex(
	#version 330
	
	uniform mat4 uProjection;
	uniform mat4 uModel;
	uniform mat4 uView;

	layout(location = 0) in vec3 aPosition;

	out vec3 fUv;
	
	void main() {
		gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
		fUv = aPosition;
	}	
)Vertex";

static const std::string s_SkyBoxFragment = R"Fragment(
	#version 330
	
	uniform mat4 uRotate;
	uniform samplerCube uMap;
	
	in vec3 fUv;
	out vec4 oColor;	

	void main() {	
		oColor = texture(uMap,  fUv);
	}
)Fragment";

static const std::string s_Vertex = R"Vertex(
	#version 330
	
	uniform mat4 uProjection;
	uniform mat4 uView;
	uniform mat4 uModel;
	
	uniform vec2 uUvOffset;

	layout(location = 0) in vec3 aPosition;
	layout(location = 1) in vec3 aNormal;
	layout(location = 2) in vec2 aUv;

	
	out vec3 fPosition;
	out vec3 fNormal;
	out vec2 fUv;
	
	void main() {
		
		vec3 position = (uView * uModel * vec4(aPosition, 1.0)).xyz;

		gl_Position = uProjection * vec4(position, 1.0);

		fNormal = aNormal;
		fPosition = aPosition;
		fUv = aUv + uUvOffset;
	}	
)Vertex";


static const std::string s_Fragment = R"Fragment(
	#version 330
	
	uniform mat4 uProjection;
	uniform mat4 uView;
	uniform mat4 uModel;
	
	uniform vec3 uCameraPos;
	uniform vec3 uLightDir;

	uniform samplerCube uEnv;
	uniform sampler2D uMap;
	uniform vec4 uColor;
	
	in vec3 fPosition;
	in vec3 fNormal;
	in vec2 fUv;

	out vec4 oColor;

	vec3 fresnelSchlick(float cosTheta, vec3 F0);

	void main() {

		vec3 N = normalize((uModel * vec4(fNormal, 0.0)).xyz);
		vec3 V = normalize(uCameraPos - (uModel * vec4(fPosition, 1.0)).xyz);
		vec3 L = normalize(uLightDir);
		vec3 H = normalize(V + L);
		vec3 R = reflect(-V, N);

		float NdL = max(0.0, dot(N, L));

		float NdH = max(0.0, dot(N, H));
		
		vec3 env = texture(uEnv, R).rgb;
		vec3 albedo = (texture(uMap, fUv) * uColor).xyz;

		vec3 ambient = albedo * 0.1;
		vec3 diffuse = albedo * NdL;

		vec3 color = albedo + env * 0.2;

		oColor = vec4(ambient + color * NdL, 1.0);
	}


	vec3 fresnelSchlick(float cosTheta, vec3 F0)
	{
		return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
	}  

	
)Fragment";


namespace bsf
{
	struct Vertex
	{
		glm::vec3 Position0;
		glm::vec3 Normal0;
		glm::vec2 UV;
	};


	static Ref<VertexArray> CreateIcosphere(float radius, uint32_t recursion)
	{
		const float t = (1.0f + glm::sqrt(5.0f)) / 2.0f;

		auto mid = [](const glm::vec3& a, const glm::vec3& b)
		{	
			return ;
		};

		auto subdivide = [](const std::vector<glm::vec3>& v)
		{
			std::vector<glm::vec3> result;
			result.reserve(v.size() * 4);

			for (uint32_t i = 0; i < v.size(); i += 3)
			{
				const auto& a = v[i + 0];
				const auto& b = v[i + 1];
				const auto& c = v[i + 2];

				auto ab = glm::normalize((a + b) / 2.0f);
				auto bc = glm::normalize((b + c) / 2.0f);
				auto ac = glm::normalize((a + c) / 2.0f);

				result.push_back(a); result.push_back(ab); result.push_back(ac);
				result.push_back(ab); result.push_back(b); result.push_back(bc);
				result.push_back(ab); result.push_back(bc); result.push_back(ac);
				result.push_back(ac); result.push_back(bc); result.push_back(c);

			}

			return result;
		};

		std::array<glm::vec3, 12> v = {
			glm::normalize(glm::vec3(-1, t, 0)),
			glm::normalize(glm::vec3(1, t, 0)),
			glm::normalize(glm::vec3(-1, -t, 0)),
			glm::normalize(glm::vec3(1, -t, 0)),
			glm::normalize(glm::vec3(0, -1, t)),
			glm::normalize(glm::vec3(0, 1, t)),
			glm::normalize(glm::vec3(0, -1, -t)),
			glm::normalize(glm::vec3(0, 1, -t)),
			glm::normalize(glm::vec3(t, 0, -1)),
			glm::normalize(glm::vec3(t, 0, 1)),
			glm::normalize(glm::vec3(-t, 0, -1)),
			glm::normalize(glm::vec3(-t, 0, 1))
		};

		std::vector<glm::vec3> triangles = {
			v[0], v[11], v[5],
			v[0], v[5], v[1],
			v[0], v[1], v[7],
			v[0], v[7], v[10],
			v[0], v[10], v[11],
			v[1], v[5], v[9],
			v[5], v[11], v[4],
			v[11], v[10], v[2],
			v[10], v[7], v[6],
			v[7], v[1], v[8],
			v[3], v[9], v[4],
			v[3], v[4], v[2],
			v[3], v[2], v[6],
			v[3], v[6], v[8],
			v[3], v[8], v[9],
			v[4], v[9], v[5],
			v[2], v[4], v[11],
			v[6], v[2], v[10],
			v[8], v[6], v[7],
			v[9], v[8], v[1]
		};

		while (recursion-- > 0)
			triangles = subdivide(triangles);

		std::vector<Vertex> vertices;
		vertices.reserve(triangles.size());

		for (const auto& t : triangles)
		{
			float u = (std::atan2f(t.z, t.x) / (2.0f * glm::pi<float>()));
			float v = (std::asinf(t.y) / glm::pi<float>()) + 0.5f;
			vertices.push_back({ t * radius, glm::normalize(t), glm::vec2(u, v) });
		}

		Ref<VertexArray> result = Ref<VertexArray>(new VertexArray({
			{ "aPosition", AttributeType::Float3 },
			{ "aNormal", AttributeType::Float3 },
			{ "aUv", AttributeType::Float2 }
		}));

		result->SetData(vertices.data(), vertices.size(), GL_STATIC_DRAW);

		return result;
	}

	static glm::vec3 Project(const glm::vec3& position, glm::vec3& normal)
	{
		constexpr float radius = 10.0f;
		constexpr glm::vec3 center = { 0.0f, 0.0f, -radius };

		float offset = position.z;
		glm::vec3 ground = { position.x, position.y, 0.0f };

		normal = glm::normalize(ground - center);



		return center + normal * (radius + offset);

	}

	static Ref<VertexArray> CreateWorld(int32_t left, int32_t right, int32_t bottom, int32_t top, int32_t divisions)
	{

		std::vector<Vertex> vertices;

		float step = 1.0f / divisions;

		for (int32_t x = left; x < right; x++)
		{
			for (int32_t y = bottom; y < top; y++)
			{
				for (int32_t dx = 0; dx < divisions; dx++)
				{
					for (int32_t dy = 0; dy < divisions; dy++)
					{
						float x0 = x + dx * step, y0 = y + dy * step;
						float x1 = x + (dx + 1) * step, y1 = y + (dy + 1) * step;
						float u0 = (x % 2) * 0.5f + dx * step * 0.5f, v0 = (y % 2) * 0.5f + dy * step * 0.5f;
						float u1 = (x % 2) * 0.5f + (dx + 1) * step * 0.5f, v1 = (y % 2) * 0.5f + (dy + 1) * step * 0.5f;
						
						std::array<glm::vec3, 4> normals;

						std::array<Vertex, 4> v = {
							Vertex{ Project(glm::vec3(x0, y0, 0.0f), normals[0]), normals[0], glm::vec2(u0, v0) },
							Vertex{ Project(glm::vec3(x1, y0, 0.0f), normals[1]), normals[1], glm::vec2(u1, v0) },
							Vertex{ Project(glm::vec3(x1, y1, 0.0f), normals[2]), normals[2], glm::vec2(u1, v1) },
							Vertex{ Project(glm::vec3(x0, y1, 0.0f), normals[3]), normals[3], glm::vec2(u0, v1) }
						};

						vertices.push_back(v[0]); vertices.push_back(v[1]); vertices.push_back(v[2]);
						vertices.push_back(v[0]); vertices.push_back(v[2]); vertices.push_back(v[3]);
					}
				}
			}
		}

		Ref<VertexArray> result = Ref<VertexArray>(new VertexArray({
			{ "aPosition", AttributeType::Float3 },
			{ "aNormal", AttributeType::Float3 },
			{ "aUv", AttributeType::Float2 }
		}));

		result->SetData(vertices.data(), vertices.size(), GL_STATIC_DRAW);

		return result;

	}

	static Ref<VertexArray> CreateSkyBox()
	{

		std::array<glm::vec3, 8> v = {
			// Front
			glm::vec3(-1.0f, -1.0f, +1.0f),
			glm::vec3(+1.0f, -1.0f, +1.0f),
			glm::vec3(+1.0f, +1.0f, +1.0f),
			glm::vec3(-1.0f, +1.0f, +1.0f),

			// Back
			glm::vec3(-1.0f, -1.0f, -1.0f),
			glm::vec3(+1.0f, -1.0f, -1.0f),
			glm::vec3(+1.0f, +1.0f, -1.0f),
			glm::vec3(-1.0f, +1.0f, -1.0f),

		};

		std::vector<glm::vec3> verts;
		verts.reserve(36);

		// Front
		verts.push_back(v[0]); verts.push_back(v[2]); verts.push_back(v[1]);
		verts.push_back(v[0]); verts.push_back(v[3]); verts.push_back(v[2]);

		// Back
		verts.push_back(v[4]); verts.push_back(v[5]); verts.push_back(v[6]);
		verts.push_back(v[4]); verts.push_back(v[6]); verts.push_back(v[7]);

		// Left
		verts.push_back(v[0]); verts.push_back(v[4]); verts.push_back(v[7]);
		verts.push_back(v[0]); verts.push_back(v[7]); verts.push_back(v[3]);

		// Right
		verts.push_back(v[1]); verts.push_back(v[6]); verts.push_back(v[5]);
		verts.push_back(v[1]); verts.push_back(v[2]); verts.push_back(v[6]);

		// Top
		verts.push_back(v[3]); verts.push_back(v[6]); verts.push_back(v[2]);
		verts.push_back(v[3]); verts.push_back(v[7]); verts.push_back(v[6]);

		// Bottom
		verts.push_back(v[0]); verts.push_back(v[1]); verts.push_back(v[5]);
		verts.push_back(v[0]); verts.push_back(v[5]); verts.push_back(v[4]);


		Ref<VertexArray> result(new VertexArray({
			{ "aPosition", AttributeType::Float3 }
		}));

		result->SetData(verts.data(), verts.size(), GL_STATIC_DRAW);

		return result;
 
	}


	void GameScene::OnAttach(Application& app)
	{
		m_Subscriptions.push_back(app.WindowResized.Subscribe(this, &GameScene::OnResize));

		app.KeyPressed.Subscribe([&](const KeyPressedEvent& evt) {
			if (evt.KeyCode == GLFW_KEY_LEFT)
			{
				m_GameLogic->Rotate(GameLogic::ERotate::Left);
			}
			else if (evt.KeyCode == GLFW_KEY_RIGHT)
			{
				m_GameLogic->Rotate(GameLogic::ERotate::Right);
			}
			else if (evt.KeyCode == GLFW_KEY_UP)
			{
				m_GameLogic->RunForward();
			}
			else if (evt.KeyCode == GLFW_KEY_SPACE)
			{
				m_GameLogic->Jump();
			}
		});

		m_Stage = Stage::FromFile("assets/data/playground.bss");
		m_GameLogic = MakeRef<GameLogic>(m_Stage);

		m_World = CreateWorld(-5, 5, -5, 5, 10);
		m_Sphere = CreateIcosphere(0.15, 3);

		m_Program = MakeRef<ShaderProgram>(s_Vertex, s_Fragment);

		{
			m_Map = CreateCheckerBoard({ 0xff0088ff, 0xff88ff00 });
			m_Map->Filter(TextureFilter::MagFilter, TextureFilterMode::Nearest);
			m_Map->Filter(TextureFilter::MinFilter, TextureFilterMode::Linear);
		}

		{
			int32_t white = 0xffffffff;
			m_White = MakeRef<Texture2D>(1, 1, &white);
			m_White->Filter(TextureFilter::MinFilter, TextureFilterMode::Nearest);
			m_White->Filter(TextureFilter::MagFilter, TextureFilterMode::Nearest);
		}

		{
			m_Bumper = MakeRef<Texture2D>("assets/textures/bumper.png");
			m_Bumper->Filter(TextureFilter::MinFilter, TextureFilterMode::LinearMipmapLinear);
			m_Bumper->Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
		}

		// SkyBox

		m_SkyBox = CreateSkyBox();

		{
			
			m_CubeMap = MakeRef<TextureCube>(
				"assets/textures/front.png",
				"assets/textures/back.png",
				"assets/textures/left.png",
				"assets/textures/right.png",
				"assets/textures/bottom.png",
				"assets/textures/top.png");
			m_CubeMap->Filter(TextureFilter::MagFilter, TextureFilterMode::Linear);
			m_CubeMap->Filter(TextureFilter::MinFilter, TextureFilterMode::Linear);

		}

		m_SkyBoxProgram = MakeRef<ShaderProgram>(s_SkyBoxVertex, s_SkyBoxFragment);


	}
	
	void GameScene::OnRender(Application& app, const Time& time)
	{
		auto windowSize = app.GetWindowSize();
		float aspect = windowSize.x / windowSize.y;


		//glPolygonMode(GL_FRONT, GL_LINE);

		glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 blue(0.0f, 0.0f, 1.0f, 1.0f);
		glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
		glm::vec4 yellow(1.0f, 1.0f, 0.0f, 1.0f);

		const int steps = 5;
		for (int i = 0; i < steps; i++)
			m_GameLogic->Advance({ time.Delta / steps, time.Elapsed });

		glEnable(GL_FRAMEBUFFER_SRGB);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		//glCullFace(GL_BACK);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::vec2 pos = m_GameLogic->GetPosition();
		int32_t ix = pos.x, iy = pos.y;
		float fx = pos.x - ix, fy = pos.y - iy;
		glm::vec3 normal;
		
		m_Projection.LoadIdentity();
		m_Projection.Perspective(glm::pi<float>() / 4.0f, aspect, 0.1f, 1000.0f);

		m_Model.LoadIdentity();
		m_View.LoadIdentity();
		
		// Draw Skybox
		{

			m_CubeMap->Bind(0);
			glDepthMask(GL_FALSE);
			m_View.Push();
			m_View.Rotate({ 0.0f, 1.0f, 0.0f }, -m_GameLogic->GetRotationAngle());
			m_SkyBoxProgram->Use();
			m_SkyBoxProgram->Uniform1i("uMap", { 0 });
			m_SkyBoxProgram->UniformMatrix4f("uProjection", m_Projection.GetMatrix());
			m_SkyBoxProgram->UniformMatrix4f("uView", m_View.GetMatrix());
			m_SkyBoxProgram->UniformMatrix4f("uModel", m_Model.GetMatrix());
			m_SkyBox->Draw(GL_TRIANGLES);
			m_View.Pop();
			glDepthMask(GL_TRUE);
		}

		// Setup the player view
		m_View.LookAt({ -3.0f, 2.0f, 0.0f }, { 0.0f, 0.0, 0.0f }, { 0.0f, 1.0f, 0.0f });
		m_View.Rotate({ 0.0f, 1.0f, 0.0f }, -m_GameLogic->GetRotationAngle());

		m_Model.Rotate({ 1.0f, 0.0f, 0.0f }, -glm::pi<float>() / 2.0f);

		glm::vec3 cameraPosition = glm::inverse(m_View.GetMatrix()) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec3 lightVector = { 0.0f, 1.0f, 0.0f };

		// Draw ground
		m_Map->Bind(0);
		m_CubeMap->Bind(1);

		m_Program->Use();

		m_Program->UniformMatrix4f("uProjection", m_Projection.GetMatrix());
		m_Program->UniformMatrix4f("uView", m_View.GetMatrix());
		m_Program->UniformMatrix4f("uModel", m_Model.GetMatrix());

		m_Program->Uniform3fv("uCameraPos", 1, glm::value_ptr(cameraPosition));
		m_Program->Uniform3fv("uLightDir", 1, glm::value_ptr(lightVector));

		m_Program->Uniform1i("uMap", { 0 });
		m_Program->Uniform1i("uEnv", { 1 });

		m_Program->Uniform4fv("uColor", 1, glm::value_ptr(white));
		m_Program->Uniform2f("uUvOffset", { (ix % 2) * 0.5f +  fx * 0.5f, (iy % 2) * 0.5f + fy * 0.5f });
		
		m_World->Draw(GL_TRIANGLES);

		m_Program->Uniform2f("uUvOffset", { 0, 0 });

		// Draw player
		m_Model.Push();
		m_Model.Translate({ 0.0f, 0.0f, 0.15f + m_GameLogic->GetHeight() });
		m_Program->UniformMatrix4f("uModel", m_Model.GetMatrix());
		m_White->Bind(0);
		m_Program->Uniform4fv("uColor", 1, glm::value_ptr(white));
		m_Sphere->Draw(GL_TRIANGLES);
		m_Model.Pop();


		// Draw spheres and rings
		for (int32_t x = -10; x < 10; x++)
		{
			for (int32_t y = -10; y < 10; y++)
			{
				auto val = m_Stage.GetValueAt(x + ix, y + iy);

				if (val != EStageObject::None)
				{
					m_Model.Push();
					m_Model.Translate(Project({ x - fx, y - fy, 0.15f }, normal));
					m_Program->UniformMatrix4f("uModel", m_Model.GetMatrix());
					m_Program->Uniform1i("uMap", { 0 });

					
					switch (val)
					{
					case EStageObject::RedSphere: 
						m_White->Bind(0);
						m_Program->Uniform4fv("uColor", 1, glm::value_ptr(red));
						m_Sphere->Draw(GL_TRIANGLES);
						break;
					case EStageObject::BlueSphere:
						m_White->Bind(0);
						m_Program->Uniform4fv("uColor", 1, glm::value_ptr(blue));
						m_Sphere->Draw(GL_TRIANGLES);
						break;
					case EStageObject::YellowSphere:
						m_White->Bind(0);
						m_Program->Uniform4fv("uColor", 1, glm::value_ptr(yellow));
						m_Sphere->Draw(GL_TRIANGLES);
						break;
					case EStageObject::StarSphere:
						m_Bumper->Bind(0);
						m_Program->Uniform4fv("uColor", 1, glm::value_ptr(white));
						m_Sphere->Draw(GL_TRIANGLES);
						break;
					}

					m_Model.Pop();
				}

			}
		}

	}
	
	void GameScene::OnDetach(Application& app)
	{
		for (auto& unsub : m_Subscriptions)
			unsub();
	}

	void GameScene::OnResize(const WindowResizedEvent& evt)
	{
		glViewport(0, 0, evt.Width, evt.Height);
	}
}