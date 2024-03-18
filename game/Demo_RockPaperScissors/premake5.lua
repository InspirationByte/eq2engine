WORKSPACE_NAME = "Demo_RockPaperScissors"

dofile "premake5-engine.lua"

group "Game"

project "Demo_RockPaperScissors"
	if IS_ANDROID then
		kind "SharedLib"
	else
    	kind "WindowedApp"

		targetname "Demo_RockPaperScissors_%{cfg.buildcfg}"
	end

	unitybuild "on"
    uses {
		"e2Core", "frameworkLib", "coreLib",
		"sysLib",
		"renderUtilLib",
		"soundSystemLib",
		"movieLib",
		--"egfLib", "physicsLib", "animatingLib", 
		--"networkLib",
		"BaseShader",
		--"bullet2",
		"imgui",
		--"sol2",
		--"oolua", "lua",
		--"Recast", "lz4"
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