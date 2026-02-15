project "jolt"
	kind "StaticLib"
	properties	{ "thirdpartylib" }

	includedirs {
		"./"
	}

	files {
		"./Jolt/AABBTree/*",
		"./Jolt/ConfigurationString.h",
		"./Jolt/Core/*",
		"./Jolt/Geometry/*",
		"./Jolt/Jolt.h",
		"./Jolt/Math/*",
		"./Jolt/Physics/Body/*",
		"./Jolt/Physics/Character/*",
		"./Jolt/Physics/Collision/*",
		"./Jolt/Physics/Collision/Shape/*",
		"./Jolt/Physics/Constraints/*",
		"./Jolt/Physics/*",
		"./Jolt/Physics/Ragdoll/*",
		--"./Jolt/Physics/SoftBody/*",
		--"./Jolt/Physics/Vehicle/*",
		"./Jolt/*",
		"./Jolt/Renderer/*",
		"./Jolt/Skeleton/*",
		"./Jolt/TriangleSplitter/*",
	}
	
	defines {
		--"JPH_DOUBLE_PRECISION",
		--"JPH_DEBUG_RENDERER",					-- Enable the debug renderer
		--"JPH_EXTERNAL_PROFILE",				-- Enable the profiler
		--"JPH_PROFILE_ENABLED",
		--"JPH_CROSS_PLATFORM_DETERMINISTIC",	-- Setting to attempt cross platform determinism

		-- custom defines to disable registration of vehicles and softbodies
		"JPH_INSBYTE_NO_VEHICLES",
		"JPH_INSBYTE_NO_SOFTBODY"
	}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		defines {
			"JPH_ENABLE_ASSERTS"
		}

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		defines {
			"JPH_ENABLE_ASSERTS"
		}

	filter "configurations:ReleaseAsan"
		runtime "Release"
		optimize "on"
		defines {
			"JPH_ENABLE_ASSERTS",
			"JPH_DISABLE_TEMP_ALLOCATOR",
			"JPH_DISABLE_CUSTOM_ALLOCATOR",
		}

usage "jolt"
	includedirs { 
		"./"
	}
	links "jolt"
