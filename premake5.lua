-- premake5.lua

local dependencyLocation = "src_dependency/"

libdirs { 
	dependencyLocation.."zlib/**",		-- FIXME: better search for platform
}

local dep_includes = {
	zlib_location = dependencyLocation.."zlib",
	jpeg_location = dependencyLocation.."jpeg",

	libogg_location = dependencyLocation.."libogg/include",
	libvorbis_location = dependencyLocation.."libvorbis/include",
	openal_location = dependencyLocation.."openal-soft/include",

	dxsdk_location = dependencyLocation.."minidx9/include",
	sdl2_location = dependencyLocation.."SDL2/include",
	lua_location = dependencyLocation.."lua/src",
	oolua_location = dependencyLocation.."oolua/include",
	wxwidgets_location = dependencyLocation.."wxWidgets/include",

	bulletphysics_location = dependencyLocation.."bullet/src",
}


workspace "equilibrium"
   configurations { "Debug", "Release" }
   
----------------------------------------------------------------------
--- coreLib
----------------------------------------------------------------------

project "eqCoreLib"
   kind "StaticLib"
   language "C++"
   targetdir "%{cfg.buildcfg}"

   vpaths {
	   ["Headers"] = { "**.h" },
	   ["Sources"] = { "**.cpp" }
   }

   files { 
		"public/core/*.cpp",
		"public/core/*.h", 
   }
   
   includedirs {
		"public",
		"public/core",
		dep_includes.zlib_location,
   }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
	  
----------------------------------------------------------------------
--- frameworkLib
----------------------------------------------------------------------
	  
project "eqFrameworkLib"
   kind "StaticLib"
   language "C++"
   targetdir "%{cfg.buildcfg}"
   
   vpaths {
	   ["Headers/*"] = { "**.h" },
	   ["Sources/*"] = { "**.cpp" }
   }
   
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
		dep_includes.zlib_location,
		dep_includes.jpeg_location,
   }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
	  
----------------------------------------------------------------------
--- eqCore dynamic library
----------------------------------------------------------------------
	  
project "eqCoreShared"
   kind "SharedLib"
   
   language "C++"
   targetdir "%{cfg.buildcfg}"
   
   links {  "eqCoreLib", "eqFrameworkLib", "zlibstat"  }
   
if os.host() == "windows" then
   links {  "DbgHelp"  }
end
   
   location "Core"
   
   vpaths {
	   ["Headers/*"] = { "**.h" },
	   ["Sources/*"] = { "**.cpp" }
   }
   
   files { 
	   "Core/**.h", 
	   "Core/**.cpp",
   }
   
   includedirs {
		"public",
		"public/core",
		dep_includes.zlib_location,
   }
   
   defines { "DLL_EXPORT" }

   filter "configurations:Debug"
	  defines { "DEBUG" }
	  symbols "On"

   filter "configurations:Release"
	  defines { "NDEBUG" }
	  optimize "On"