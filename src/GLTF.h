#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "Ref.h"
#include "Time.h"
#include "MatrixStack.h"

namespace bsf
{

	class Texture2D;
	class VertexArray;
	class ShaderProgram;

	struct GLTFMaterial
	{
		glm::vec3 BaseColor = { 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> BaseColorTexture = nullptr;

		GLTFMaterial() = default;
		GLTFMaterial(GLTFMaterial&&) = default;

		GLTFMaterial(const GLTFMaterial&) = delete;
		GLTFMaterial& operator=(const GLTFMaterial&) = delete;
	};

	struct GLTFPrimitive
	{
		Ref<VertexArray> Geometry;
		Ref<GLTFMaterial> Material;
	};

	using GLTFMesh = std::vector<GLTFPrimitive>;

	struct GLTFNode
	{
		std::string Name = "";
		glm::mat4 GlobalTransform = glm::identity<glm::mat4>();
		glm::mat4 LocalTransform = glm::identity<glm::mat4>();
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::quat Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };
		
		Ref<GLTFMesh> Mesh = nullptr;
		
		std::vector<Ref<GLTFNode>> Children;
		Ref<GLTFNode> Parent = nullptr;

		void ComputeLocalTransform() { LocalTransform = glm::translate(Translation) * glm::mat4_cast(Rotation) * glm::scale(Scale); }

		void PreTraverse(const std::function<void(GLTFNode&)> action);
		void PostTraverse(const std::function<void(GLTFNode&)> action);

	};

	struct GLTFScene : public GLTFNode
	{
		void ComputeGlobalTransform();

	};

	struct GLTFAnimationChannel
	{
		std::string Path = "";
		Ref<GLTFNode> Target = nullptr;
		std::vector<float> Time;
		std::vector<glm::vec4> Values;
	};


	struct GLTFAnimation
	{
		std::string Name = "";
		std::vector<GLTFAnimationChannel> Channels;
	};

	enum class GLTFAttributes {
		Position,
		Normal,
		Uv
	};

	struct GLTFRenderConfig
	{
		Ref<ShaderProgram> Program;
		std::string ModelMatrixUniform = "uModel";
		std::string BaseColorTextureUniform = "uTexture";
		std::string BaseColorUniform = "uColor";
	};


	class GLTF
	{
	public:
		bool Load(std::string_view fileName, const std::initializer_list<GLTFAttributes>& attribs);

		void Render(const Time& time, const GLTFRenderConfig& config);

		const std::vector<Ref<GLTFScene>>& GetScenes() const { return m_Scenes; }


	private:

		MatrixStack m_ModelStack;

		std::vector<Ref<Texture2D>> m_Textures;
		std::vector<Ref<GLTFMaterial>> m_Materials;
		std::vector<Ref<GLTFMesh>> m_Meshes;
		std::vector<Ref<GLTFNode>> m_Nodes;
		std::vector<Ref<GLTFScene>> m_Scenes;
		std::vector<Ref<GLTFAnimation>> m_Animations;
	};
}