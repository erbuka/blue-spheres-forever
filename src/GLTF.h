#pragma once

#include <memory>

#include "Ref.h"
#include "Common.h"

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
		uint64_t ModelMatrixUniform = HS("uModel");
		uint64_t BaseColorTextureUniform = HS("uMap");
		uint64_t BaseColorUniform = HS("uColor");
		uint64_t JointTransformUniform = HS("uJointTransform[0]");
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
		void Update(const Time& time);

		void StopAllAnimations();
		void PlayAnimation(std::string_view name, bool loop = true, float timeWarp = 1.0f);
		void FadeToAnimation(std::string_view next, float fadeTime, bool loop = true, float timeWarp = 1.0f);

		void SetAnimationGlobalTimeWarp(float timeWarp);

	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
	};
}