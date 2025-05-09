WORKSPACE_NAME = "GPUDriven"

include "../.."

property "gameapp"
	if _ACTION ~= "vscode" then
		location "%{ prj_location(prj) }"
	end

group ""

project "GPUDriven"
	if IS_ANDROID then
		kind "SharedLib"
	else
    	kind "WindowedApp"
		targetname "GPUDriven_%{cfg.buildcfg}"
	end
	
	properties { "unitybuild", "gameapp" }
    uses {
		"e2Core", "frameworkLib", "coreLib",
		"sysLib",
		"renderUtilLib", "grimLib",
		"soundSystemLib",
		"egfLib", "physicsLib", "animatingLib", 
		"BaseShader",
		"imgui"
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
			targetname "GPUDriven"
	end