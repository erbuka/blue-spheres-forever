#include "BsfPch.h"

#include <json/json.hpp>

#include "Config.h"
#include "Log.h"
#include "Common.h"

namespace bsf
{
	using namespace nlohmann;

	static constexpr std::string_view s_ConfigFile = "assets/config.json";

	void to_json(json& json, const DisplayModeDescriptor& mode)
	{
		json = json::object();

		json["width"] = mode.Width;
		json["height"] = mode.Height;

	}

	void from_json(const json& json, DisplayModeDescriptor& mode)
	{
		mode.Width = json["width"].get<uint32_t>();
		mode.Height = json["height"].get<uint32_t>();
	}

	void to_json(json& json, const Config& config)
	{
		json = json::object();
		json["displayMode"] = config.DisplayMode;
		json["fullscreen"] = config.Fullscreen;
	}

	void from_json(const json& json, Config& mode)
	{
		mode.DisplayMode = json["displayMode"].get<DisplayModeDescriptor>();
		mode.Fullscreen = json["fullscreen"].get<bool>();
	}

	bool operator==(const DisplayModeDescriptor& a, const DisplayModeDescriptor& b)
	{
		return a.Width == b.Width &&
			a.Height == b.Height;
	}

	std::vector<DisplayModeDescriptor> GetDisplayModes()
	{
		std::vector<DisplayModeDescriptor> result;
		int32_t count;
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* videoModes = glfwGetVideoModes(monitor, &count);

		for (size_t i = 0; i < count; ++i)
		{
			// At least 60hz and 800px wide
			if (videoModes[i].refreshRate < 60 || videoModes[i].width < 800)
				continue;

			DisplayModeDescriptor mode;

			mode.Width = videoModes[i].width;
			mode.Height = videoModes[i].height;

			result.push_back(std::move(mode));

		}

		return result;
	}


	bool Config::Save() const
	{
		std::ofstream of;

		of.open(s_ConfigFile.data());

		if (!of.is_open())
		{
			BSF_ERROR("Couldn't save configuration file");
			return false;
		}

		json obj = *this;

		of << obj.dump();

		of.close();

		return true;
	}

	Config Config::Load()
	{
		namespace fs = std::filesystem;

		if (!fs::is_regular_file(s_ConfigFile))
		{
			Config config;

			config.DisplayMode = GetDisplayModes()[0];
			config.Fullscreen = false;

			config.Save();

			return config;

		}
		else
		{
			Config result;
			auto obj = json::parse(ReadTextFile(s_ConfigFile));
			from_json(obj, result);
			return result;
		}
	}

	const std::string DisplayModeDescriptor::ToString() const
	{
		return fmt::format("{0} x {1}", Width, Height);
	}
}
