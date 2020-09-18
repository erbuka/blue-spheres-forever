#pragma once

#include <variant>
#include <vector>
#include <optional>
#include <utility>

#include <glm/glm.hpp>

#include "Ref.h"
#include "Time.h"
#include "MatrixStack.h"

namespace bsf
{

	class Texture2D;
	class VertexArray;
	class ShaderProgram;


	struct GLTFNode;
	struct GLTFPrimitive;
	struct GLTFJoint;

	enum class GLTFType
	{
		Scalar,
		Vec2,
		Vec3,
		Vec4,
		Mat2,
		Mat3,
		Mat4
	};

	enum class GLTFPath
	{
		Translation,
		Rotation,
		Scale
	};


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

	struct GLTFJoint
	{
	private:
		glm::mat4* m_Ibm = nullptr;
		glm::mat4* m_JointTransform = nullptr;
	public:

		Ref<GLTFNode> Root = nullptr;

		GLTFJoint() = default;
		GLTFJoint(const Ref<GLTFNode>& root, glm::mat4* ibm, glm::mat4* joint) :
			Root(root), m_Ibm(ibm), m_JointTransform(joint) {}

		const glm::mat4& GetInverseBindTransform() const { return *m_Ibm; }
		void SetJointTransform(const glm::mat4& jt) { *m_JointTransform = jt; }

	};

	struct GLTFSkin
	{
		std::vector<Ref<GLTFNode>> Joints;
		std::vector<glm::mat4> InverseBindTransform;
		std::vector<glm::mat4> JointTransform;
	};

	struct GLTFNode
	{
		std::string Name = "";
		glm::mat4 GlobalTransform = glm::identity<glm::mat4>();
		glm::mat4 InverseGlobalTransform = glm::identity<glm::mat4>();
		glm::mat4 LocalTransform = glm::identity<glm::mat4>();
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::quat Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		Ref<GLTFSkin> Skin = nullptr;
		std::optional<GLTFJoint> Joint = std::nullopt;
		
		Ref<GLTFMesh> Mesh = nullptr;
		
		std::vector<Ref<GLTFNode>> Children;
		Ref<GLTFNode> Parent = nullptr;

		void ComputeLocalTransform() { LocalTransform = glm::translate(Translation) * glm::mat4_cast(Rotation) * glm::scale(Scale); }

		void PreTraverse(const std::function<void(GLTFNode&)> action);
		void PostTraverse(const std::function<void(GLTFNode&)> action);

	};

	struct GLTFScene : public GLTFNode
	{
		void Update();

	};

	struct GLTFAnimationChannel
	{
		GLTFPath Path = GLTFPath::Translation;
		Ref<GLTFNode> Target = nullptr;
		std::vector<float> Time;
		std::variant<std::vector<glm::vec3>, std::vector<glm::vec4>> Data;
	};



	struct GLTFAnimation
	{
		std::string Name = "";
		std::vector<GLTFAnimationChannel> Channels;
	};


	enum class GLTFAttributes {
		Position,
		Normal,
		Uv,
		Joints_0,
		Weights_0
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
		std::vector<Ref<GLTFSkin>> m_Skins;
	};
}