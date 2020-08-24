#pragma once

#include "Ref.h"

namespace bsf
{

	class Model;
	struct ModelDef;

	class WavefrontLoader
	{
	public:
		WavefrontLoader() = default;
		Ref<ModelDef> Load(const std::string& fileName);
	};
}
