#pragma once

#include "Common.h"

namespace bsf
{

	struct WavefrontMesh
	{
		std::string Name;
		std::vector<glm::vec3> Positions, Normals;
		std::vector<glm::vec2> Uvs;
	};

	struct WavefrontModel
	{
		std::vector<WavefrontMesh> Meshes;
		const Ref<Model> CreateModel(GLenum usage = GL_STATIC_DRAW);
	};

	class WavefrontLoader
	{
	public:
		WavefrontLoader() = default;
		Ref<WavefrontModel> Load(const std::string& fileName);
	};
}

