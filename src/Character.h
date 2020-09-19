#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "Asset.h"
#include "Ref.h"
#include "GLTF.h"

namespace bsf
{
	struct Character : public Asset
	{
		glm::mat4 Matrix = glm::identity<glm::mat4>();
		GLTF Model;
	};
}