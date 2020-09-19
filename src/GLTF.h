#pragma once

#include <memory>

#include "Ref.h"

namespace bsf
{
	class ShaderProgram;
	struct Time;

	enum class GLTFAttributes {
		Position,
		Normal,
		Uv,
		Joints_0,
		Weights_0
	};

	struct GLTFRenderConfig
	{
		Ref<ShaderProgram> Program;
		std::string ModelMatrixUniform = "uModel";
		std::string BaseColorTextureUniform = "uTexture";
		std::string BaseColorUniform = "uColor";
	};


	class GLTF
	{
	public:

		GLTF();
		~GLTF();

		GLTF(const GLTF&) = delete;
		GLTF& operator=(const GLTF&) = delete;

			 
		bool Load(std::string_view fileName, const std::initializer_list<GLTFAttributes>& attribs);
		void Render(const Time& time, const GLTFRenderConfig& config);

		void PlayAnimation(std::string_view name, bool loop = true, float timeWarp = 1.0f);
		void FadeToAnimation(std::string_view next, float fadeTime, bool loop = true, float timeWarp = 1.0f);
	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
	};
}