#include "Renderer2D.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "Log.h"

#include <glad/glad.h>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

static constexpr uint32_t s_MaxTriangleVertices = 30000;
static constexpr uint32_t s_MaxTextureUnits = 32;

#pragma region Shaders Code

static const std::string s_VertexSource = R"VERTEX(

	#version 330

	uniform mat4 uProjection;

	layout(location = 0) in vec2 aPosition;
	layout(location = 1) in vec2 aUv;
	layout(location = 2) in vec4 aColor;
	layout(location = 3) in uint aTexture;

	out vec2 fUv;
	out vec4 fColor;
	flat out uint fTexture;

	void main() {
		gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
		fUv = aUv;
		fColor = aColor;
		fTexture = aTexture;
	}	

)VERTEX";

static const std::string s_FragmentSource = R"FRAGMENT(
	
	#version 330

	uniform sampler2D uTextures[32];

	in vec2 fUv;
	in vec4 fColor;
	flat in uint fTexture;

	out vec4 oColor;

	void main() {
		oColor = fColor * texture(uTextures[fTexture], fUv);	
	}	

)FRAGMENT";
#pragma endregion

namespace bsf
{

	Renderer2D::Renderer2D() :
		m_TriangleVertices(nullptr),
		m_TriangleVa(0),
		m_TriangleVb(0),
		m_CurTriangleIndex(0),
		m_Projection(glm::identity<glm::mat4>())
	{
		Initialize();
	}

	Renderer2D::~Renderer2D()
	{
		delete[] m_TriangleVertices;

		glDeleteBuffers(1, &m_TriangleVb);
		glDeleteVertexArrays(1, &m_TriangleVa);

	}

	void Renderer2D::Initialize()
	{

		// Init Vertex Arrays

		m_TriangleVertices = new Vertex[s_MaxTriangleVertices];

		BSF_GLCALL(glGenVertexArrays(1, &m_TriangleVa));
		BSF_GLCALL(glGenBuffers(1, &m_TriangleVb));

		BSF_GLCALL(glBindVertexArray(m_TriangleVa));
		BSF_GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_TriangleVb));
		BSF_GLCALL(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * s_MaxTriangleVertices, (void*)0, GL_DYNAMIC_DRAW));

		BSF_GLCALL(glEnableVertexAttribArray(0));
		BSF_GLCALL(glEnableVertexAttribArray(1));
		BSF_GLCALL(glEnableVertexAttribArray(2));
		BSF_GLCALL(glEnableVertexAttribArray(3));

		BSF_GLCALL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)0));
		BSF_GLCALL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, sizeof(Vertex), (const void*)8));
		BSF_GLCALL(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)16));
		BSF_GLCALL(glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(Vertex), (const void*)32));


		// Init Shaders

		m_TriangleProgram = MakeRef<ShaderProgram>(s_VertexSource, s_FragmentSource);

		// Init white texture;

		m_Textures.resize(s_MaxTextureUnits);

		uint32_t data = 0xffffffff;
		WhiteTexture = MakeRef<::bsf::Texture>(1, 1, 1.0f, &data);

	}

	void Renderer2D::Begin(const glm::mat4& projection)
	{
		// Reset state stack
		m_State = std::stack<Renderer2DState>();
		m_State.push({});

		// Projection matrix
		m_Projection = projection;
	}

	void Renderer2D::DrawQuadInternal(const glm::vec2& position, const glm::vec2& size, const glm::vec2& pivot, const Ref<::bsf::Texture>& texture)
	{

		std::array<glm::vec2, 4> positions = {
			position - pivot * size,
			position - pivot * size + glm::vec2(size.x, 0.0f),
			position - pivot * size + glm::vec2(size.x, size.y),
			position - pivot * size + glm::vec2(0.0f, size.y),
		};


		std::array<glm::vec2, 4> uvs = {
			glm::vec2{ 0.0f, 0.0f },
			glm::vec2{ 1.0f, 0.0f },
			glm::vec2{ 1.0f, 1.0f },
			glm::vec2{ 0.0f, 1.0f }
		};

		DrawTriangleInternal({ positions[0], positions[1], positions[2] }, { uvs[0], uvs[1], uvs[2] }, texture);
		DrawTriangleInternal({ positions[0], positions[2], positions[3] }, { uvs[0], uvs[2], uvs[3] }, texture);

	}


	void Renderer2D::DrawTriangleInternal(const std::array<glm::vec2, 3>& positions, const std::array<glm::vec2, 3>& uvs, const Ref<::bsf::Texture>& texture)
	{

		const auto& state = m_State.top();

		if (m_CurTriangleIndex == s_MaxTriangleVertices)
			End();

		uint32_t textureID = WhiteTexture->GetID();

		if (texture != nullptr)
		{
			uint32_t texIndex;

			for (texIndex = 0; texIndex < m_Textures.size(); texIndex++)
			{
				if (m_Textures[texIndex] == texture->GetID()) // The texture is cached 
				{
					break;
				}
			}

			if (texIndex == s_MaxTextureUnits) // Too many textures -> Need to flush
			{
				End();
				texIndex = 0;
			}

			textureID = m_Textures[texIndex];

		}


		std::array<Vertex, 3> transformed = { };

		for (size_t i = 0; i < positions.size(); i++)
		{
			auto worldPos = state.Matrix * glm::vec4(positions[i].x, positions[i].y, 0.0f, 1.0f);

			transformed[i].Postion = { worldPos.x, worldPos.y };
			transformed[i].UV = uvs[i];
			transformed[i].Color = m_State.top().Color;
			transformed[i].TextureID = textureID;
		}

		std::memcpy((void*)&m_TriangleVertices[m_CurTriangleIndex], (void*)transformed.data(), transformed.size() * sizeof(Vertex));

		m_CurTriangleIndex += transformed.size();

	}

	void Renderer2D::DrawQuad(const glm::vec2& position)
	{
		DrawQuadInternal(position, { 1.0f, 1.0f }, m_State.top().QuadPivot, nullptr);
	}



	void Renderer2D::LoadIdentity()
	{
		m_State.top().Matrix = glm::identity<glm::mat4>();
	}

	void Renderer2D::Scale(const glm::vec2& scale)
	{
		auto& matrix = m_State.top().Matrix;
		matrix = glm::scale(matrix, { scale.x, scale.y, 1.0f });
	}

	void Renderer2D::Translate(const glm::vec2& translate)
	{
		auto& matrix = m_State.top().Matrix;
		matrix = glm::translate(matrix, { translate.x, translate.y, 0.0f });
	}

	void Renderer2D::Rotate(float angle)
	{
		auto& matrix = m_State.top().Matrix;
		matrix = glm::rotate(matrix, angle, { 0.0f, 0.0f, 1.0f });
	}

	void Renderer2D::Texture(const Ref<::bsf::Texture>& texture)
	{
		m_State.top().CurrentTexture = texture;
	}

	void Renderer2D::NoTexture()
	{
		m_State.top().CurrentTexture = nullptr;
	}

	void Renderer2D::QuadPivot(EQuadPivot mode)
	{
		switch (mode)
		{

		case EQuadPivot::Center: QuadPivot({ 0.5f, 0.5f }); break;
		case EQuadPivot::Top: QuadPivot({ 0.5f, 1.0f }); break;
		case EQuadPivot::Bottom: QuadPivot({ 0.5f, 0.0f }); break;

		case EQuadPivot::TopLeft: QuadPivot({ 0.0f, 1.0f }); break;
		case EQuadPivot::Left: QuadPivot({ 0.0f, 0.5f }); break;
		case EQuadPivot::BottomLeft: QuadPivot({ 0.0f, 0.0f }); break;

		case EQuadPivot::TopRight: QuadPivot({ 1.0f, 1.0f }); break;
		case EQuadPivot::Right: QuadPivot({ 1.0f, 0.5f }); break;
		case EQuadPivot::BottomRight: QuadPivot({ 1.0f, 0.0f }); break;
		}
	}

	void Renderer2D::QuadPivot(const glm::vec2& pivot)
	{
		m_State.top().QuadPivot = pivot;
	}


	void Renderer2D::Color(const glm::vec4& color)
	{
		m_State.top().Color = color;
	}


	void Renderer2D::Push()
	{
		m_State.push(m_State.top());
	}

	void Renderer2D::Pop()
	{
		assert(m_State.size() > 1);
		m_State.pop();
	}

	void Renderer2D::End()
	{

		// Flush Triangles
		if (m_CurTriangleIndex > 0)
		{

			m_TriangleProgram->Use();

			m_TriangleProgram->UniformMatrix4f("uProjection", m_Projection);


			for (uint32_t i = 0; i < m_Textures.size(); i++)
			{
				BSF_GLCALL(glActiveTexture(GL_TEXTURE0 + i));
				BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Textures[i]));
			}

			m_TriangleProgram->Uniform1iv("uTextures", m_Textures.size(), m_Textures.data());

			BSF_GLCALL(glBindVertexArray(m_TriangleVa));
			BSF_GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_TriangleVb));
			BSF_GLCALL(glBufferSubData(GL_ARRAY_BUFFER, 0, m_CurTriangleIndex * sizeof(Vertex), (const void*)m_TriangleVertices));
			BSF_GLCALL(glDrawArrays(GL_TRIANGLES, 0, m_CurTriangleIndex));

			m_CurTriangleIndex = 0;
		}

		// Reset Textures
		std::memset(m_Textures.data(), 0, m_Textures.size() * sizeof(uint32_t));
		m_Textures[0] = WhiteTexture->GetID();

	}
}