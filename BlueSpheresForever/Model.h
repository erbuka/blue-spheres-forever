#pragma once

#include <vector>

#include "Common.h"
#include "Asset.h"

namespace bsf
{
	class Model;
	class VertexArray;


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

	class AnimatedModel : public Asset
	{
	public:
		uint32_t GetFrameCount() const { return m_Frames.size(); }
		const std::vector<Ref<Model>>& GetFrames() { return m_Frames; }
		const Ref<Model>& GetFrame(uint32_t index) { assert(index < m_Frames.size()); return m_Frames.at(index); }
		void AddFrame(const Ref<Model>& frame) { m_Frames.push_back(frame); }
	private:
		std::vector<Ref<Model>> m_Frames;
	};

	Ref<Model> CreateModel(Ref<ModelDef>& modelDef, GLenum usage = GL_STATIC_DRAW);

}

