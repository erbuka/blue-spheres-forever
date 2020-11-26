#pragma once

#include "Asset.h"

#include <string_view>
#include <memory>

namespace bsf
{

	class AudioDevice
	{
	public:
		AudioDevice();
		~AudioDevice();
	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
	};	


	class Audio : public Asset
	{
	public:
		Audio(std::string_view fileName);
		~Audio();

		void SetVolume(float volume);
		float GetVolume() const;

		void SetLoop(float loopPoint);
		void ResetLoop();

		bool IsLooping() const;
		float GetLoopPoint() const;

		void Play();
		void Stop();
		void FadeOut(float time);


	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
	};


}

