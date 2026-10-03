project "tracy_client"
	language "C++"
	kind "StaticLib"
	properties	{ "thirdpartylib" }

	includedirs { "public" }

	files { "public/TracyClient.cpp" }

	filter { "configurations:not Retail" }
		defines { "TRACY_ENABLE" }
	
usage "tracy"
	includedirs { "public" }
	links { "tracy_client" }
	filter { "configurations:not Retail" }
		defines { "TRACY_ENABLE" }