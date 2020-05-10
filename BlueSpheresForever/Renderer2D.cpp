#include "BsfPch.h"

#include "Renderer2D.h"
#include "VertexArray.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "Assets.h"
#include "Log.h"
#include "Font.h"




static constexpr uint32_t s_MaxTriangleVertices = 30000;
static constexpr uint32_t s_MaxTextureUnits = 32;

#pragma region Shaders Code

static const std::string s_VertexSource = R"VERTEX(

	#version 330 core

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
	
	#version 330 core

	uniform sampler2D uTextures[32];

	in vec2 fUv;
	in vec4 fColor;
	flat in uint fTexture;

	out vec4 oColor;

	void main() {
		oColor = fColor * texture(uTextures[fTexture], fUv);
		//oColor = vec4(fUv, 0.0, 1.0);
	}	

)FRAGMENT";
#pragma endregion

namespace bsf
{

	Renderer2D::Renderer2D() :
		m_TriangleVertices(nullptr),
		m_CurTriangleIndex(0),
		m_Projection(glm::identity<glm::mat4>())
	{
		Initialize();
	}

	Renderer2D::~Renderer2D()
	{
		delete[] m_TriangleVertices;
	}

	void Renderer2D::Initialize()
	{

		// Init Vertex Arrays
		m_TriangleVertices = new Vertex2D[s_MaxTriangleVertices];

		m_Triangles = Ref<VertexArray>(new VertexArray({
			{ "aPosition", AttributeType::Float2  },
			{ "aUv", AttributeType::Float2  },
			{ "aColor", AttributeType::Float4  },
			{ "aTexture", AttributeType::UInt  },
		}));
		m_Triangles->SetData(nullptr, s_MaxTriangleVertices, GL_DYNAMIC_DRAW);

		// Init Shaders

		m_TriangleProgram = MakeRef<ShaderProgram>(s_VertexSource, s_FragmentSource);

		// Init white texture;
		m_WhiteTexture = Assets::Get().GetTexture(AssetName::TexWhite);

		m_Textures.resize(s_MaxTextureUnits);
		std::memset(m_Textures.data(), 0, m_Textures.size() * sizeof(uint32_t));
		m_Textures[0] = m_WhiteTexture->GetId();

		m_TextureUnits.resize(s_MaxTextureUnits);
		std::iota(m_TextureUnits.begin(), m_TextureUnits.end(), 0);


	}

	void Renderer2D::Begin(const glm::mat4& projection)
	{
		// Reset state stack
		m_State = std::stack<Renderer2DState>();
		m_State.push({});

		// Projection matrix
		m_Projection = projection;
	}

	void Renderer2D::DrawQuadInternal(const glm::vec2& position, const glm::vec2& size, const glm::vec2& pivot)
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

		DrawTriangleInternal({ positions[0], positions[1], positions[2] }, { uvs[0], uvs[1], uvs[2] });
		DrawTriangleInternal({ positions[0], positions[2], positions[3] }, { uvs[0], uvs[2], uvs[3] });

	}


	void Renderer2D::DrawTriangleInternal(const std::array<glm::vec2, 3>& positions, const std::array<glm::vec2, 3>& uvs)
	{

		const auto& state = m_State.top();

		if (m_CurTriangleIndex == s_MaxTriangleVertices)
			End();

		uint32_t textureIndex = 0;

		if (state.CurrentTexture != nullptr)
		{

			auto cached = std::find(m_Textures.begin(), m_Textures.end(), state.CurrentTexture->GetId());

			if (cached == m_Textures.end()) // texture not cached, try to find empty slot
			{
				auto tex = std::find(m_Textures.begin(), m_Textures.end(), 0);

				if (tex == m_Textures.end()) // Too many textures, need to flush
				{
					End();
					tex = m_Textures.begin() + 1;
				}

				*tex = state.CurrentTexture->GetId();

				cached = tex;

			}

			textureIndex = cached - m_Textures.begin();

		}


		std::array<Vertex2D, 3> transformed = { };

		for (size_t i = 0; i < positions.size(); i++)
		{
			auto worldPos = state.Matrix * glm::vec4(positions[i].x, positions[i].y, 0.0f, 1.0f);

			transformed[i].Postion = { worldPos.x, worldPos.y };
			transformed[i].UV = uvs[i];
			transformed[i].Color = m_State.top().Color;
			transformed[i].TextureID = textureIndex;
		}

		std::memcpy((void*)&m_TriangleVertices[m_CurTriangleIndex], (void*)transformed.data(), transformed.size() * sizeof(Vertex2D));

		m_CurTriangleIndex += transformed.size();

	}

	void Renderer2D::DrawQuad(const glm::vec2& position)
	{
		DrawQuadInternal(position, { 1.0f, 1.0f }, m_State.top().Pivot);
	}

	void Renderer2D::DrawString(const Ref<Font>& font, const std::string& text)
	{
		static std::array<glm::vec2, 4> pos, uvs = {};

		float strWidth = font->GetStringWidth(text);

		Texture(font->GetTexture());

		float offsetX = -m_State.top().Pivot.x * strWidth;
		float offsetY = m_State.top().Pivot.y;

		for (auto c : text)
		{
			const auto& glyph = font->GetGlyphInfo(c);

			pos[0] = { offsetX + glyph.Min.x, offsetY + glyph.Min.y };
			pos[1] = { offsetX + glyph.Max.x, offsetY + glyph.Min.y };
			pos[2] = { offsetX + glyph.Max.x, offsetY + glyph.Max.y };
			pos[3] = { offsetX + glyph.Min.x, offsetY + glyph.Max.y };

			
			uvs[0] = { glyph.UvMin.x, glyph.UvMin.y };
			uvs[1] = { glyph.UvMax.x, glyph.UvMin.y };
			uvs[2] = { glyph.UvMax.x, glyph.UvMax.y };
			uvs[3] = { glyph.UvMin.x, glyph.UvMax.y };
			

			DrawTriangleInternal({ pos[0], pos[1], pos[2] }, { uvs[0], uvs[1], uvs[2] });
			DrawTriangleInternal({ pos[0], pos[2], pos[3] }, { uvs[0], uvs[2], uvs[3] });

			offsetX += glyph.Advance;

		}
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

	void Renderer2D::Texture(const Ref<::bsf::Texture2D>& texture)
	{
		m_State.top().CurrentTexture = texture;
	}

	void Renderer2D::NoTexture()
	{
		m_State.top().CurrentTexture = nullptr;
	}

	void Renderer2D::Pivot(EPivot mode)
	{
		switch (mode)
		{

		case EPivot::Center: Pivot({ 0.5f, 0.5f }); break;
		case EPivot::Top: Pivot({ 0.5f, 1.0f }); break;
		case EPivot::Bottom: Pivot({ 0.5f, 0.0f }); break;

		case EPivot::TopLeft: Pivot({ 0.0f, 1.0f }); break;
		case EPivot::Left: Pivot({ 0.0f, 0.5f }); break;
		case EPivot::BottomLeft: Pivot({ 0.0f, 0.0f }); break;

		case EPivot::TopRight: Pivot({ 1.0f, 1.0f }); break;
		case EPivot::Right: Pivot({ 1.0f, 0.5f }); break;
		case EPivot::BottomRight: Pivot({ 1.0f, 0.0f }); break;
		}
	}

	void Renderer2D::Pivot(const glm::vec2& pivot)
	{
		m_State.top().Pivot = pivot;
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
			GLEnableScope scope({ GL_CULL_FACE, GL_DEPTH_TEST, GL_BLEND });

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			m_TriangleProgram->Use();

			m_TriangleProgram->UniformMatrix4f("uProjection", m_Projection);
			
			for (uint32_t i = 0; i < m_Textures.size(); i++)
			{
				BSF_GLCALL(glActiveTexture(GL_TEXTURE0 + i));
				BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Textures[i]));
			}
			
			m_TriangleProgram->Uniform1iv("uTextures", m_Textures.size(), m_TextureUnits.data());
			
			m_Triangles->SetSubData(m_TriangleVertices, 0, m_CurTriangleIndex);

			m_Triangles->Draw(GL_TRIANGLES, m_CurTriangleIndex);

			m_CurTriangleIndex = 0;
		}

		// Reset Textures
		std::memset(m_Textures.data(), 0, m_Textures.size() * sizeof(uint32_t));
		m_Textures[0] = m_WhiteTexture->GetId();

	}
}