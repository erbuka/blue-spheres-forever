#pragma once

#include <string_view>

#include "Ref.h"

namespace bsf
{

	class Model;
	struct ModelDef;

	class WavefrontLoader
	{
	public:
		WavefrontLoader() = default;
		Ref<ModelDef> Load(std::string_view fileName);
	};
}

