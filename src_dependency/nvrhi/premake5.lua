project "nvrhi"
	properties {
		"staticlib",
		"thirdpartylib"
	}
	
	defines { 
		"NVRHI_WITH_VULKAN"
		-- no NVRHI_WITH_RTXMU
		-- no NVRHI_WITH_AFTERMATH
	}
	
	includedirs {
		"./include"
	}

	files
	{
		"include/**.h",
		"src/**.h",
		"src/**.cpp"
	}
	
	filter "configurations:Debug"
		defines { "NVRHI_WITH_VALIDATION" }
	
	filter "system:windows"
		defines { 
			"NVRHI_WITH_DX11",
			"NVRHI_WITH_DX12",
		}
		
usage "nvrhi"
	includedirs { 
		"./include"
	}
	links "nvrhi"