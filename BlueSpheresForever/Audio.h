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

		void Play();
		void Stop();

	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
	};

}

