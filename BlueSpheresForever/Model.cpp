#include "BsfPch.h"
#include "Model.h"
#include "VertexArray.h"

namespace bsf
{

	

	Ref<Model> CreateModel(Ref<ModelDef>& modelDef, GLenum usage)
	{
		auto result = MakeRef<Model>();

		for (const auto& mesh : modelDef->Meshes)
		{
			std::vector<PBRVertex> data(mesh.Positions.size());

			for (uint32_t i = 0; i < mesh.Positions.size(); i++)
			{
				data[i].Position = mesh.Positions[i];
				data[i].Normal = mesh.Normals[i];
				data[i].Uv = mesh.Uvs[i];
			}

			auto va = Ref<VertexArray>(new VertexArray({
				{ "aPosition", AttributeType::Float3 },
				{ "aNormal", AttributeType::Float3 },
				{ "aUv", AttributeType::Float2 }
				}));

			va->SetData(data.data(), data.size(), usage);

			result->AddMesh(va);
		}

		return result;
	}

}