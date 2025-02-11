project "gtest"
	language "C++"
	kind "StaticLib"
	properties	{ "thirdpartylib" }

	includedirs { "./", "include" }

	files { "src/*.cc", "src/*.h" }
	removefiles  { "src/gtest_main.cc", "src/gtest-all.cc" }
	
usage "gtest"
	includedirs { "./", "include" }
	links { "gtest" }