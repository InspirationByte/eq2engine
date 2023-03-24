project "SDL2"
	kind "SharedLib"
	
	includedirs
	{
		"./include",
	}
	
	disablewarnings {
		"unused-parameter",
		"sign-compare"
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
		"-Wshift-sign-overflow",
		"-Wstrict-prototypes",
		"-Wkeyword-macro",
	}

	defines
	{
		"GL_GLEXT_PROTOTYPES"
	}

	files
	{
		"./src/*.c",
		"./src/audio/*.c",
		"./src/audio/dummy/*.c",
		"./src/audio/aaudio/*.c",
		"./src/audio/openslES/*.c",
		"./src/atomic/*.c",
		"./src/cpuinfo/*.c",
		"./src/dynapi/*.c",
		"./src/events/*.c",
		"./src/file/*.c",
		"./src/haptic/*.c",
		"./src/hidapi/*.c",
		"./src/joystick/*.c",
		"./src/joystick/hidapi/*.c",
		"./src/joystick/virtual/*.c",
		"./src/loadso/dlopen/*.c",
		"./src/locale/*.c",
		"./src/misc/*.c",
		"./src/power/*.c",
		"./src/sensor/*.c",
		"./src/render/*.c",
		"./src/render/*/*.c",
		"./src/stdlib/*.c",
		"./src/thread/*.c",
		"./src/thread/pthread/*.c",
		"./src/timer/*.c",
		"./src/timer/unix/*.c",
		"./src/video/*.c",
		"./src/video/yuv2rgb/*.c",
		"./src/test/*.c"
	}

	filter "system:android"
		files {
			"./src/audio/android/*.c",
			"./src/core/android/*.c",
			"./src/haptic/android/*.c",
			"./src/hidapi/android/*.cpp",
			"./src/joystick/android/*.c",
			"./src/locale/android/*.c",
			"./src/misc/android/*.c",
			"./src/power/android/*.c",
			"./src/filesystem/android/*.c",
			"./src/sensor/android/*.c",
			"./src/video/android/*.c",
		}
		links {
			"dl", "GLESv1_CM", "GLESv2", "OpenSLES", "log", "android",
			"cpufeatures"
		}
		includedirs {
			"$(NDK_ROOT)/sources/android/cpufeatures"
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		
-- NOTE: android only
project "cpufeatures"
	kind "StaticLib"
	includedirs {
		"$(NDK_ROOT)/sources/android/cpufeatures",
	}
	
	files {
		"$(NDK_ROOT)/sources/android/cpufeatures/cpu-features.c" 
	}
	
-- NOTE: android only
project "SDL2_main"
	kind "StaticLib"
	includedirs{
		"./include",
	}
	
	files {
		"./src/main/android/SDL_android_main.c" 
	}

-- SDL2 as a usage
usage "SDL2"
	links 
	{
		"SDL2"
	}
	
	filter "system:Linux"
		includedirs {
            "/usr/include/SDL2"
        }
		
	filter "system:Android"
		links {
			"SDL2_main"
		}
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