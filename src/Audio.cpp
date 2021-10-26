#include "BsfPch.h"
#include "Audio.h"
#include "Log.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>


namespace bsf
{
	
	static constexpr ma_uint32 s_sampleRate = 44100;
	static constexpr ma_uint32 s_channels = 2;
	static constexpr auto s_format = ma_format_f32;
	static constexpr float s_frameTime = 1.0f / s_sampleRate;

	static void DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
	
	struct Audio::Impl {
		bool m_Fading = false;
		float m_TotalFadeTime = 0.0f;
		float m_FadeTime = 0.0f;
		bool m_Playing = false;
		bool m_Loop = false;
		float m_LoopPoint = 0.0f;
		float m_Volume = 1.0f;
		ma_decoder m_Decoder;
		Impl(std::string_view fileName);
		~Impl();
		void Play();
		void Stop();
		void FadeOut(float time);
		void ResetLoop();
		void SetLoop(float loopPoint);
	};

	struct AudioDevice::Impl
	{
		ma_device m_Device;
		Impl();
		~Impl();
	};

	static std::vector<Audio::Impl*> m_Streams;

	AudioDevice::Impl::Impl() {
		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.format = s_format;   // Set to ma_format_unknown to use the device's native format.
		config.playback.channels = s_channels;               // Set to 0 to use the device's native channel count.
		config.sampleRate = s_sampleRate;           // Set to 0 to use the device's native sample rate.
		config.dataCallback = DataCallback;   // This function will be called when miniaudio needs more data.
		config.pUserData = this;   // Can be accessed from the device object (device.pUserData).

		if (ma_device_init(NULL, &config, &m_Device) != MA_SUCCESS) {
			BSF_ERROR("Cannot initialize audio device");
			return;
		}

		ma_device_start(&m_Device);     // The device is sleeping by default so you'll need to start it manually.


		BSF_INFO("Audio device started");

	}

	AudioDevice::Impl::~Impl() {
		// Do something here. Probably your program's main loop.
		ma_device_uninit(&m_Device);    // This will stop the device so no need to do that manually.
		BSF_INFO("Audio device stopped");
	}

	Audio::Impl::Impl(std::string_view fileName)
	{
		ma_decoder_config config = ma_decoder_config_init(s_format, s_channels, s_sampleRate);
		if (ma_decoder_init_file(fileName.data(), &config, &m_Decoder) != MA_SUCCESS) 
		{
			BSF_ERROR("Can't open audio file: {0}", fileName.data());
		}

		m_Decoder.pUserData = this;

		m_Streams.push_back(this);

		BSF_INFO("Loaded audio file: {0}", fileName.data());
	}
	Audio::Impl::~Impl() 
	{
		auto& streams = m_Streams;
		streams.erase(std::remove(streams.begin(), streams.end(), this), streams.end());
		ma_decoder_uninit(&m_Decoder);
	}

	void Audio::Impl::Play() {
		ma_decoder_seek_to_pcm_frame(&m_Decoder, 0);
		m_Playing = true;
		m_Fading = false;
	}

	void Audio::Impl::Stop() {
		m_Playing = false;
	}


	void Audio::Impl::FadeOut(float time)
	{
		if (m_Playing)
		{
			m_Fading = true;
			m_FadeTime = time;
			m_TotalFadeTime = time;
		}
	}

	void Audio::Impl::ResetLoop()
	{
		m_Loop = false;
	}

	void Audio::Impl::SetLoop(float loopPoint)
	{
		m_Loop = true;
		m_LoopPoint = loopPoint;
	}

	static void DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
	{
		// In playback mode copy data to pOutput. In capture mode read data from pInput. In full-duplex mode, both
		// pOutput and pInput will be valid and you can move data from pInput into pOutput. Never process more than
		// frameCount frames.

		static std::vector<float> m_Buffer;

		auto& streams = m_Streams;

		float* fOutput = (float*)pOutput;
		float* fInput = (float*)pInput;

		m_Buffer.resize(frameCount * s_channels);

		const auto readFrames = [](ma_uint32 max, Audio::Impl* stream) -> ma_uint32 {
			return ma_decoder_read_pcm_frames(&stream->m_Decoder, static_cast<void*>(m_Buffer.data()), max);
		};
		
		const auto mixFrames = [&](const ma_uint32 count, const ma_uint32 offset, Audio::Impl* stream) {
			for (size_t i = 0; i < count; ++i)
			{
				float volume = stream->m_Volume;

				if (stream->m_Fading)
				{
					stream->m_FadeTime = std::max(0.0f, stream->m_FadeTime - s_frameTime);
					volume = stream->m_Volume * stream->m_FadeTime / stream->m_TotalFadeTime;
				}

				fOutput[(offset + i) * s_channels] += m_Buffer[i * s_channels] * volume;
				fOutput[(offset + i) * s_channels + 1] += m_Buffer[i * s_channels + 1] * volume;
			}
		};

		for (auto s : streams)
		{
			if (!s->m_Playing)
				continue;

			auto currentFrameCount = readFrames(frameCount, s);
			mixFrames(currentFrameCount, 0, s);

			if (currentFrameCount < frameCount)
			{
				if (s->m_Loop) 
				{
					const auto offset = currentFrameCount;
					const ma_uint32 loopFrameIdx = ma_uint32(s->m_LoopPoint * s_sampleRate);
					ma_decoder_seek_to_pcm_frame(&s->m_Decoder, loopFrameIdx);
					currentFrameCount = readFrames(frameCount - currentFrameCount, s);
					mixFrames(currentFrameCount, offset, s);
				}
				else
				{
					s->Stop();
				}
			}

			if (s->m_Fading && s->m_FadeTime == 0.0f)
				s->Stop();


		}

	}

	#pragma region Interface


	AudioDevice::AudioDevice()
	{
		m_Impl = std::make_unique<Impl>();
	}

	AudioDevice::~AudioDevice()
	{
		m_Impl.reset();
	}

	Audio::Audio(std::string_view fileName)
	{
		m_Impl = std::make_unique<Impl>(fileName);
	}

	Audio::~Audio()
	{
		m_Impl.reset();
	}

	void Audio::SetVolume(float volume) { m_Impl->m_Volume = volume;}
	float Audio::GetVolume() const { return m_Impl->m_Volume;}
	void Audio::SetLoop(float loopPoint) { m_Impl->SetLoop(loopPoint); }
	void Audio::ResetLoop() { m_Impl->ResetLoop(); }
	bool Audio::IsLooping() const { return m_Impl->m_Loop; }
	float Audio::GetLoopPoint() const { return m_Impl->m_LoopPoint; }
	void Audio::Play() { m_Impl->Play(); }
	void Audio::Stop() { m_Impl->Stop(); }
	void Audio::FadeOut(float time) { m_Impl->FadeOut(time); }

	#pragma endregion

}


