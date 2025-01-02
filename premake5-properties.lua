

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
	configmap {
		["Debug"] = "Debug",
		["Release"] = "Release",
		["ReleaseAsan"] = "Release",
		["Profile"] = "Retail",
		["Retail"] = "Retail",
	}
	targetdir "build/thirdpartylib/%{cfg.platform}/%{cfg.buildcfg}"
	
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