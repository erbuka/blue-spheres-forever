#pragma once

#include "Common.h"

#include <array>
#include <memory>
#include <stack>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace bsf
{
	class VertexArray;
	class ShaderProgram;
	class Texture;

	enum class EQuadPivot
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

		glm::vec2 QuadPivot = { 0, 0 };
	};

	class Renderer2D
	{
	public:

		Renderer2D();
		~Renderer2D();

		void Initialize();

		void Begin(const glm::mat4& projection);

		void DrawQuad(const glm::vec2& position);
		
		void LoadIdentity();
		void Scale(const glm::vec2& scale);
		void Translate(const glm::vec2& translate);
		void Rotate(float angle);

		void Texture(const Ref<::bsf::Texture2D>& texture);
		void NoTexture();

		void QuadPivot(EQuadPivot mode);
		void QuadPivot(const glm::vec2& pivot);

		void Color(const glm::vec4& color);

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
		};

		void DrawQuadInternal(const glm::vec2& position, const glm::vec2& size, const glm::vec2& pivot);
		void DrawTriangleInternal(const std::array<glm::vec2, 3>& positions, const std::array<glm::vec2, 3>& uvs);

		Ref<::bsf::Texture> m_WhiteTexture;
		std::vector<uint32_t> m_Textures;
		std::vector<int32_t> m_TextureUnits;
		std::stack<Renderer2DState> m_State;
		glm::mat4 m_Projection;
		Vertex2D* m_TriangleVertices;
		uint32_t m_CurTriangleIndex;
		Ref<VertexArray> m_Triangles;

		Ref<ShaderProgram> m_TriangleProgram;

	};

}