#include "BsfPch.h"
#include "Audio.h"
#include "Log.h"

#include <bass/bass.h>

#define BASS_CHECK(x) if((x) == FALSE) BSF_ERROR("BASS Error at {0}:{1}", __FILE__, __LINE__)  


namespace bsf
{

	static void CALLBACK Sync_SlideAttribute(HSYNC handle, DWORD channel, DWORD data, void* user)
	{
		if (data == BASS_ATTRIB_VOL)
		{
			Audio* audio = static_cast<Audio*>(user);
			if (audio->GetVolume() == 0.0f)
				audio->Stop();
		}

	}

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

	Audio::Audio(std::string_view fileName)
	{
		m_Impl = std::make_unique<Impl>();
		if ((m_Impl->m_Stream = BASS_StreamCreateFile(FALSE, fileName.data(), 0, 0, 0)) == 0)
		{
			BSF_ERROR("Can't create stream from file: {0}", fileName);
			return;
		}

		BASS_ChannelSetSync(m_Impl->m_Stream, BASS_SYNC_MIXTIME, BASS_SYNC_SLIDE, &Sync_SlideAttribute, (void*)this);
	}

	Audio::~Audio()
	{
		if (m_Impl->m_Stream != 0)
			BASS_CHECK(BASS_StreamFree(m_Impl->m_Stream) == FALSE);

	}

	void Audio::SetVolume(float volume)
	{
		BASS_CHECK(BASS_ChannelSetAttribute(m_Impl->m_Stream, BASS_ATTRIB_VOL, volume));
	}

	float Audio::GetVolume() const
	{
		float volume;
		BASS_CHECK(BASS_ChannelGetAttribute(m_Impl->m_Stream, BASS_ATTRIB_VOL, &volume));
		return volume;
	}

	void Audio::Play()
	{
		SetVolume(1.0f);
		BASS_CHECK(BASS_ChannelPlay(m_Impl->m_Stream, true));
	}

	void Audio::Stop()
	{
		BASS_CHECK(BASS_ChannelStop(m_Impl->m_Stream));
	}

	void Audio::FadeOut(float time)
	{
		if(BASS_ChannelIsActive(m_Impl->m_Stream))
			BASS_CHECK(BASS_ChannelSlideAttribute(m_Impl->m_Stream, BASS_ATTRIB_VOL, 0.0f, time * 1000));
	}


}


#undef BASS_CHECK