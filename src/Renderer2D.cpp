#include "BsfPch.h"

#include "Renderer2D.h"
#include "VertexArray.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "Assets.h"
#include "Log.h"
#include "Font.h"
#include "Diagnostic.h"

static constexpr uint32_t s_MaxTriangleVertices = 30000 * 3;
static constexpr uint32_t s_MaxTextureUnits = 32;

#pragma region Shaders Code

static const std::string s_VertexSource = R"VERTEX(

	#version 330

	uniform mat4 uProjection;

	layout(location = 0) in vec2 aPosition;
	layout(location = 1) in vec2 aUv;
	layout(location = 2) in vec4 aColor;
	layout(location = 3) in uint aTexture;
	layout(location = 4) in uint aClip;
	layout(location = 5) in vec4 aClipPlanes;

	out vec2 fPosition;
	out vec2 fUv;
	out vec4 fColor;
	flat out uint fTexture;
	flat out uint fClip;
	flat out vec4 fClipPlanes;

	void main() {
		gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
		fPosition = aPosition;
		fUv = aUv;
		fColor = aColor;
		fTexture = aTexture;
		fClip = aClip;
		fClipPlanes = aClipPlanes;
	}

)VERTEX";

static const std::string s_FragmentSource = R"FRAGMENT(

	#version 330

	uniform sampler2D uTextures[32];

	in vec2 fPosition;
	in vec2 fUv;
	in vec4 fColor;
	flat in uint fTexture;
	flat in uint fClip;
	flat in vec4 fClipPlanes;

	out vec4 oColor;

	void main() {

		if(fClip > 0u && (fPosition.x < fClipPlanes.x || fPosition.x > fClipPlanes.y || fPosition.y < fClipPlanes.z || fPosition.y > fClipPlanes.w))
			discard;
		else {
			vec4 texColor;

			switch(fTexture) {
				case 0u: texColor = texture(uTextures[0], fUv); break;
				case 1u: texColor = texture(uTextures[1], fUv); break;
				case 2u: texColor = texture(uTextures[2], fUv); break;
				case 3u: texColor = texture(uTextures[3], fUv); break;
				case 4u: texColor = texture(uTextures[4], fUv); break;
				case 5u: texColor = texture(uTextures[5], fUv); break;
				case 6u: texColor = texture(uTextures[6], fUv); break;
				case 7u: texColor = texture(uTextures[7], fUv); break;
				case 8u: texColor = texture(uTextures[8], fUv); break;
				case 9u: texColor = texture(uTextures[9], fUv); break;
				case 10u: texColor = texture(uTextures[10], fUv); break;
				case 11u: texColor = texture(uTextures[11], fUv); break;
				case 12u: texColor = texture(uTextures[12], fUv); break;
				case 13u: texColor = texture(uTextures[13], fUv); break;
				case 14u: texColor = texture(uTextures[14], fUv); break;
				case 15u: texColor = texture(uTextures[15], fUv); break;
				case 16u: texColor = texture(uTextures[16], fUv); break;
				case 17u: texColor = texture(uTextures[17], fUv); break;
				case 18u: texColor = texture(uTextures[18], fUv); break;
				case 19u: texColor = texture(uTextures[19], fUv); break;
				case 20u: texColor = texture(uTextures[20], fUv); break;
				case 21u: texColor = texture(uTextures[21], fUv); break;
				case 22u: texColor = texture(uTextures[22], fUv); break;
				case 23u: texColor = texture(uTextures[23], fUv); break;
				case 24u: texColor = texture(uTextures[24], fUv); break;
				case 25u: texColor = texture(uTextures[25], fUv); break;
				case 26u: texColor = texture(uTextures[26], fUv); break;
				case 27u: texColor = texture(uTextures[27], fUv); break;
				case 28u: texColor = texture(uTextures[28], fUv); break;
				case 29u: texColor = texture(uTextures[29], fUv); break;
				case 30u: texColor = texture(uTextures[30], fUv); break;
				case 31u: texColor = texture(uTextures[31], fUv); break;
			}

			oColor = fColor * texColor;
		}
	}

)FRAGMENT";
#pragma endregion

namespace bsf
{

	Renderer2D::Renderer2D() :
		m_TriangleVertices(nullptr),
		m_CurVertexIndex(0),
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

		auto trianglesVb = Ref<VertexBuffer>(new VertexBuffer({
			{ "aPosition", AttributeType::Float2  },
			{ "aUv", AttributeType::Float2  },
			{ "aColor", AttributeType::Float4  },
			{ "aTexture", AttributeType::UInt  },
			{ "aClip", AttributeType::UInt },
			{ "aClipPlanes", AttributeType::Float4},
		}, nullptr, s_MaxTriangleVertices, GL_DYNAMIC_DRAW));

		m_Triangles = Ref<VertexArray>(new VertexArray(s_MaxTriangleVertices, { trianglesVb }));

		m_pTriangleProgram = MakeRef<ShaderProgram>(s_VertexSource, s_FragmentSource);

		m_Textures.resize(s_MaxTextureUnits);

		m_TextureUnits.resize(s_MaxTextureUnits);
		std::iota(m_TextureUnits.begin(), m_TextureUnits.end(), 0);


	}

	void Renderer2D::Begin(const glm::mat4& projection)
	{
		End();

		// Reset state stack
		m_State = std::stack<Renderer2DState>();
		m_State.emplace();

		// Projection matrix
		m_Projection = projection;
	}

	void Renderer2D::DrawTriangleInternal(const std::array<glm::vec2, 3>& positions, const std::array<glm::vec2, 3>& uvs)
	{
		BSF_DIAGNOSTIC_FUNC();

		const auto& state = m_State.top();

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


		Triangle2D transformed;

		for (size_t i = 0; i < positions.size(); i++)
		{
			auto worldPos = state.Matrix * glm::vec4(positions[i].x, positions[i].y, 0.0f, 1.0f);

			transformed[i].Postion = { worldPos.x, worldPos.y };
			transformed[i].UV = uvs[i];
			transformed[i].Color = m_State.top().Color;
			transformed[i].TextureID = textureIndex;

			if (state.Clip.has_value())
			{
				transformed[i].Clip = true;
				transformed[i].ClipPlanes = (glm::vec4)state.Clip.value();
			}
			else
			{
				transformed[i].Clip = false;
			}


		}

		std::memcpy((void*)&m_TriangleVertices[m_CurVertexIndex], (void*)transformed.data(), transformed.size() * sizeof(Vertex2D));
		m_CurVertexIndex += transformed.size();

		if (m_CurVertexIndex == s_MaxTriangleVertices)
			End();

	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec2& uvSize, const glm::vec2& uvOffset)
	{
		BSF_DIAGNOSTIC_FUNC();

		const auto& pivot = m_State.top().Pivot;
		const std::array<glm::vec2, 4> positions = {
			position - pivot * size,
			position - pivot * size + glm::vec2(size.x, 0.0f),
			position - pivot * size + glm::vec2(size.x, size.y),
			position - pivot * size + glm::vec2(0.0f, size.y),
		};


		const std::array<glm::vec2, 4> uvs = {
			glm::vec2{ uvOffset.x + 0.0f,		uvOffset.y + 0.0f		},
			glm::vec2{ uvOffset.x + uvSize.x,	uvOffset.y + 0.0f		},
			glm::vec2{ uvOffset.x + uvSize.x,	uvOffset.y + uvSize.y	},
			glm::vec2{ uvOffset.x + 0.0f,		uvOffset.y + uvSize.y	}
		};

		DrawTriangleInternal({ positions[0], positions[1], positions[2] }, { uvs[0], uvs[1], uvs[2] });
		DrawTriangleInternal({ positions[0], positions[2], positions[3] }, { uvs[0], uvs[2], uvs[3] });

	}

	void Renderer2D::DrawString(const Ref<Font>& font, const FormattedString& str, const glm::vec2& position)
	{
		static std::array<glm::vec2, 4> pos, uvs = {};

		float strWidth = font->GetStringWidth(str.GetPlainText());

		Push();

		Texture(font->GetTexture());

		float offsetX = position.x - m_State.top().Pivot.x * strWidth;
		float offsetY = position.y - m_State.top().Pivot.y;

		for (uint32_t i = 0; i < str.Size(); ++i)
		{
			if (str[i].Color.has_value())
				Color(str[i].Color.value());
			else
				Color(m_State.top().Color);

			const auto& glyph = font->GetGlyphInfo(str[i].Code);

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

		Pop();
	}

	void Renderer2D::DrawStringShadow(const Ref<Font>& font, const FormattedString& str, const glm::vec2& position)
	{

		const auto& state = m_State.top();
		Push();
		Color(state.TextShadowColor);
		// Draw the shadow (no color/formatting)
		DrawString(font, str.GetPlainText(), position + state.TextShadowOffset);
		Pop();

		DrawString(font, str, position);

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

	void Renderer2D::TextShadowColor(const glm::vec4& color)
	{
		m_State.top().TextShadowColor = color;
	}

	void Renderer2D::TextShadowOffset(const glm::vec2& offset)
	{
		m_State.top().TextShadowOffset = offset;
	}

	void Renderer2D::Clip(const Rect& clip)
	{

		if (m_State.top().Clip.has_value())
			m_State.top().Clip = m_State.top().Clip.value().Intersect(clip);
		else
			m_State.top().Clip = clip;

	}

	void Renderer2D::NoClip()
	{
		m_State.top().Clip = std::nullopt;
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
		if (m_CurVertexIndex > 0)
		{
			BSF_DIAGNOSTIC_FUNC();

			GLEnableScope scope({ GL_CULL_FACE, GL_DEPTH_TEST, GL_BLEND });

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


			m_pTriangleProgram->Use();

			m_pTriangleProgram->UniformMatrix4f(HS("uProjection"), m_Projection);

			for (uint32_t i = 0; i < m_Textures.size(); i++)
			{
				BSF_GLCALL(glActiveTexture(GL_TEXTURE0 + i));
				BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Textures[i]));
			}

			m_pTriangleProgram->Uniform1iv(HS("uTextures[0]"), (uint32_t)m_Textures.size(), m_TextureUnits.data());

			m_Triangles->GetVertexBuffer(0)->SetSubData(m_TriangleVertices, 0, m_CurVertexIndex);

			m_Triangles->DrawArrays(GL_TRIANGLES, m_CurVertexIndex);

			m_CurVertexIndex = 0;


		}

		// Reset Textures
		std::memset(m_Textures.data(), 0, m_Textures.size() * sizeof(uint32_t));
		m_Textures[0] = Assets::GetInstance().Get<Texture2D>(AssetName::TexWhite)->GetId();

	}
	FormattedString::FormattedString(const char* ch) : FormattedString(std::string(ch))
	{
	}
	FormattedString::FormattedString(const std::string& str)
	{
		Add(str);
	}
	FormattedString& FormattedString::Color(const glm::vec4& color)
	{
		m_CurrentColor = color;
		return *this;
	}
	FormattedString& FormattedString::ResetColor()
	{
		m_CurrentColor.reset();
		return *this;
	}

	FormattedString& FormattedString::Add(const std::string& str)
	{
		m_PlainText += str;
		for (auto c : str)
			m_Characters.push_back({ c, m_CurrentColor });
		return *this;
	}
}
