-- premake5.lua

workspace "equilibrium"
   configurations { "Debug", "Release" }

project "eqCoreLib"
   kind "StaticLib"
   language "C++"
   targetdir "%{cfg.buildcfg}"

   files { 
		"public/core/*.h", 
		"public/core/*.cpp" 
   }
   
   includedirs {
		"public",
		"public/core",
		--"src_dependency\zlib\include"
   }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
	  
project "eqFrameworkLib"
   kind "StaticLib"
   language "C++"
   targetdir "%{cfg.buildcfg}"
   
   files { 
	   "public/imaging/*.h", 
	   "public/imaging/*.cpp",
	   "public/math/*.h", 
	   "public/math/*.cpp",
	   "public/network/*.h", 
	   "public/network/*.cpp",
	   "public/utils/*.h", 
	   "public/utils/*.cpp" 
   }
   
   includedirs {
		"public",
		"public/core",
		--"src_dependency\zlib\include"
   }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
	  
project "eqCoreShared"
   kind "SharedLib"
   
   language "C++"
   targetdir "%{cfg.buildcfg}"
   
   location "Core"
   
   files { 
	   "Core/*.h", 
	   "Core/*.cpp",
   }
   
   includedirs {
		"public",
		"public/core",
		--"src_dependency\zlib\include"
   }
   
   defines { "DLL_EXPORT" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"