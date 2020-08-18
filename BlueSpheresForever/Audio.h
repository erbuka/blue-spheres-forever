#pragma once

#include "Asset.h"

#include <string>
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
		Audio(const std::string& fileName);
		~Audio();

		void SetVolume(float volume);
		float GetVolume() const;

		void Play();
		void Stop();
		void FadeOut(float time);


	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
	};

}

