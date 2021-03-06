#pragma once

#include "Common.h"
#include "Renderer2D.h"

#include <array>
#include <memory>
#include <stack>
#include <unordered_map>
#include <optional>
#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace bsf
{
	class VertexArray;
	class ShaderProgram;
	class Texture;
	class Font;

	enum class EPivot
	{
		TopLeft,
		Left,
		BottomLeft,

		TopRight,
		Right,
		BottomRight,

		Top,
		Bottom,
		Center
	};

	struct Renderer2DState
	{
		Ref<Texture2D> CurrentTexture = nullptr;
		glm::mat4 Matrix = glm::identity<glm::mat4>();
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 TextShadowColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		glm::vec2 TextShadowOffset = { 0.1f, 0.1f };
		glm::vec2 Pivot = { 0, 0 };
		std::optional<Rect> Clip;
	};


	struct FormattedString
	{
	public:

		struct Character
		{
			char Code;
			std::optional<glm::vec4> Color;
		};

		FormattedString() = default;
		FormattedString(const char* ch);
		FormattedString(const std::string& str);

		FormattedString& Color(const glm::vec4& color);
		FormattedString& ResetColor();

		FormattedString& Add(const std::string& str);

		size_t Size() const { return m_Characters.size(); }

		const std::string& GetPlainText() const { return m_PlainText; }

		const Character& operator[](size_t i) const { return m_Characters[i]; }
		FormattedString& operator+=(const std::string& str) { return Add(str); }
		FormattedString& operator+=(char c) { return Add(std::string(1, c)); }

	private:
		std::string m_PlainText;
		std::vector<Character> m_Characters;
		std::optional<glm::vec4> m_CurrentColor = std::nullopt;
	};

	class Renderer2D
	{
	public:

		Renderer2D();
		~Renderer2D();

		Renderer2D(const Renderer2D&) = delete;
		Renderer2D(Renderer2D&&) = delete;

		void Initialize();

		void Begin(const glm::mat4& projection);

		void DrawQuad(const glm::vec2& position = { 0.0f, 0.0f }, const glm::vec2& size = { 1.0f, 1.0f }, const glm::vec2& uv = { 1.0f, 1.0f }, const glm::vec2& ovOffset = { 0.0f, 0.0f });
		void DrawString(const Ref<Font>& font, const FormattedString& str, const glm::vec2& position = { 0.0f, 0.0f });
		void DrawStringShadow(const Ref<Font>& font, const FormattedString& str, const glm::vec2& position = { 0.0f, 0.0f });
		
		void LoadIdentity();
		void Scale(const glm::vec2& scale);
		void Scale(float scale) { Scale({ scale, scale }); }
		void Translate(const glm::vec2& translate);
		void Rotate(float angle);

		void Texture(const Ref<::bsf::Texture2D>& texture);
		void NoTexture();

		void Pivot(EPivot mode);
		void Pivot(const glm::vec2& pivot);

		void Color(const glm::vec4& color);
		void TextShadowColor(const glm::vec4& color);
		void TextShadowOffset(const glm::vec2& offset);

		void Clip(const Rect& clip);
		void NoClip();

		void Push();
		void Pop();

		void End();
	private:

		struct Vertex2D
		{
			glm::vec2 Postion;
			glm::vec2 UV;
			glm::vec4 Color;
			uint32_t TextureID;
			uint32_t Clip;
			glm::vec4 ClipPlanes;
		};

		using Triangle2D = std::array<Vertex2D, 3>;


		void DrawTriangleInternal(const std::array<glm::vec2, 3>& positions, const std::array<glm::vec2, 3>& uvs);


		std::vector<uint32_t> m_Textures;
		std::vector<int32_t> m_TextureUnits;
		std::stack<Renderer2DState> m_State;
		glm::mat4 m_Projection;
		Vertex2D* m_TriangleVertices;
		size_t m_CurVertexIndex;
		Ref<VertexArray> m_Triangles;

		Ref<ShaderProgram> m_pTriangleProgram;

	};

}