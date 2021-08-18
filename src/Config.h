#include <vector>

#include "Common.h";

namespace bsf
{

	struct DisplayModeDescriptor
	{
		uint32_t Width, Height;
		const std::string ToString() const;
	};

	bool operator==(const DisplayModeDescriptor& a, const DisplayModeDescriptor& b);

	std::vector<DisplayModeDescriptor> GetDisplayModes();

	struct Config
	{
		DisplayModeDescriptor DisplayMode;
		bool Fullscreen;
		bool Save() const;
		static Config Load();
	};



	namespace GlobalShadingConfig
	{
		inline float SkyExposure = 1.0f;
		inline float DeferredExposure = 1.0f;

		inline float LightRadiance = 5.0f;
	}

}