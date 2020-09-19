#pragma once

#include <variant>
#include <vector>
#include <optional>
#include <utility>

#include <glm/glm.hpp>

#include "Ref.h"
#include "Time.h"
#include "MatrixStack.h"
#include "Log.h"

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
		float MinTime = 0.0f;
		float MaxTime = 0.0f;
		GLTFPath Path = GLTFPath::Translation;
		Ref<GLTFNode> Target = nullptr;
		std::vector<float> Time;
		std::variant<std::vector<glm::vec3>, std::vector<glm::quat>> Data;

		template<typename T>
		T Interpolate(float t) const
		{
			const auto& values = std::get<std::vector<T>>(Data);

			if (t < MinTime) return values[0];
			
			for (size_t i = 0; i < Time.size() - 1; ++i)
			{
				if (t >= Time[i] && t < Time[i + 1])
				{
					const float delta = (t - Time[i]) / (Time[i + 1] - Time[i]);
					if constexpr (std::is_same_v<T, glm::vec3>) glm::lerp(values[i], values[i + 1], delta);
					else if constexpr (std::is_same_v<T, glm::quat>) return glm::slerp(values[i], values[i + 1], delta);
					else BSF_ERROR("Invalid interpolated type: {0}", typeid(T).name());
				}
			}
			

			return values[values.size() - 1];

		}

	};


	struct GLTFAnimation
	{
		float MinTime = 0.0f;
		float MaxTime = 0.0f;
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

		void PlayAnimation(std::string_view name, bool loop = true, float timeWarp = 1.0f);
		void FadeToAnimation(std::string_view next, float fadeTime, bool loop = true, float timeWarp = 1.0f);

	private:

		MatrixStack m_ModelStack;

		struct AnimationState
		{
			Ref<GLTFAnimation> Animation = nullptr;
			float Time = 0.0f;
			float TimeWarp = 1.0f;
			bool Loop = true;
		};

		std::optional<AnimationState> m_CurrentAnimation = std::nullopt;
		std::optional<AnimationState> m_NextAnimation = std::nullopt;
		float m_AnimationTransition = 0.0f;
		float m_AnimationTransitionDuration = 0.0f;

		std::vector<Ref<Texture2D>> m_Textures;
		std::vector<Ref<GLTFMaterial>> m_Materials;
		std::vector<Ref<GLTFMesh>> m_Meshes;
		std::vector<Ref<GLTFNode>> m_Nodes;
		std::vector<Ref<GLTFScene>> m_Scenes;
		std::vector<Ref<GLTFAnimation>> m_Animations;
		std::vector<Ref<GLTFSkin>> m_Skins;

		void UpdateAnimationState(AnimationState& state, const Time& time);
		void UpdateAnimations(const Time& time);
	};
}