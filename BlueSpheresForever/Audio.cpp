#include "BsfPch.h"
#include "Audio.h"
#include "Log.h"

#include <bass/bass.h>

#define BASS_CHECK(x) if((x) == FALSE) BSF_ERROR("BASS Error at {0}:{1}", __FILE__, __LINE__)  


namespace bsf
{
	struct AudioDevice::Impl
	{
		Impl() { BASS_CHECK(BASS_Init(1, 44100, BASS_DEVICE_STEREO, 0, nullptr)); }
		~Impl() { BASS_CHECK(BASS_Free()); }
	};


	struct Audio::Impl
	{
		HSTREAM m_Stream = 0;
	};

	AudioDevice::AudioDevice()
	{
		m_Impl = std::make_unique<Impl>();
	}

	AudioDevice::~AudioDevice()
	{
		m_Impl = nullptr;
	}

	Audio::Audio(const std::string& fileName)
	{
		m_Impl = std::make_unique<Impl>();
		if ((m_Impl->m_Stream = BASS_StreamCreateFile(FALSE, fileName.c_str(), 0, 0, 0)) == 0)
		{
			BSF_ERROR("Can't create stream from file: {0}", fileName);
			return;
		}
	}

	Audio::~Audio()
	{
		if (m_Impl->m_Stream != 0)
			BASS_CHECK(BASS_StreamFree(m_Impl->m_Stream) == FALSE);

	}

	void Audio::Play()
	{
		BASS_CHECK(BASS_ChannelPlay(m_Impl->m_Stream, true));
	}

	void Audio::Stop()
	{
		BASS_CHECK(BASS_ChannelStop(m_Impl->m_Stream));
	}


}


#undef BASS_CHECK