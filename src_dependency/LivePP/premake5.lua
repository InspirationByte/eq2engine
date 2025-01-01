if ENABLE_LIVEPP then

property "live_pp"	
	-- Windows x64 only and Release configuration
	filter {"system:Windows", "platforms:x64", "configurations:Release"}
		defines { "HAS_LIVEPP_SUPPORT" }
		buildoptions {
			"/Gm-",
		}
		linkoptions {
			"/FUNCTIONPADMIN",
			"/OPT:NOREF",
			"/OPT:NOICF",
			"/DEBUG:FULL",
		}
		includedirs {
			"./API/x64"
		}
else
	property "live_pp"	
		-- leave empty usage
end