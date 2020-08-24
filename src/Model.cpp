#include "BsfPch.h"

#include "Model.h"
#include "VertexArray.h"
#include "Common.h"

namespace bsf
{
	std::vector<Ref<VertexBuffer>> CreateVertexBuffers(const Ref<ModelDef>& modelDef, const glm::vec3 scale, GLenum usage)
	{
		std::vector<Ref<VertexBuffer>> result;

		result.reserve(modelDef->Meshes.size());

		for (const auto& mesh : modelDef->Meshes)
		{
			std::vector<Vertex3D> data(mesh.Positions.size());

			for (uint32_t i = 0; i < mesh.Positions.size(); i++)
			{
				data[i].Position = mesh.Positions[i] * scale;
				data[i].Normal = mesh.Normals[i];
				data[i].Uv = mesh.Uvs[i];
			}

			auto vb = Ref<VertexBuffer>(new VertexBuffer({
				{ "aPosition", AttributeType::Float3 },
				{ "aNormal", AttributeType::Float3 },
				{ "aUv", AttributeType::Float2 }
				}, data.data(), data.size()));

			result.push_back(vb);

		}

		return result;

	}


	Ref<Model> CreateModel(const Ref<ModelDef>& modelDef, const glm::vec3 scale, GLenum usage)
	{

		auto result = MakeRef<Model>();

		for (auto vb : CreateVertexBuffers(modelDef, scale, usage))
			result->AddMesh(Ref<VertexArray>(new VertexArray(vb->GetVertexCount(), { vb })));
		
		return result;
	}

}