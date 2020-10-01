#pragma once

#include <spdlog\spdlog.h>

#define BSF_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define BSF_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define BSF_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define BSF_ERROR(...) SPDLOG_ERROR(__VA_ARGS__), throw std::runtime_error("Error")

#ifdef BSF_SAFE_GLCALL
#define BSF_GLCALL(x) (x); if(auto err = glGetError(); err != GL_NO_ERROR) BSF_ERROR("OpenGL Error ({0})", err)
#else
#define BSF_GLCALL(x) (x)
#endif



