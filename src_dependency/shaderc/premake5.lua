-- Google ShaderC as a usage
local VULKAN_SDK_LOCATION = os.getenv("VULKAN_SDK") or ""

usage "shaderc"
	includedirs {
		VULKAN_SDK_LOCATION.."/Include"
	}
	libdirs { 
		VULKAN_SDK_LOCATION.."./Lib",
	}
	links {
		"shaderc",
		"shaderc_combined",
	}
