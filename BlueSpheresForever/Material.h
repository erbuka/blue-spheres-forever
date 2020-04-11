#pragma once

#include "Common.h"

#include <glm/glm.hpp>

namespace bsf
{
	class ShaderProgram;
	class Texture2D;

	struct Material
	{
		glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> Map = nullptr;
		Ref<Texture2D> NormalMap = nullptr;
		Ref<Texture2D> MetallicMap = nullptr;
		Ref<Texture2D> RoughnessMap = nullptr;
		Ref<ShaderProgram> Program;

		virtual void Bind(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) = 0;
	};

	struct PBRMaterial : public Material
	{
		void Bind(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) override;
	};

}