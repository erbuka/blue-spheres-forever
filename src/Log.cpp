#include "BsfPch.h"

#include <spdlog\sinks\basic_file_sink.h>

#include "Log.h"

namespace bsf
{
	static constexpr std::string_view s_LogFile = "assets/log.txt";

	void InitializeFileLog()
	{
		auto logger = spdlog::basic_logger_mt("default_log", s_LogFile.data(), true);
		spdlog::set_default_logger(logger);
	}

}

