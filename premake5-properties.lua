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
	targetdir "%{_MAIN_SCRIPT_DIR}/build/bin/%{cfg.platform}/%{cfg.buildcfg}"
	libdirs {
		 "build/thirdpartylib/",
		 "build/lib/"
 	}
	
	location "%{ prj_location(prj, wks) }"
	
	filter "kind:StaticLib"
		targetdir "build/lib/%{cfg.platform}/%{cfg.buildcfg}"

	-- MSVC thing only
	filter "system:Windows"
		implibdir "build/lib/%{cfg.platform}/%{cfg.buildcfg}"

	filter "system:Linux"
		toolset "clang"

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
		optimize "Speed"
		symbols "On"
		rtti "Off"

	filter "configurations:Retail"
        defines {
			"NDEBUG",
			"_RETAIL"
        }
		optimize "Speed"
		rtti "Off"
		
	filter { "configurations:Retail", "system:Windows" }
		buildoptions { "/GL", "/Ot" }
		linkoptions { "/LTCG:incremental" }

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
		-- use specific windows SDK
		systemversion(WINSDK_VER)
		
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
		
	filter {"system:Windows", "configurations:Retail or configurations:Profile" }
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
	if not BUILD_SINGLE_FILE then
		unitybuild "on"
		maxfilesinunity "30"
	end
	
property "sharedlib"
	kind "SharedLib"
	
property "staticlib"
	kind "StaticLib"
	
property "app"
	filter "platforms:*64"
		debugdir "%{wks.location}../../build/Bin64"
		debugenvs "PATH=%{wks.location}../../build/Bin64"

	filter "platforms:*86"
		debugdir "%{wks.location}../../build/Bin32"
		debugenvs "PATH=%{wks.location}../../build/Bin64"
	
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

function prj_location(prj, wks, def)
	if _ACTION == "vscode" then
		return _MAIN_SCRIPT_DIR
	end

	if _ACTION == "gmake2" and wks ~= nil and def == nil then
		def = _MAIN_SCRIPT_DIR.."/build/"..wks.name..".solution"
	else
		def = _MAIN_SCRIPT_DIR .. "/build/"
	end
	if prj ~= nil then
		if prj.group ~= nil and string.len(prj.group) > 0 then
			return "build/" .. prj.group .. '/' .. prj.name
		end
		return "build/" .. prj.name
	end
	return def
end
