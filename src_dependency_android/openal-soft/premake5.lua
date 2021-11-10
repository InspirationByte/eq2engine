project "openal_soft"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"

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
	
	filter "system:android"
		-- TODO: detect abi
		removefiles {
			"./core/mixer/mixer_sse.cpp",
			"./core/mixer/mixer_sse2.cpp",
			"./core/mixer/mixer_sse41.cpp"
		}
		
		files {
			"./alc/backends/opensl.cpp",
		}
		
		defines
		{
			"ALSOFT_REQUIRE_OPENSL",
			"RESTRICT=__restrict",
			"NOMINMAX",
			"AL_ALEXT_PROTOTYPES"
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
