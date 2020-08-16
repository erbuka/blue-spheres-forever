#include "BsfPch.h"

#include "RTXTestScene.h"
#include "ShaderProgram.h"
#include "Application.h"
#include "Framebuffer.h"
#include "VertexArray.h"
#include "Texture.h"



static const std::string s_vsQuad = R"(
	#version 330 core
	
	layout(location = 0) in vec2 aPosition;
	
	out vec2 fPosition;
	out vec2 fUv;	

	void main() {
		gl_Position = vec4(aPosition, 0.0, 1.0);
		fPosition = aPosition;
		fUv = aPosition * 0.5 + 0.5;
	}
	
)";


static const std::string s_fsQuad = R"(
	#version 330 core
	
	uniform sampler2D uTexture;	
	uniform vec2 uTexelSize;

	in vec2 fPosition;	
	in vec2 fUv;
	
	out vec3 oColor;
	
	void main() {
			
		
		vec3 color = vec3(0.0);
	
		for(int x = -1; x <= 1; x++) {
			for(int y = -1; y <= 1; y++) {
				color += texture(uTexture, fUv + vec2(x, y) * uTexelSize).rgb;
			}
		}	

		oColor = color / 9.0;
	}

)";


static const std::string s_fsRTX = R"(
	#version 330 core

	uniform mat4 uView;	

	uniform vec2 uScreenSize;	
	
	uniform int uSphereCount;
	uniform vec3 uSpherePositions[512];


	in vec2 fPosition;	
	in vec2 fUv;

	out vec3 oColor;
	
	const float c_Fov2 = 3.141592 / 8.0;

	vec3 Raytrace(in vec3 ray) {
		vec3 color = vec3(1.0);

		int hit = 0;

		float a = dot(ray, ray);
		
		for(int i = 0; i < uSphereCount; i++) {
			vec3 C = uSpherePositions[i];
			float b = 2 * dot(ray, -C);
			float c = dot(-C,-C) - 1.0;
			
			float delta = b*b - 4.0 * a * c;
				
			hit += delta >= 0.0 ? 1 : 0;
			
		}

		color = hit > 0 ? vec3(1.0) : vec3(0.0);

		return color;
	}

	void main() {
		float aspect = uScreenSize.x / uScreenSize.y;
		float fovX2 = c_Fov2 * aspect;
		float y = sin(fPosition.y * c_Fov2);
		float x = sin(fPosition.x * fovX2);
		
		vec3 ray = normalize(vec3(x, y, -1.0));


		oColor = Raytrace(ray);

	
	}	

)";

namespace bsf
{


	void RTXTestScene::OnAttach()
	{
		auto windowSize = GetApplication().GetWindowSize();

		GetApplication().WindowResized.Subscribe(this, &RTXTestScene::OnResize);

		m_pQuad = MakeRef<ShaderProgram>(s_vsQuad, s_fsQuad);
		m_pRtx = MakeRef<ShaderProgram>(s_vsQuad, s_fsRTX);

		m_fbRTX = MakeRef<Framebuffer>(windowSize.x, windowSize.y, false);
		m_fbRTX->CreateColorAttachment("color", GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);
		m_fbRTX->GetColorAttachment("color")->SetFilter(TextureFilter::Linear, TextureFilter::Linear);

		{
			std::array<glm::vec2, 6> vertices = {
				glm::vec2(-1, -1),
				glm::vec2(1, -1),
				glm::vec2(1, 1),

				glm::vec2(-1, -1),
				glm::vec2(1, 1),
				glm::vec2(-1, 1)
			};

			auto vb = Ref<VertexBuffer>(new VertexBuffer({
				{ "aPosition", AttributeType::Float2 }
			}, vertices.data(), vertices.size()));

			m_vaQuad = Ref<VertexArray>(new VertexArray(vertices.size(), { vb }));
		}

		{
			constexpr float minDist = 30.0f;
			constexpr float maxDist = 70.0f;
			constexpr uint32_t count = 100;

			for (uint32_t i = 0; i < count; i++)
			{
				float dist = glm::linearRand(minDist, maxDist);
				glm::vec3 pos = glm::ballRand(dist);
				m_SpherePositions.push_back(std::move(pos));
			}


		}

	}

	void RTXTestScene::OnRender(const Time& time)
	{
		auto windowSize = GetApplication().GetWindowSize();


		m_fbRTX->Bind();
		{
			glViewport(0, 0, windowSize.x, windowSize.y);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			m_View.Reset();
			m_View.LookAt({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f });


			m_pRtx->Use();
			m_pRtx->UniformMatrix4f("uView", m_View);
			m_pRtx->Uniform2f("uScreenSize", { windowSize.x, windowSize.y });
			m_pRtx->Uniform1i("uSphereCount", { (int32_t)m_SpherePositions.size() });
			m_pRtx->Uniform3fv("uSpherePositions", m_SpherePositions.size(), (float*)m_SpherePositions.data());
			m_vaQuad->Draw(GL_TRIANGLES);

		}
		m_fbRTX->Unbind();


		glViewport(0, 0, windowSize.x, windowSize.y);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		m_pQuad->Use();
		m_pQuad->UniformTexture("uTexture", m_fbRTX->GetColorAttachment("color"));
		m_pQuad->Uniform2f("uTexelSize", { 1.0f / (windowSize.x / 4.0f), 1.0f / (windowSize.y / 4.0f) });
		m_vaQuad->Draw(GL_TRIANGLES);
	}

	void RTXTestScene::OnDetach()
	{
	}

	void RTXTestScene::OnResize(const WindowResizedEvent& evt)
	{
		m_fbRTX->Resize(evt.Width / 4, evt.Height / 4);
	}
}