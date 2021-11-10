project "SDL2"
	kind "StaticLib"
	
	includedirs
	{
		"./include",
	}

	files
	{
		"./src/*.c",
		"./src/audio/*.c",
		"./src/audio/android/*.c",
		"./src/audio/dummy/*.c",
		"./src/audio/openslES/*.c",
		"./src/atomic/SDL_atomic.c.arm",
		"./src/atomic/SDL_spinlock.c.arm",
		"./src/core/android/*.c",
		"./src/cpuinfo/*.c",
		"./src/dynapi/*.c",
		"./src/events/*.c",
		"./src/file/*.c",
		"./src/haptic/*.c",
		"./src/haptic/android/*.c",
		"./src/joystick/*.c",
		"./src/joystick/android/*.c",
		"./src/joystick/hidapi/*.c",
		"./src/loadso/dlopen/*.c",
		"./src/power/*.c",
		"./src/power/android/*.c",
		"./src/filesystem/android/*.c",
		"./src/sensor/*.c",
		"./src/sensor/android/*.c",
		"./src/render/*.c",
		"./src/render/*/*.c",
		"./src/stdlib/*.c",
		"./src/thread/*.c",
		"./src/thread/pthread/*.c",
		"./src/timer/*.c",
		"./src/timer/unix/*.c",
		"./src/video/*.c",
		"./src/video/android/*.c",
		"./src/video/yuv2rgb/*.c",
		"./src/test/*.c",
		"./src/hidapi/android/hid.cpp"
	}

	disablewarnings {
		"unused-parameter",
		"no-sign-compare"
	}

	links {
		-- TODO: hidapi
		"dl", "GLESv1_CM", "GLESv2", "OpenSLES", "log", "android"
	}

	buildoptions {
		"-Wall",
		"-Wextra",
		"-Wdocumentation",
		"-Wdocumentation-unknown-command",
		"-Wmissing-prototypes",
		"-Wunreachable-code-break",
		"-Wunneeded-internal-declaration",
		"-Wmissing-variable-declarations",
		"-Wfloat-conversion",
		"-Wshorten-64-to-32",
		"-Wunreachable-code-return",
	}

	defines
	{
		"GL_GLEXT_PROTOTYPES",
	}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

-- SDL2 as a usage
usage "SDL2"
	links {
		"SDL2",
	}
	
	filter "system:Linux"
		includedirs {
            "/usr/include/SDL2"
        }
		
	filter "system:Android"
		includedirs {
			"./include"
		}

	filter "system:Windows"
		includedirs {
			"./include"
		}
		libdirs { 
			"./lib/%{cfg.platform}",
		}