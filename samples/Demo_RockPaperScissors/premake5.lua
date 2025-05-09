WORKSPACE_NAME = "Demo_RockPaperScissors"

include "../.."

property "gameapp"
	if _ACTION ~= "vscode" then
		location "%{ prj_location(prj) }"
	end

group ""

project "Demo_RockPaperScissors"
	if IS_ANDROID then
		kind "SharedLib"
	else
    	kind "WindowedApp"
		targetname "Demo_RockPaperScissors_%{cfg.buildcfg}"
	end

	properties { "unitybuild", "gameapp" }
    uses {
		"e2Core", "frameworkLib", "coreLib",
		"sysLib",
		"renderUtilLib",
		"soundSystemLib",
		"movieLib",
		"BaseShader",
		"imgui",
	}
    files {
		"./**",
	}

    includedirs {
		"./"
	}

    filter "system:Windows"
		linkoptions {
			"/SAFESEH:NO", -- Image Has Safe Exception Handers: No. Because of openal-soft
		}
		
	filter "system:Android"
		links { 
			"android" 
		}

		-- TODO: copy files instead
		files {
			"./android/AndroidManifest.xml",
		}
	if not IS_ANDROID then
		filter "configurations:Retail"
			targetname "Demo_RockPaperScissors"
	end