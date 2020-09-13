#include "BsfPch.h"

#include <json/json.hpp>

#include "GLTF.h"
#include "Texture.h"
#include "Assets.h"
#include "Color.h"
#include "VertexArray.h"
#include "ShaderProgram.h"
#include "Common.h"


namespace bsf
{

	using GLTFBuffer = std::vector<uint8_t>;


	static const std::unordered_map<GLTFAttributes, std::tuple<std::string, std::string, GLenum, AttributeType>> s_Attributes = {
		{ GLTFAttributes::Position,		{ "POSITION",		"VEC3",		GL_FLOAT,  AttributeType::Float3	} },
		{ GLTFAttributes::Normal,		{ "NORMAL",			"VEC3",		GL_FLOAT,  AttributeType::Float3	} },
		{ GLTFAttributes::Uv,			{ "TEXCOORD_0",		"VEC2",		GL_FLOAT,  AttributeType::Float2	} },
	};

	static const std::unordered_map<GLenum, std::tuple<std::string, AttributeType>> s_Index = {
		{ GL_UNSIGNED_INT,		{	"SCALAR",	AttributeType::UInt		} },
		{ GL_UNSIGNED_SHORT,	{	"SCALAR",	AttributeType::UShort	} }
	};


	bool GLTF::Load(std::string_view fileName, const std::initializer_list<GLTFAttributes>& attribs)
	{
		using namespace nlohmann;


		struct GLTFBufferView
		{
			const GLTFBuffer& Buffer;
			size_t ByteOffset = 0;
			size_t ByteLength = 0;

			const uint8_t* Data() const { return Buffer.data() + ByteOffset; }

		};

		struct GLTFAccessor
		{
			const GLTFBufferView& BufferView;
			size_t ByteOffset = 0;
			GLenum ComponentType = 0;
			size_t Count = 0;
			std::string Type = "SCALAR";

			const uint8_t* Data() const { return BufferView.Data() + ByteOffset; }

		};

		const std::regex regBase64("^data:.+;base64,");
		auto fileData = json::parse(ReadTextFile(fileName));
		auto& assets = Assets::GetInstance();

		std::vector<GLTFBuffer> buffers;
		std::vector<GLTFBufferView> bufferViews;
		std::vector<GLTFAccessor> accessors;
		std::vector<std::tuple<std::vector<unsigned char>, uint32_t, uint32_t>> images;

		m_Textures.clear();
		m_Meshes.clear();
		m_Materials.clear();
		m_Scenes.clear();
		m_Nodes.clear();

		const auto loopIfExists = [&](const std::string key, std::function<void(const json& item)> func) {
			if (fileData.contains(key))
			{
				const auto& obj = fileData[key];
				std::for_each(obj.begin(), obj.end(), func);
			}
		};

		loopIfExists("buffers", [&](const json& bufferSpec) {

			auto str = bufferSpec.at("uri").get<std::string>();
			
			std::smatch result;
			GLTFBuffer buffer;

			if (std::regex_search(str, result, regBase64))
				Base64Decode(std::string_view(str).substr(result.position() + result.length()), buffer);

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

			if(accSpec.contains("byteOffset"))
				accessor.ByteOffset = accSpec["byteOffset"].get<size_t>();
			
			if(accSpec.contains("count"))
				accessor.Count = accSpec["count"].get<size_t>();

			if (accSpec.contains("type"))
				accessor.Type = accSpec["type"].get<std::string>();

			accessors.push_back(accessor);

		});

		loopIfExists("images", [&](const json& imgSpec) {
			const auto& bufferView = bufferViews[imgSpec["bufferView"].get<size_t>()];
			images.push_back(LoadPng(bufferView.Data(), bufferView.ByteLength, false));
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
				pbr["baseColorFactor"].get<glm::vec3>() : 
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
					const auto& [name, type, componentType, internalType] = s_Attributes.at(attr);

					const auto& accessor = accessors[primitiveDef["attributes"][name].get<size_t>()];

					assert(accessor.ComponentType == componentType);
					assert(accessor.Type == type);

					auto vb = Ref<VertexBuffer>(new VertexBuffer({ { name, internalType } }, accessor.Data(), accessor.Count));

					vertexBuffers.push_back(std::move(vb));

				}
				
				primitive.Geometry = MakeRef<VertexArray>(vertexBuffers[0]->GetVertexCount(), vertexBuffers.size());
				for (size_t i = 0; i < vertexBuffers.size(); ++i)
					primitive.Geometry->SetVertexBuffer(i, vertexBuffers[i]);

				if (primitiveDef.contains("indices"))
				{
					const auto& accessor = accessors[primitiveDef["indices"].get<size_t>()];

					auto [type, internalType] = s_Index.at(accessor.ComponentType);
					
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

		// Create node hierarchy
		for (size_t i = 0; i < m_Nodes.size(); ++i)
		{
			const auto& nodeDef = fileData["nodes"][i];
			if (nodeDef.contains("children"))
			{
				for (auto idx : nodeDef["children"].get<std::vector<size_t>>())
				{
					m_Nodes[i]->Children.push_back(m_Nodes[idx]);
					m_Nodes[idx]->Parent = m_Nodes[i];
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
				GLTFAnimationChannel channel;

				channel.Target = m_Nodes[channelDef["target"]["node"].get<size_t>()];
				channel.Path = channelDef["target"]["path"].get<std::string>();

				auto samplerDef = animDef["samplers"][channelDef["sampler"].get<size_t>()];

				const auto& input = accessors[samplerDef["input"].get<size_t>()];
				assert(input.Type == "SCALAR");
				assert(input.ComponentType == GL_FLOAT);
				channel.Time.resize(input.Count);
				std::memcpy(channel.Time.data(), input.Data(), input.Count * sizeof(float));

				const auto& output = accessors[samplerDef["output"].get<size_t>()];
				assert(input.Type == "VEC3" || input.Type == "VEC4");
				assert(input.ComponentType == GL_FLOAT);
				channel.Values.resize(input.Count);
				for (size_t i = 0; i < input.Count; i++)
				{
					glm::vec4 val = { 0.0f, 0.0f, 0.0f, 0.0f };
				}

				animation->Channels.push_back(std::move(channel));
			}

			m_Animations.push_back(std::move(animation));
		});

		return true;
	}

	void GLTF::Render(const Time& time, const GLTFRenderConfig& config)
	{

		auto& scene = m_Scenes[0];
		auto program = config.Program;
		
		glm::mat4 current;
		glGetUniformfv(program->GetId(), program->GetUniformLocation(config.ModelMatrixUniform), glm::value_ptr(current));

		m_ModelStack.Reset();
		m_ModelStack.Multiply(current);

		scene->ComputeGlobalTransform();
		scene->PostTraverse([&](GLTFNode& node) {
			if (node.Mesh)
			{
				m_ModelStack.Push();
				m_ModelStack.Multiply(node.GlobalTransform);

				program->UniformMatrix4f(config.ModelMatrixUniform, m_ModelStack);

				for (auto& primitive : *(node.Mesh))
				{
					program->Uniform3fv(config.BaseColorUniform, 1, glm::value_ptr(primitive.Material->BaseColor));
					program->UniformTexture(config.BaseColorTextureUniform, primitive.Material->BaseColorTexture);
					primitive.Geometry->Draw(GL_TRIANGLES);
				}

				m_ModelStack.Pop();

			}
		});


	}

	void GLTFNode::PreTraverse(const std::function<void(GLTFNode&)> action)
	{
		for (auto& child : Children)
			child->PreTraverse(action);

		action(*this);
	}

	void GLTFNode::PostTraverse(const std::function<void(GLTFNode&)> action)
	{
		action(*this);

		for (auto& child : Children)
			child->PostTraverse(action);
	}

	void GLTFScene::ComputeGlobalTransform()
	{
		for (auto& child : Children)
		{
			child->PostTraverse([](GLTFNode& self) {
				self.GlobalTransform = self.Parent->GlobalTransform * self.LocalTransform;
			});
		}
	}

}