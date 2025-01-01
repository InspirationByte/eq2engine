project "RecastDebugUtils"
	language "C++"
	kind "StaticLib"
	properties	{ "thirdpartylib" }
	includedirs { 
		"DebugUtils/Include",
		"Detour/Include",
		"DetourTileCache/Include",
		"Recast/Include"
	}
	files { 
		"DebugUtils/Include/*.h",
		"DebugUtils/Source/*.cpp"
	}

project "RecastDetour"
	language "C++"
	kind "StaticLib"
	properties	{ "thirdpartylib" }
	includedirs { 
		"Detour/Include" 
	}
	files { 
		"Detour/Include/*.h", 
		"Detour/Source/*.cpp" 
	}
	-- linux library cflags and libs
	filter "system:linux"
		buildoptions { 
			"-Wno-class-memaccess"
		}


project "RecastDetourCrowd"
	language "C++"
	kind "StaticLib"
	properties	{ "thirdpartylib" }
	includedirs {
		"DetourCrowd/Include",
		"Detour/Include",
		"Recast/Include"
	}
	files {
		"DetourCrowd/Include/*.h",
		"DetourCrowd/Source/*.cpp"
	}

project "RecastDetourTileCache"
	language "C++"
	kind "StaticLib"
	properties	{ "thirdpartylib" }
	includedirs {
		"DetourTileCache/Include",
		"Detour/Include",
		"Recast/Include"
	}
	files {
		"DetourTileCache/Include/*.h",
		"DetourTileCache/Source/*.cpp"
	}
	links {
		"RecastDebugUtils",
		"RecastDetour"
	}

project "Recast"
	language "C++"
	kind "StaticLib"
	properties	{ "thirdpartylib" }
	includedirs { 
		"Recast/Include" 
	}
	files { 
		"Recast/Include/*.h",
		"Recast/Source/*.cpp" 
	}
	links {
		"RecastDebugUtils",
		"RecastDetour",
		"RecastDetourCrowd",
		"RecastDetourTileCache"
	}

usage "Recast"
	includedirs { 
		"DebugUtils/Include",
		"Detour/Include",
		"DetourCrowd/Include",
		"DetourTileCache/Include",
		"Recast/Include",
		"Recast/Source"
	}
	links { 
		"RecastDebugUtils",
		"RecastDetour",
		"RecastDetourCrowd",
		"RecastDetourTileCache",
		"Recast"
	}


if false and not IS_ANDROID then

project "RecastDemo"
	language "C++"
	kind "WindowedApp"
	properties	{ "thirdpartylib" }
	uses { "SDL2", "SDL2_main", "lz4" }
	includedirs { 
		"RecastDemo/Include",
		"RecastDemo/Contrib",
		"DebugUtils/Include",
		"Detour/Include",
		"DetourCrowd/Include",
		"DetourTileCache/Include",
		"Recast/Include"
	}
	files { 
		"RecastDemo/Include/*.h",
		"RecastDemo/Source/*.cpp"
	}

	-- project dependencies
	links { 
		"RecastDebugUtils",
		"RecastDetour",
		"RecastDetourCrowd",
		"RecastDetourTileCache",
		"Recast"
	}

	-- windows library cflags and libs
	filter "system:windows"
		links { 
			"glu32",
			"opengl32"
		}

project "RecastTests"
	language "C++"
	kind "ConsoleApp"
	properties	{ "thirdpartylib" }
	uses { "SDL2", "SDL2_main" }

	includedirs { 
		"DebugUtils/Include",
		"Detour/Include",
		"DetourCrowd/Include",
		"DetourTileCache/Include",
		"Recast/Include",
		"Recast/Source",
		"Tests/Recast",
		"Tests",
	}
	files	{ 
		"Tests/*.h",
		"Tests/*.hpp",
		"Tests/*.cpp",
		"Tests/Recast/*.h",
		"Tests/Recast/*.cpp",
		"Tests/Detour/*.h",
		"Tests/Detour/*.cpp",
	}

	-- project dependencies
	links { 
		"RecastDebugUtils",
		"RecastDetour",
		"RecastDetourCrowd",
		"RecastDetourTileCache",
		"Recast"
	}

	-- windows library cflags and libs
	filter "system:windows"
		links { 
			"glu32",
			"opengl32",
		}

end