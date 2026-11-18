-- msdfgen core v1.13 (https://github.com/Chlumsky/msdfgen), MIT license
-- only the core library: no FreeType / Skia / PNG extensions

project "msdfgen"
	kind "StaticLib"
	properties	{ "thirdpartylib" }
	defines {
		"MSDFGEN_PUBLIC=",
		"MSDFGEN_USE_CPP11"
	}
	includedirs {
		"./"
	}
	files {
		"./msdfgen/msdfgen.h",
		"./msdfgen/core/**.h",
		"./msdfgen/core/**.hpp",
		"./msdfgen/core/**.cpp"
	}

usage "msdfgen"
	defines {
		"MSDFGEN_PUBLIC=",
		"MSDFGEN_USE_CPP11"
	}
	includedirs {
		"./"
	}
	links "msdfgen"
