workspace "BlueSpheresForever"
    architecture "x86_64"
    configurations { "Debug", "Release", "Distribution" }
    startproject "BlueSpheresForever"
    location(_ACTION)

    filter "configurations:Debug"
        defines { "DEBUG", "BSF_ENABLE_DIAGNOSTIC", "BSF_SAFE_GLCALL" }
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines { "BSF_ENABLE_DIAGNOSTIC", "BSF_SAFE_GLCALL" }
        symbols "On"
        optimize "On"

    filter "configurations:Distribution"
        defines { "NDEBUG", "BSF_DISTRIBUTION" }
        symbols "Off"
        optimize "On"

    filter "system:windows"
        systemversion "latest"

project "Glad"
    location(_ACTION)
    kind "StaticLib"
    language "C"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs { "vendor/glad/include" }
    files { "vendor/glad/src/**.c" }

project "GLFW"
    location(_ACTION)
    kind "StaticLib"
    language "C"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    defines { "GLFW_STATIC" }

    includedirs {
        "vendor/glfw/src"
    }
    files {
        "vendor/glfw/src/context.c",
        "vendor/glfw/src/init.c",
        "vendor/glfw/src/input.c",
        "vendor/glfw/src/monitor.c",
        "vendor/glfw/src/osmesa_context.c",
        "vendor/glfw/src/vulkan.c",
        "vendor/glfw/src/window.c",
        "vendor/glfw/src/xkb_unicode.c",
    }

    defines { "_CRT_SECURE_NO_WARNINGS" }

    filter "system:windows"
        defines { "_GLFW_WIN32" }
        files {
            "vendor/glfw/src/wgl_context.c",
            "vendor/glfw/src/win32_init.c",
            "vendor/glfw/src/win32_joystick.c",
            "vendor/glfw/src/win32_monitor.c",
            "vendor/glfw/src/win32_thread.c",
            "vendor/glfw/src/win32_time.c",
            "vendor/glfw/src/win32_window.c",
        }

    filter "system:linux"
        defines { "_GLFW_X11" }
        files {
            "vendor/glfw/src/egl_context.c",
            "vendor/glfw/src/glx_context.c",
            "vendor/glfw/src/x11_init.c",
            "vendor/glfw/src/x11_monitor.c",
            "vendor/glfw/src/x11_window.c",
            "vendor/glfw/src/posix_thread.c",
            "vendor/glfw/src/posix_time.c",
            "vendor/glfw/src/linux_joystick.c",
        }


project "ImGui"
    location(_ACTION)
    kind "StaticLib"
    language "C++"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs {
        "vendor/imgui",
        "vendor/imgui/examples",
        "vendor/glfw/include",
        "vendor/glad/include"
    }

    defines { "IMGUI_IMPL_OPENGL_LOADER_GLAD" }

    files {
        "vendor/imgui/*.cpp",
        "vendor/imgui/examples/imgui_impl_glfw.cpp",
        "vendor/imgui/examples/imgui_impl_opengl3.cpp",
     }

project "BlueSpheresForever"
    location(_ACTION)
    language "C++"
    cppdialect "C++17"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs {
        "vendor/glm",
        "vendor/glad/include",
        "vendor/glfw/include",
        "vendor/spdlog/include",
        "vendor/stb/include",
        "vendor/json/include",
        "vendor/fmt/include",
        "vendor/miniaudio"
    }


    defines { "SPDLOG_FMT_EXTERNAL", "FMT_HEADER_ONLY" }

    pchheader "src/BsfPch.h"
    pchsource "src/BsfPch.cpp"

    files { "src/**.cpp", "src/**.h"  }

    links { "Glad", "GLFW" }

    postbuildcommands {
        "{COPY} ../src/assets ../bin/%{cfg.buildcfg}/%{prj.name}/assets"
    }

    filter "system:windows"
        links { "opengl32" }

    filter "system:linux"
        links { "GL", "X11", "dl", "pthread" }

    filter "configurations:not Distribution"
        kind "ConsoleApp"
        includedirs {
            "vendor/imgui",
            "vendor/imgui/examples",
        }
        links { "ImGui" }

    filter "configurations:Distribution"
        kind "ConsoleApp"
