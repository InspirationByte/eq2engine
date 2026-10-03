project "tracy_client"
	language "C++"
	kind "SharedLib"
	targetname "tracy"
	
	targetdir "%{_MAIN_SCRIPT_DIR}/build/bin/%{cfg.platform}/%{cfg.buildcfg}"
	--properties	{ "thirdpartylib" }
	includedirs { "public" }

	files { "public/TracyClient.cpp" }
	defines { "TRACY_EXPORTS" }

	filter { "configurations:not Retail" }
		defines { "TRACY_ENABLE" }
	
usage "tracy"
	includedirs { "public" }
	links { "tracy_client" }
	filter { "configurations:not Retail" }
		defines { "TRACY_ENABLE", "TRACY_IMPORTS" }