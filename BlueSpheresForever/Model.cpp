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
			std::vector<Vertex3D> data(mesh.Positions.size());

			for (uint32_t i = 0; i < mesh.Positions.size(); i++)
			{
				data[i].Position = mesh.Positions[i];
				data[i].Normal = mesh.Normals[i];
				data[i].Uv = mesh.Uvs[i];
			}

			auto vb = Ref<VertexBuffer>(new VertexBuffer({
				{ "aPosition", AttributeType::Float3 },
				{ "aNormal", AttributeType::Float3 },
				{ "aUv", AttributeType::Float2 }
			}, data.data(), data.size()));


			result->AddMesh(Ref<VertexArray>(new VertexArray(data.size(), { vb })));
		}

		return result;
	}

	Ref<Model> CreateMorphModel(Ref<ModelDef>& m0, Ref<ModelDef>& m1, GLenum usage)
	{

		return nullptr;
		/*
		assert(m0->Meshes.size() == m1->Meshes.size());

		for (uint32_t i = 0; i < m0->Meshes.size(); i++)
		{
			assert(m0->Meshes[i].Positions.size() == m1->Meshes[i].Positions.size());
			assert(m0->Meshes[i].Normals.size() == m1->Meshes[i].Normals.size());
			assert(m0->Meshes[i].Uvs.size() == m1->Meshes[i].Uvs.size());
		}

		auto result = MakeRef<Model>();

		for (uint32_t i = 0; i < m0->Meshes.size(); i++)
		{
			const auto& mesh0 = m0->Meshes[i];
			const auto& mesh1 = m1->Meshes[i];

			std::vector<MorphVertex3D> vertices(mesh0.Positions.size());

			for (uint32_t j = 0; j < mesh0.Positions.size(); i++)
			{
				vertices[j].Position0 = mesh0.Positions[j];
				vertices[j].Position1 = mesh1.Positions[j];
				vertices[j].Normal0 = mesh0.Normals[j];
				vertices[j].Normal1 = mesh1.Normals[j];
				vertices[j].Uv0 = mesh0.Uvs[j];
				vertices[j].Uv1 = mesh1.Uvs[j];
			}

			auto vb = Ref<VertexArray>(new VertexArray({
				{ "aPosition0", AttributeType::Float3 },
				{ "aPosition1", AttributeType::Float3 },
				{ "aNormal0", AttributeType::Float3 },
				{ "aNormal1", AttributeType::Float3 },
				{ "aUv0", AttributeType::Float2 },
				{ "aUv1", AttributeType::Float2 },
			}));

			va->SetData(vertices.data(), vertices.size(), usage);

			result->AddMesh(va);

		}

		return result;

		*/
	}

}