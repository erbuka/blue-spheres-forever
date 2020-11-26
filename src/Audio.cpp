#include "BsfPch.h"
#include "Audio.h"
#include "Log.h"

#include <bass/bass.h>

#define BASS_CHECK(x) if((x) == FALSE) BSF_ERROR("BASS Error at {0}:{1}", __FILE__, __LINE__)  


namespace bsf
{

	
	static void CALLBACK Sync_End(HSYNC handle, DWORD channel, DWORD data, void* user)
	{
		Audio* audio = static_cast<Audio*>(user);
		QWORD pos = BASS_ChannelSeconds2Bytes(channel, audio->GetLoopPoint());
		BASS_ChannelSetPosition(channel, pos, BASS_POS_BYTE);
	}

	static void CALLBACK Sync_Slide(HSYNC handle, DWORD channel, DWORD data, void* user)
	{
		Audio* audio = static_cast<Audio*>(user);
		if (handle == BASS_SYNC_SLIDE)
		{
			if (data == BASS_ATTRIB_VOL)
			{
				if (audio->GetVolume() == 0.0f)
					audio->Stop();
			}
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
		HSYNC m_LoopSync = 0;
		bool m_Loop = false;
		float m_LoopPoint = 0.0f;
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
		if ((m_Impl->m_Stream = BASS_StreamCreateFile(FALSE, fileName.data(), 0, 0, BASS_STREAM_PRESCAN)) == 0)
		{
			BSF_ERROR("Can't create stream from file: {0}", fileName);
			return;
		}

		BASS_ChannelSetSync(m_Impl->m_Stream, BASS_SYNC_MIXTIME | BASS_SYNC_SLIDE, 0, &Sync_Slide, (void*)this);
		
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

	void Audio::SetLoop(float loopPoint)
	{
		ResetLoop();

		m_Impl->m_Loop = true;
		m_Impl->m_LoopPoint = loopPoint;
		m_Impl->m_LoopSync = BASS_ChannelSetSync(m_Impl->m_Stream, BASS_SYNC_MIXTIME | BASS_SYNC_END, 0, &Sync_End, (void*)this);

	}

	void Audio::ResetLoop()
	{
		if (m_Impl->m_LoopSync)
		{
			BASS_ChannelRemoveSync(m_Impl->m_Stream, m_Impl->m_LoopSync);
			m_Impl->m_LoopSync = 0;
		}

		m_Impl->m_Loop = false;
	}

	bool Audio::IsLooping() const
	{
		return m_Impl->m_Loop;
	}

	float Audio::GetLoopPoint() const
	{
		return m_Impl->m_LoopPoint;
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