project "gtest_main"
	language "C++"
	kind "StaticLib"

	includedirs { "./", "include" }

	files { "src/*.cc", "src/*.h" }
	removefiles  { "src/gtest_main.cc", "src/gtest-all.cc" }
	
usage "gtest"
	includedirs { "include" }
	links { "gtest_main" }