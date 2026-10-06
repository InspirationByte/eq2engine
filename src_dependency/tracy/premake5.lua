project "tracy_client"
	language "C++"
	kind "StaticLib"
	targetname "tracy"
	
	properties { "thirdpartylib" }
	includedirs { "public" }

	files { "public/TracyClient.cpp" }
	defines {
		"TRACY_EXPORTS", 
		"TRACY_ON_DEMAND",	-- profile only when debugger is connected
		"TRACY_ONLY_LOCALHOST",	-- we only allow localhost debugging atm
	}

	filter { "configurations:not Retail", "configurations:not Debug" }
		defines { "TRACY_ENABLE" }
	
property "tracy"
	includedirs { "public" }
	links "tracy_client"
	defines {
		"TRACY_ON_DEMAND",	-- profile only when debugger is connected
		"TRACY_ONLY_LOCALHOST",	-- we only allow localhost debugging atm
	}
	filter { "configurations:not Retail", "configurations:not Debug" }
		defines { "TRACY_ENABLE" }