property "e2_ws_settings"
    language 'C++'
	cppdialect 'C++17'
	flags 'MultiProcessorCompile'
	shortcommands 'On'
	linkgroups 'On'
	pic 'On'
	floatingpointexceptions  'Off'
	unsignedchar  'On'
	
	objdir "build/obj"
	targetdir "build/bin/%{cfg.platform}/%{cfg.buildcfg}"
	libdirs {
		 "build/thirdpartylib/",
		 "build/lib/"
 	}
	
	filter "kind:StaticLib"
		targetdir "build/lib/%{cfg.platform}/%{cfg.buildcfg}"

	-- MSVC thing only
	filter "system:Windows"
		implibdir "build/lib/%{cfg.platform}/%{cfg.buildcfg}"

property "e2_ws_configurations"
    filter "configurations:Debug"
        defines { 
            "DEBUG"
        }
        symbols "On"

    filter "configurations:Release"
        defines {
            "NDEBUG",
        }
		optimize "On"
		symbols "On"
		
	filter "configurations:ReleaseAsan"
        defines {
            "NDEBUG",
        }
		optimize "On"
		symbols "On"
		sanitize "address"

	filter "configurations:Profile"
        defines {
			"NDEBUG",
			"_PROFILE"
        }
		optimize "On"
		symbols "On"
		rtti "Off"

	filter "configurations:Retail"
        defines {
			"NDEBUG",
			"_RETAIL"
        }
		optimize "On"
		rtti "Off"

	filter "system:Linux"
		defines {
			"__LINUX__"
		}
		
	----
	
	filter "system:android"
		floatingpointexceptions  'On'	-- can't remember, probably for NEON
		platforms {
			"android-arm",
			"android-arm64"
			--"android-x86_64"
		}
		
		buildoptions {
			"-pthread"
		}
		
		linkoptions {
			"--no-undefined",
			"-pthread",
			"-mfloat-abi=softfp",	-- force NEON to be used
			"-mfpu=neon"
		}

		filter "platforms:*-x86"
			architecture "x86"

		filter "platforms:*-x86_64"
			architecture "x86_64"

		filter "platforms:*-arm"
			architecture "arm"

		filter "platforms:*-arm64"
			architecture "arm64"

    filter "system:linux"
		platforms { 
			--"x86", 
			"x64"
			-- TODO: arm
		}

	filter "system:Windows"
		platforms { 
			--"x86", 
			"x64"
			-- TODO: arm
		}
		
property "windows_msvc"
	filter "system:Windows"
		linkoptions {
			"/NOEXP"
		}
		disablewarnings { 
			"4996", 
			"4554", 
			"4244", 
			"4101", 
			"4838", 
			"4309"
		}
		enablewarnings { 
			"26433"
		}
	filter "configurations:Retail or configurations:Profile"
		buildoptions { "/GR-" }

property "gcc_clang"
	filter "system:Linux or system:Android"
		links { "pthread" }
		buildoptions {
			"-fpermissive",
		}
		disablewarnings {
			-- disable warnings which are emitted by my stupid (and not so) code
			"narrowing",
			"c++11-narrowing",
			"writable-strings",
			"logical-op-parentheses",
			"parentheses",
			"register",
			"unused-local-typedef",
			"nonportable-include-path",
			"format-security",
			"unused-parameter",
			"sign-compare",
			"ignored-attributes",	-- annyoing, don't re-enable
			"write-strings",		-- TODO: fix this
			"subobject-linkage"		-- TODO: fix this
		}

property "unitybuild"
	unitybuild "on"
	maxfilesinunity "30"
	
property "sharedlib"
	kind "SharedLib"
	
property "staticlib"
	kind "StaticLib"
	
property "tools"
	filter "configurations:Retail or configurations:Profile"
		kind "None"
		
property "thirdpartylib"
	--[[configmap {
		["Debug"] = "Debug",
		["Release"] = "Release",
		["ReleaseAsan"] = "Release",
		["Profile"] = "Retail",
		["Retail"] = "Retail",
	}]] -- fookin std annotate_string & annotate_vector
	targetdir "build/thirdpartylib/%{cfg.platform}/%{cfg.buildcfg}"
		
function prj_name(prj, wks, def)
	if _ACTION == "gmake2" and def == nil then
		def = wks.name..".solution"
	end
	if prj ~= nil then
		if prj.group ~= nil and string.len(prj.group) > 0 then
			return prj.group .. '/' .. prj.name
		end
		return prj.name
	end
	return def
end