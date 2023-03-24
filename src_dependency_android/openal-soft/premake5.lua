project "openal_soft"
	kind "StaticLib"
	language "C++"
	--cppdialect "C++17"

	includedirs
	{
		"./",
		"./include",
		"./al",
		"./alc",
		"./common",	
		"./core",
		"./build-android"
	}

	files
	{
		"./common/**.h",
		"./common/**.cpp",
		"./core/**.h",
		"./core/**.cpp",
		"./al/**.h",
		"./al/**.cpp",
		"./alc/*.h",
		"./alc/*.cpp",
		"./alc/effects/*.h",
		"./alc/effects/*.cpp",
		"./alc/backends/base.cpp",
		"./alc/backends/loopback.cpp",
		"./alc/backends/null.cpp"
	}

	defines
	{
		"AL_LIBTYPE_STATIC"
	}

	-- config does not define HAVE_RTKIT
	removefiles {
		"./core/dbus_wrap.h",
		"./core/dbus_wrap.cpp",
		"./core/rtkit.h",
		"./core/rtkit.cpp",
	}

	filter "architecture:arm or arm64"
		defines {
			"HAVE_NEON"
		}
  		removefiles {
			"./core/mixer/mixer_sse.cpp",
			"./core/mixer/mixer_sse2.cpp",
			"./core/mixer/mixer_sse3.cpp",
			"./core/mixer/mixer_sse41.cpp"
		}

	filter "architecture:x86 or x86_64"
		defines {
			-- NOTE: this is temporary, we would need to have them
			-- "HAVE_SSE"
			-- "HAVE_SSE2"
			-- "HAVE_SSE3"
			-- "HAVE_SSE4_1"
		}
		removefiles {
			"./core/mixer/mixer_neon.cpp",

			-- NOTE: this is temporary, we would need to have them
			"./core/mixer/mixer_sse.cpp",
			"./core/mixer/mixer_sse2.cpp",
			"./core/mixer/mixer_sse3.cpp",
			"./core/mixer/mixer_sse41.cpp"
		}

	-- TODO: filter "system:windows"
	
	filter "system:android"		
		files {
			"./alc/backends/opensl.cpp",
		}
		
		defines
		{
			"HAVE_OPENSL",
			"ALSOFT_REQUIRE_OPENSL",
			"RESTRICT=__restrict",
			"NOMINMAX",
			"AL_ALEXT_PROTOTYPES"
		}

		links {
			"OpenSLES"
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

usage "openal-soft"
	includedirs {
		"include",
		"include/AL"
	}

	links "openal_soft"
