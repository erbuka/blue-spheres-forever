#pragma once

#include "spdlog/spdlog.h"

#ifdef DEBUG
	#define BSF_INFO(...) spdlog::info(__VA_ARGS__)
	#define BSF_WARN(...) spdlog::warn(__VA_ARGS__)
	#define BSF_ERROR(...) spdlog::error(__VA_ARGS__)

	#define BSF_GLCALL(x) x; if(auto err = glGetError(); err != GL_NO_ERROR) BSF_ERROR("OpenGL Error ({2}) at {0}:{1}", __FILE__, __LINE__, err) 
#else
	#define BSF_INFO(...) spdlog::info(__VA_ARGS__)
	#define BSF_WARN(...) spdlog::warn(__VA_ARGS__)
	#define BSF_ERROR(...) spdlog::error(__VA_ARGS__)

	#define BSF_GLCALL(x) x; if(auto err = glGetError(); err != GL_NO_ERROR) BSF_ERROR("OpenGL Error ({2}) at {0}:{1}", __FILE__, __LINE__, err) 
#endif
