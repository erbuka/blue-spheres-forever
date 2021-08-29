#include "BsfPch.h"

#include <json/json.hpp>

#include "GLTF.h"
#include "Texture.h"
#include "Assets.h"
#include "Color.h"
#include "VertexArray.h"
#include "ShaderProgram.h"
#include "Common.h"
#include "Table.h"
#include "Time.h"
#include "MatrixStack.h"


namespace bsf
{
	using namespace std::string_view_literals;
	using GLTFBuffer = std::vector<std::byte>;

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

	static constexpr Table<5, GLTFAttributes, std::string_view, GLTFType, GLenum, AttributeType> s_Attributes = {
		std::make_tuple(GLTFAttributes::Position,	"POSITION"sv,		GLTFType::Vec3,		GL_FLOAT,			AttributeType::Float3),
		std::make_tuple(GLTFAttributes::Normal,		"NORMAL"sv,			GLTFType::Vec3,		GL_FLOAT,			AttributeType::Float3),
		std::make_tuple(GLTFAttributes::Uv,			"TEXCOORD_0"sv,		GLTFType::Vec2,		GL_FLOAT,			AttributeType::Float2),
		std::make_tuple(GLTFAttributes::Joints_0,	"JOINTS_0"sv,		GLTFType::Vec4,		GL_UNSIGNED_SHORT,  AttributeType::UShort4),
		std::make_tuple(GLTFAttributes::Weights_0,	"WEIGHTS_0"sv,		GLTFType::Vec4,		GL_FLOAT,			AttributeType::Float4)
	};

	static constexpr Table<2, GLenum, GLTFType, AttributeType> s_IndexBufferTypes = {
		std::make_tuple(GL_UNSIGNED_INT,	GLTFType::Scalar,	AttributeType::UInt),
		std::make_tuple(GL_UNSIGNED_SHORT,	GLTFType::Scalar,	AttributeType::UShort)
	};

	static constexpr Table<3, std::string_view, GLTFPath> s_GLTFPaths = {
		std::make_tuple("translation",	GLTFPath::Translation),
		std::make_tuple("rotation",		GLTFPath::Rotation),
		std::make_tuple("scale",		GLTFPath::Scale)
	};

	static constexpr Table<7, std::string_view, size_t, GLTFType> s_GLTFTypes = {
		std::make_tuple("SCALAR"sv,  1,		GLTFType::Scalar),
		std::make_tuple("VEC2"sv,	 2,		GLTFType::Vec2),
		std::make_tuple("VEC3"sv,	 3,		GLTFType::Vec3),
		std::make_tuple("VEC4"sv,	 4,		GLTFType::Vec4),
		std::make_tuple("MAT2"sv,	 4,		GLTFType::Mat2),
		std::make_tuple("MAT3"sv,	 9,		GLTFType::Mat3),
		std::make_tuple("MAT4"sv,	 16,	GLTFType::Mat4)
	};

	static constexpr Table<5, GLenum, size_t> s_GLTFComponentTypes = {
		std::make_tuple(GL_INT, 4),
		std::make_tuple(GL_UNSIGNED_INT, 4),
		std::make_tuple(GL_FLOAT, 4),
		std::make_tuple(GL_SHORT, 2),
		std::make_tuple(GL_UNSIGNED_SHORT, 2)
	};

	struct GLTFNode;
	struct GLTFPrimitive;
	struct GLTFJoint;

	struct GLTFBufferView
	{
		const GLTFBuffer& Buffer;
		size_t ByteOffset = 0;
		size_t ByteLength = 0;

		const std::byte* Data() const { return Buffer.data() + ByteOffset; }

	};

	struct GLTFAccessor
	{
		const GLTFBufferView& BufferView;
		size_t ByteOffset = 0;
		GLenum ComponentType = 0;
		size_t Count = 0;
		GLTFType Type = GLTFType::Scalar;

		size_t ByteLength() const {
			return Count * s_GLTFTypes.Get<2, 1>(Type) * s_GLTFComponentTypes.Get<0, 1>(ComponentType);
		}

		template<typename T>
		const T& View(const size_t index) const
		{
			assert(sizeof(T) * (index + 1) <= ByteLength());
			return *(reinterpret_cast<const T*>(Data()) + index);
		}

		const std::byte* Data() const { return BufferView.Data() + ByteOffset; }

	};


	struct GLTFMaterial
	{
		glm::vec4 BaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
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
		glm::mat4 InitialInverseGlobalTransform = glm::identity<glm::mat4>();
		glm::mat4 GlobalTransform = glm::identity<glm::mat4>();
		glm::mat4 LocalTransform = glm::identity<glm::mat4>();
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::quat Rotation = glm::identity<glm::quat>();
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		Ref<GLTFSkin> Skin = nullptr;
		std::optional<GLTFJoint> Joint = std::nullopt;

		Ref<GLTFMesh> Mesh = nullptr;

		std::vector<Ref<GLTFNode>> Children;
		Ref<GLTFNode> Parent = nullptr;

		void ComputeLocalTransform() { LocalTransform = glm::translate(Translation) * glm::mat4_cast(Rotation) * glm::scale(Scale); }

		void PreTraverse(const std::function<void(GLTFNode&)> action)
		{

			for (auto& child : Children)
				child->PreTraverse(action);

			action(*this);
		}

		void PostTraverse(const std::function<void(GLTFNode&)> action)
		{
			action(*this);

			for (auto& child : Children)
				child->PostTraverse(action);
		}

	};

	struct GLTFScene : public GLTFNode
	{
		void Update()
		{
			for (auto& child : Children)
			{
				child->PostTraverse([](GLTFNode& self) {
					
					self.ComputeLocalTransform();
					self.GlobalTransform = self.Parent->GlobalTransform * self.LocalTransform;

					if (self.Joint.has_value())
					{
						auto& joint = self.Joint.value();
						joint.SetJointTransform(joint.Root->InitialInverseGlobalTransform * self.GlobalTransform * joint.GetInverseBindTransform());
					}
				});
			}
		}
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

	struct GLTF::Impl
	{
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
		float m_AnimationGlobalTimeWarp = 1.0f;

		std::vector<Ref<Texture2D>> m_Textures;
		std::vector<Ref<GLTFMaterial>> m_Materials;
		std::vector<Ref<GLTFMesh>> m_Meshes;
		std::vector<Ref<GLTFNode>> m_Nodes;
		std::vector<Ref<GLTFScene>> m_Scenes;
		std::vector<Ref<GLTFAnimation>> m_Animations;
		std::vector<Ref<GLTFSkin>> m_Skins;

		void UpdateAnimationState(AnimationState& state, const Time& time)
		{
			state.Time += time.Delta * state.TimeWarp * m_AnimationGlobalTimeWarp;

			const float duration = state.Animation->MaxTime - state.Animation->MinTime;

			while (state.Loop && state.Time > state.Animation->MaxTime)
				state.Time -= duration;

			while (state.Loop && state.Time < state.Animation->MinTime)
				state.Time += duration;

			state.Time = std::clamp(state.Time, state.Animation->MinTime, state.Animation->MaxTime);
		}

		void UpdateAnimations(const Time& time)
		{
			if (!m_CurrentAnimation.has_value())
				return;

			auto& current = m_CurrentAnimation.value();

			UpdateAnimationState(current, time);

			for (const auto& channel : current.Animation->Channels)
			{
				switch (channel.Path)
				{
				case GLTFPath::Translation:
					channel.Target->Translation = channel.Interpolate<glm::vec3>(current.Time);
					break;
				case GLTFPath::Rotation:
					channel.Target->Rotation = channel.Interpolate<glm::quat>(current.Time);
					break;
				case GLTFPath::Scale:
					channel.Target->Scale = channel.Interpolate<glm::vec3>(current.Time);
					break;
				}
			}

			if (m_NextAnimation.has_value())
			{
				auto& next = m_NextAnimation.value();

				m_AnimationTransition = std::min(m_AnimationTransitionDuration, m_AnimationTransition + time.Delta);
				float delta = m_AnimationTransition / m_AnimationTransitionDuration;

				UpdateAnimationState(next, time);

				for (const auto& channel : next.Animation->Channels)
				{
					switch (channel.Path)
					{
					case GLTFPath::Translation:
						channel.Target->Translation = glm::lerp(channel.Target->Translation, channel.Interpolate<glm::vec3>(next.Time), delta);
						break;
					case GLTFPath::Rotation:
						channel.Target->Rotation = glm::slerp(channel.Target->Rotation, channel.Interpolate<glm::quat>(next.Time), delta);
						break;
					case GLTFPath::Scale:
						channel.Target->Scale = glm::lerp(channel.Target->Scale, channel.Interpolate<glm::vec3>(next.Time), delta);
						break;
					}
				}

				if (delta >= 1.0f)
				{
					m_CurrentAnimation = m_NextAnimation;
					m_NextAnimation = std::nullopt;
				}

			}

		}

		bool Load(std::string_view fileName, const std::initializer_list<GLTFAttributes>& attribs)
		{
			using namespace nlohmann;


			const std::regex regBase64("^data:.+;base64,");
			auto fileData = json::parse(ReadTextFile(fileName));
			auto& assets = Assets::GetInstance();
			auto baseDir = std::filesystem::path(fileName).parent_path();


			std::vector<GLTFBuffer> buffers;
			std::vector<GLTFBufferView> bufferViews;
			std::vector<GLTFAccessor> accessors;
			std::vector<std::tuple<std::vector<std::byte>, uint32_t, uint32_t>> images;

			m_Textures.clear();
			m_Meshes.clear();
			m_Materials.clear();
			m_Scenes.clear();
			m_Nodes.clear();
			m_Animations.clear();
			m_Skins.clear();

			const auto loopIfExists = [&](const std::string key, std::function<void(const json& item)> func) {
				if (fileData.contains(key))
				{
					const auto& obj = fileData[key];
					std::for_each(obj.begin(), obj.end(), func);
				}
			};

			const auto loopIfExistsIdx = [&](const std::string key, std::function<void(const json& item, size_t index)> func) {
				if (fileData.contains(key))
				{
					const auto& obj = fileData[key];

					size_t idx = 0;
					for (const auto& o : obj)
						func(o, idx++);

				}
			};

			loopIfExists("buffers", [&](const json& bufferSpec) {

				auto uri = bufferSpec.at("uri").get<std::string>();

				std::smatch result;
				GLTFBuffer buffer;

				if (std::regex_search(uri, result, regBase64))
				{
					// Base64 Encoded
					Base64Decode(std::string_view(uri).substr(result.position() + result.length()), buffer);
				} 
				else
				{
					// File
					buffer = ReadBinaryFile((baseDir / uri).string());
				}

				buffers.push_back(std::move(buffer));
			});

			loopIfExists("bufferViews", [&](const json& bvSpec) {
				GLTFBufferView view = {
					buffers[bvSpec["buffer"].get<size_t>()],
					bvSpec["byteOffset"].get<size_t>(),
					bvSpec["byteLength"].get<size_t>()
				};

				bufferViews.push_back(view);
			});

			loopIfExists("accessors", [&](const json& accSpec) {
				GLTFAccessor accessor = { bufferViews[accSpec["bufferView"].get<size_t>()] };

				accessor.ComponentType = accSpec["componentType"].get<GLenum>();

				if (accSpec.contains("byteOffset"))
					accessor.ByteOffset = accSpec["byteOffset"].get<size_t>();

				if (accSpec.contains("count"))
					accessor.Count = accSpec["count"].get<size_t>();

				if (accSpec.contains("type"))
					accessor.Type = s_GLTFTypes.Get<0, 2>(accSpec["type"].get<std::string>());

				accessors.push_back(accessor);

			});

			loopIfExists("images", [&](const json& imgSpec) {
				if (imgSpec.contains("bufferView"))
				{
					const auto& bufferView = bufferViews[imgSpec["bufferView"].get<size_t>()];
					images.push_back(ImageLoad(bufferView.Data(), bufferView.ByteLength, false));
				}
				else if (imgSpec.contains("uri"))
				{
					const auto data = ReadBinaryFile((baseDir / imgSpec["uri"].get<std::string>()).string());
					images.push_back(ImageLoad(data.data(), data.size(), false));
				}
				else
				{
					throw std::runtime_error("Invalid image");
				}
			});

			loopIfExists("textures", [&](const json& texSpec) {
				const auto& [pixels, width, height] = images[texSpec["source"].get<size_t>()];
				auto texture = MakeRef<Texture2D>(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
				texture->SetPixels(pixels.data(), width, height);
				texture->SetFilter(TextureFilter::Linear, TextureFilter::Linear);
				m_Textures.push_back(std::move(texture));
			});

			loopIfExists("materials", [&](const json& matSpec) {
				auto material = MakeRef<GLTFMaterial>();
				const auto& pbr = matSpec["pbrMetallicRoughness"];

				material->BaseColor = pbr.contains("baseColorFactor") ?
					pbr["baseColorFactor"].get<glm::vec4>() :
					Colors::White;

				material->BaseColorTexture = pbr.contains("baseColorTexture") ?
					m_Textures[pbr["baseColorTexture"]["index"].get<size_t>()] :
					assets.Get<Texture2D>(AssetName::TexWhite);

				m_Materials.push_back(std::move(material));
			});

			loopIfExists("meshes", [&](const json& meshDef) {
				auto mesh = MakeRef<GLTFMesh>();

				for (const auto& primitiveDef : meshDef["primitives"])
				{
					GLTFPrimitive primitive;

					primitive.Material = m_Materials[primitiveDef["material"].get<size_t>()];

					std::vector<Ref<VertexBuffer>> vertexBuffers;

					for (auto attr : attribs)
					{
						const auto& [_, name, type, componentType, internalType] = s_Attributes[attr];

						const auto& accessor = accessors[primitiveDef["attributes"][name.data()].get<size_t>()];

						assert(accessor.ComponentType == componentType);
						assert(accessor.Type == type);

						auto vb = Ref<VertexBuffer>(new VertexBuffer({ { name.data(), internalType } }, accessor.Data(), accessor.Count));

						vertexBuffers.push_back(std::move(vb));

					}

					primitive.Geometry = MakeRef<VertexArray>(vertexBuffers[0]->GetVertexCount(), vertexBuffers.size());
					for (size_t i = 0; i < vertexBuffers.size(); ++i)
						primitive.Geometry->SetVertexBuffer(i, vertexBuffers[i]);

					if (primitiveDef.contains("indices"))
					{
						const auto& accessor = accessors[primitiveDef["indices"].get<size_t>()];

						const auto& [_, type, internalType] = s_IndexBufferTypes[accessor.ComponentType];

						assert(accessor.Type == type);

						auto ib = MakeRef<IndexBuffer>(accessor.Data(), internalType, accessor.Count);

						primitive.Geometry->SetIndexBuffer(ib);
					}

					mesh->push_back(std::move(primitive));

				}

				m_Meshes.push_back(mesh);
			});




			// Create nodes
			loopIfExists("nodes", [&](const json& nodeDef) {
				auto node = MakeRef<GLTFNode>();

				if (nodeDef.contains("name"))
					node->Name = nodeDef["name"].get<std::string>();

				if (nodeDef.contains("translation"))
					node->Translation = nodeDef["translation"].get<glm::vec3>();

				if (nodeDef.contains("rotation"))
					node->Rotation = nodeDef["rotation"].get<glm::quat>();
				;
				if (nodeDef.contains("scale"))
					node->Scale = nodeDef["scale"].get<glm::vec3>();

				if (nodeDef.contains("matrix"))
					node->LocalTransform = nodeDef["matrix"].get<glm::mat4>();

				if (nodeDef.contains("mesh"))
					node->Mesh = m_Meshes[nodeDef["mesh"].get<size_t>()];

				m_Nodes.push_back(std::move(node));
			});

			// Skins
			loopIfExists("skins", [&](const json& skinDef) {
				auto skin = MakeRef<GLTFSkin>();
				auto joints = skinDef["joints"].get<std::vector<size_t>>();

				skin->Joints.resize(joints.size());
				skin->InverseBindTransform.resize(joints.size());
				skin->JointTransform.resize(joints.size());

				const auto& ibmAccessor = accessors[skinDef["inverseBindMatrices"].get<size_t>()];

				assert(ibmAccessor.Count == joints.size());
				assert(ibmAccessor.Type == GLTFType::Mat4);
				assert(ibmAccessor.ComponentType == GL_FLOAT);

				std::memcpy(skin->InverseBindTransform.data(), ibmAccessor.Data(), ibmAccessor.ByteLength());

				for (size_t i = 0; i < joints.size(); ++i)
					skin->Joints[i] = m_Nodes[joints[i]];


				m_Skins.push_back(std::move(skin));
			});


			// Parent/child binding and skin
			for (size_t i = 0; i < m_Nodes.size(); ++i)
			{
				const auto& nodeDef = fileData["nodes"][i];

				// Parent / child
				if (nodeDef.contains("children"))
				{
					for (auto idx : nodeDef["children"].get<std::vector<size_t>>())
					{
						m_Nodes[i]->Children.push_back(m_Nodes[idx]);
						m_Nodes[idx]->Parent = m_Nodes[i];
					}
				}

				// Skin
				if (nodeDef.contains("skin"))
				{
					auto& skin = m_Skins[nodeDef["skin"].get<size_t>()];

					m_Nodes[i]->Skin = skin;

					// TODO Should check if there's a skeleton
					const auto& skinRoot = m_Nodes[i];

					for (size_t j = 0; j < skin->Joints.size(); ++j)
					{
						skin->Joints[j]->Joint = GLTFJoint{
							skinRoot,
							&skin->InverseBindTransform[j],
							&skin->JointTransform[j]
						};
					}

				}
			}

			// Compute initial global transforms
			for (auto& node : m_Nodes)
			{
				if (!node->Parent)
				{
					node->ComputeLocalTransform();
					node->GlobalTransform = node->LocalTransform;
					node->InitialInverseGlobalTransform = glm::inverse(node->GlobalTransform);
					for (auto& child : node->Children)
					{
						child->PostTraverse([](GLTFNode& self) {
							self.ComputeLocalTransform();
							self.GlobalTransform = self.Parent->GlobalTransform * self.LocalTransform;
							self.InitialInverseGlobalTransform = glm::inverse(self.GlobalTransform);
						});
					}
				}
			}


			loopIfExists("scenes", [&](const json& sceneDef) {
				auto scene = MakeRef<GLTFScene>();

				for (auto idx : sceneDef["nodes"].get<std::vector<size_t>>())
				{
					scene->Children.push_back(m_Nodes[idx]);
					m_Nodes[idx]->Parent = scene;
				}

				m_Scenes.push_back(std::move(scene));
			});


			loopIfExists("animations", [&](const json& animDef) {
				auto animation = MakeRef<GLTFAnimation>();

				for (auto channelDef : animDef["channels"])
				{
					auto samplerDef = animDef["samplers"][channelDef["sampler"].get<size_t>()];

					const auto& input = accessors[samplerDef["input"].get<size_t>()];
					const auto& output = accessors[samplerDef["output"].get<size_t>()];

					assert((output.Type == GLTFType::Vec3 || output.Type == GLTFType::Vec4) && output.ComponentType == GL_FLOAT);
					assert(input.Type == GLTFType::Scalar && input.ComponentType == GL_FLOAT);
					assert(input.Count == output.Count);

					GLTFAnimationChannel channel;

					channel.Target = m_Nodes[channelDef["target"]["node"].get<size_t>()];
					channel.Path = s_GLTFPaths.Get<0, 1>(channelDef["target"]["path"].get<std::string>());

					channel.Time.resize(input.Count);
					std::memcpy(channel.Time.data(), input.Data(), input.ByteLength());

					if (output.Type == GLTFType::Vec3) channel.Data = std::vector<glm::vec3>();
					else if (output.Type == GLTFType::Vec4) channel.Data = std::vector<glm::quat>();
					else throw std::range_error("Invalid animation output type");

					std::visit([&](auto&& data) -> void {
						data.resize(output.Count);
						std::memcpy(data.data(), output.Data(), output.ByteLength());
					}, channel.Data);

					auto [minTime, maxTime] = std::minmax_element(channel.Time.begin(), channel.Time.end());

					channel.MinTime = *minTime;
					channel.MaxTime = *maxTime;

					animation->Channels.push_back(std::move(channel));
				}

				animation->Name = animDef["name"].get<std::string>();

				animation->MinTime = std::numeric_limits<float>::infinity();
				animation->MaxTime = -std::numeric_limits<float>::infinity();

				for (const auto& c : animation->Channels)
				{
					animation->MinTime = std::min(c.MinTime, animation->MinTime);
					animation->MaxTime = std::max(c.MaxTime, animation->MaxTime);
				}

				m_Animations.push_back(std::move(animation));
			});


			// TODO Debug Hips
			
			for (auto& anim : m_Animations)
			{
				if (anim->Name == "ball")
				{
					for (auto& chan : anim->Channels)
					{
						if (chan.Target->Name == "Hips")
						{
							std::visit([&](auto& v) {
								json j = v;
								fmt::print("---path {} ---\n{}\n", chan.Path, j.dump(1));
							}, chan.Data);
						}
					}
				}
			}
			

			//

			return true;
		}

		void Update(const Time& time)
		{
			auto& scene = m_Scenes[0];
			UpdateAnimations(time);
			scene->Update();
		}

		void Render(const Time& time, const GLTFRenderConfig& config)
		{
			auto& scene = m_Scenes[0];
			auto program = config.Program;

			glm::mat4 current;
			glGetUniformfv(program->GetId(), program->GetUniformLocation(config.ModelMatrixUniform), glm::value_ptr(current));

			m_ModelStack.Reset();
			m_ModelStack.Multiply(current);

			scene->PostTraverse([&](GLTFNode& node) {
				if (node.Mesh)
				{
					m_ModelStack.Push();
					m_ModelStack.Multiply(node.GlobalTransform);


					program->UniformMatrix4f(config.ModelMatrixUniform, m_ModelStack);

					if (node.Skin)
						program->UniformMatrix4fv(config.JointTransformUniform, node.Skin->JointTransform.size(),
							glm::value_ptr(node.Skin->JointTransform[0]));

					for (auto& primitive : *(node.Mesh))
					{
						program->Uniform4fv(config.BaseColorUniform, 1, glm::value_ptr(primitive.Material->BaseColor));
						program->UniformTexture(config.BaseColorTextureUniform, primitive.Material->BaseColorTexture);
						primitive.Geometry->Draw(GL_TRIANGLES);
					}

					m_ModelStack.Pop();

				}
			});


		}

		void StopAllAnimations()
		{
			m_CurrentAnimation = std::nullopt;
			m_NextAnimation = std::nullopt;
		}

		void PlayAnimation(std::string_view name, bool loop, float timeWarp)
		{
			const auto it = std::find_if(m_Animations.begin(), m_Animations.end(), [name](const auto& anim) { return anim->Name == name; });
			m_CurrentAnimation = {
				*it,
				0.0f,
				timeWarp,
				loop
			};
			m_NextAnimation = std::nullopt;
		}

		void FadeToAnimation(std::string_view next, float fadeTime, bool loop, float timeWarp)
		{
			if (!m_CurrentAnimation.has_value())
			{
				PlayAnimation(next, loop, timeWarp);
				return;
			}

			const auto it = std::find_if(m_Animations.begin(), m_Animations.end(), [next](const auto& anim) {return anim->Name == next; });

			m_NextAnimation = {
				*it,
				0.0f,
				timeWarp,
				loop
			};

			m_AnimationTransition = 0.0f;
			m_AnimationTransitionDuration = fadeTime;

		}

		void SetAnimationGlobalTimeWarp(float timeWarp) {
			m_AnimationGlobalTimeWarp = timeWarp;
		}

	};

	GLTF::GLTF()
	{
		m_Impl = std::make_unique<Impl>();
	}

	GLTF::~GLTF() {}

	bool GLTF::Load(std::string_view fileName, const std::initializer_list<GLTFAttributes>& attribs)
	{
		BSF_INFO("Loading GLTF: {0}", fileName.data());
		return m_Impl->Load(fileName, attribs);
	}

	void GLTF::Render(const Time& time, const GLTFRenderConfig& config)
	{
		m_Impl->Render(time, config);
	}
	void GLTF::Update(const Time& time)
	{
		m_Impl->Update(time);
	}
	void GLTF::StopAllAnimations()
	{
		m_Impl->StopAllAnimations();
	}
	void GLTF::PlayAnimation(std::string_view name, bool loop, float timeWarp)
	{
		m_Impl->PlayAnimation(name, loop, timeWarp);
	}

	void GLTF::FadeToAnimation(std::string_view next, float fadeTime, bool loop, float timeWarp)
	{
		m_Impl->FadeToAnimation(next, fadeTime, loop, timeWarp);
	}

	void GLTF::SetAnimationGlobalTimeWarp(float timeWarp)
	{
		m_Impl->SetAnimationGlobalTimeWarp(timeWarp);
	}

}