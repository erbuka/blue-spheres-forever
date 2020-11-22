#include <vector>

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


}