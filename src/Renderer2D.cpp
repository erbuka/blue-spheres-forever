#include "BsfPch.h"


#include <glm/gtx/perpendicular.hpp>

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
		#ifdef DEBUG
		oColor = vec4(0.0, 0.0, 0.0, 1.0);
		#else
		oColor = fColor * texture(uTextures[fTexture], fUv);
		#endif
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

	

		auto trianglesVb = Ref<VertexBuffer>(new VertexBuffer({
			{ "aPosition", AttributeType::Float2  },
			{ "aUv", AttributeType::Float2  },
			{ "aColor", AttributeType::Float4  },
			{ "aTexture", AttributeType::UInt  },
		}, nullptr, s_MaxTriangleVertices, GL_DYNAMIC_DRAW));

		m_Triangles = Ref<VertexArray>(new VertexArray(s_MaxTriangleVertices, { trianglesVb }));

		// Init Shaders

		m_TriangleProgram = MakeRef<ShaderProgram>(s_VertexSource, s_FragmentSource);

		m_Textures.resize(s_MaxTextureUnits);

		m_TextureUnits.resize(s_MaxTextureUnits);
		std::iota(m_TextureUnits.begin(), m_TextureUnits.end(), 0);


	}

	void Renderer2D::Begin(const glm::mat4& projection)
	{
		End();

		// Reset state stack
		m_State = std::stack<Renderer2DState>();
		m_State.push({});

		// Projection matrix
		m_Projection = projection;
	}

	void Renderer2D::DrawTriangleInternal(const std::array<glm::vec2, 3>& positions, const std::array<glm::vec2, 3>& uvs)
	{

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


		Triangle2D transformed = { };

		for (size_t i = 0; i < positions.size(); i++)
		{
			auto worldPos = state.Matrix * glm::vec4(positions[i].x, positions[i].y, 0.0f, 1.0f);

			transformed[i].Postion = { worldPos.x, worldPos.y };
			transformed[i].UV = uvs[i];
			transformed[i].Color = m_State.top().Color;
			transformed[i].TextureID = textureIndex;
		}

		std::vector<Triangle2D> triangles;

		if (state.Clip.has_value())
			triangles = ClipTriangle(transformed);
		else
			triangles.push_back(transformed);


		for (const auto& triangle : triangles)
		{
			std::memcpy((void*)&m_TriangleVertices[m_CurTriangleIndex], (void*)triangle.data(), triangle.size() * sizeof(Vertex2D));
			m_CurTriangleIndex += triangle.size();

			if (m_CurTriangleIndex == s_MaxTriangleVertices)
				End();

		}


	}

	std::pair<float, float> Renderer2D::IntersectLines(const std::array<glm::vec2, 2>& l0, const std::array<glm::vec2, 2>& l1)
	{
		const auto& a = l0[0];
		const auto& b = l0[1];
		const auto& c = l1[0];
		const auto& d = l1[1];

		glm::mat2 A = {
			b.x - a.x, b.y - a.y,
			c.x - d.x, c.y - d.y
		};

		glm::vec2 B = { c.x - a.x, c.y - a.y };

		auto st = glm::inverse(A) * B;

		return { st.x, st.y };

	}

	std::vector<Renderer2D::Triangle2D> Renderer2D::ClipTriangle(const Triangle2D& input)
	{
		const auto& clipRect = m_State.top().Clip.value();

		// Create the points that defines the clipping planes (ccw order)
		const std::array<glm::vec2, 4> points = {
			glm::vec2(clipRect.Left(), clipRect.Bottom()),
			glm::vec2(clipRect.Right(), clipRect.Bottom()),
			glm::vec2(clipRect.Right(), clipRect.Top()),
			glm::vec2(clipRect.Left(), clipRect.Top()),
		};

		std::vector<Triangle2D> triangles; // final result
		std::vector<Triangle2D> tris2; // assembled triangles

		// Start with the initial triangle
		triangles.push_back(input);

		std::array<std::pair<float, float>, 3> intersections;

		std::array<Vertex2D, 4> assembleList;

		// We consider one plane at time
		for (size_t i = 0; i < points.size(); i++)
		{

			// Define the 2 point that give us the clipping plane
			const std::array<glm::vec2, 2> plane = { points[i], points[(i + 1) % points.size()] };
			glm::vec2 dir = glm::normalize(plane[1] - plane[0]);
			// Get the normal direction (that's why the ccw ordering is important)
			glm::vec2 norm = { -dir.y, dir.x };

			// We clip each triangle in the current list and generate new triangles is necessary
			for (auto& triangle : triangles)
			{

				// For each edge, find the intersections with the current clipping plane
				intersections[0] = IntersectLines({ triangle[0].Postion, triangle[1].Postion }, plane);
				intersections[1] = IntersectLines({ triangle[1].Postion, triangle[2].Postion }, plane);
				intersections[2] = IntersectLines({ triangle[2].Postion, triangle[0].Postion }, plane);

				int32_t count = 0;

				// Based on the intersections and on the original points we create a list of new points
				// that are used to assemble new triangles. Note that the number of new points can be
				// eighter 0, 3 or 4
				for (size_t j = 0; j < intersections.size(); ++j)
				{

					const auto& t = intersections[j].first;

					// If the starting point of the edge is "inside" the plane, add it to the list
					if (glm::dot(triangle[j].Postion - plane[0], norm) > 0.0f)
						assembleList[count++] = triangle[j];
					
					
					if (t >= 0.0f && t <= 1.0f)
					{ // If there's an intersection, add the new intersetion point to the list						
						assembleList[count] = triangle[j];
						assembleList[count].Postion = glm::lerp(triangle[j].Postion, triangle[(j + 1) % triangle.size()].Postion, t);
						assembleList[count].UV = glm::lerp(triangle[j].UV, triangle[(j + 1) % triangle.size()].UV, t);
						count++;
					}

				}

				// With the assemble list we generate a triangle fan (can be eighter 0,1 or 2 triangles
				// depending on the number of points)
				for (int32_t i = 1; i < count - 1; i++)
					tris2.push_back({ assembleList[0], assembleList[i], assembleList[i + 1] });


			}

			// We put the new triangles (tris2) in our final list (triangles). At the next cycle the triangles
			// are going to be tested against another clipping plane. At the end this list is going
			// to contain triangles that are inside all the clipping planes

			triangles = std::move(tris2); // std::move guarantees that the moved container is empty after the operation


		}



		return triangles;

	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec2& uvSize, const glm::vec2& uvOffset)
	{
		const auto& pivot = m_State.top().Pivot;
		std::array<glm::vec2, 4> positions = {
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

		float strWidth = font->GetStringWidth(str.GetText());

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
		DrawString(font, str.GetText(), position + state.TextShadowOffset);
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
		if (m_CurTriangleIndex > 0)
		{
			GLEnableScope scope({ GL_CULL_FACE, GL_DEPTH_TEST, GL_BLEND });

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


			m_TriangleProgram->Use();

			m_TriangleProgram->UniformMatrix4f("uProjection", m_Projection);
			
			for (uint32_t i = 0; i < m_Textures.size(); i++)
			{
				BSF_GLCALL(glActiveTexture(GL_TEXTURE0 + i));
				BSF_GLCALL(glBindTexture(GL_TEXTURE_2D, m_Textures[i]));
			}
			
			m_TriangleProgram->Uniform1iv("uTextures[0]", m_Textures.size(), m_TextureUnits.data());
			
			m_Triangles->GetVertexBuffer(0)->SetSubData(m_TriangleVertices, 0, m_CurTriangleIndex);

			m_Triangles->Draw(GL_TRIANGLES, m_CurTriangleIndex);

			m_CurTriangleIndex = 0;

			//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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
	void FormattedString::SetColor(const glm::vec4& color)
	{
		m_CurrentColor = color;
	}
	void FormattedString::ResetColor()
	{
		m_CurrentColor.reset();
	}

	void FormattedString::Add(const std::string& str)
	{
		m_PlainText += str;
		for (auto c : str)
			m_Characters.push_back({ c, m_CurrentColor });
	}
}