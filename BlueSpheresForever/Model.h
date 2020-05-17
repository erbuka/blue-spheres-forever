#pragma once

#include <vector>

#include "Common.h"
#include "Asset.h"

namespace bsf
{
	class Model;
	class VertexArray;
	class VertexBuffer;


	struct MeshDef
	{
		std::string Name;
		std::vector<glm::vec3> Positions, Normals;
		std::vector<glm::vec2> Uvs;
	};

	struct ModelDef
	{
		std::vector<MeshDef> Meshes;
	};

	class Model : public Asset
	{
	public:
		uint32_t GetMeshCount() const { return m_Meshes.size(); }
		const std::vector<Ref<VertexArray>>& GetMeshes() { return m_Meshes; }
		const Ref<VertexArray>& GetMesh(uint32_t index) { assert(index < m_Meshes.size()); return m_Meshes.at(index); }
		void AddMesh(const Ref<VertexArray>& mesh) { m_Meshes.push_back(mesh); }

	private:
		std::vector<Ref<VertexArray>> m_Meshes;
	};

	std::vector<Ref<VertexBuffer>> CreateVertexBuffers(const Ref<ModelDef>& modelDef, const glm::vec3 scale, GLenum usage);
	Ref<Model> CreateModel(const Ref<ModelDef>& modelDef, const glm::vec3 scale = { 1.0f, 1.0f, 1.0f }, GLenum usage = GL_STATIC_DRAW);

}

