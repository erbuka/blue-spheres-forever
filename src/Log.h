#pragma once

#include "spdlog/spdlog.h"
#define BSF_INFO(...) spdlog::info(__VA_ARGS__)
#define BSF_WARN(...) spdlog::warn(__VA_ARGS__)
#define BSF_ERROR(...) spdlog::error(__VA_ARGS__)

#ifdef BSF_ENABLE_DIAGNOSTIC
#define BSF_DEBUG(...) spdlog::debug(__VA_ARGS__)
#define BSF_GLCALL(x) x; if(auto err = glGetError(); err != GL_NO_ERROR) { BSF_ERROR("OpenGL Error ({2}) at {0}:{1}", __FILE__, __LINE__, err); __debugbreak(); }  
#else
#define BSF_DEBUG(...)
#define BSF_GLCALL(x) x
#endif

