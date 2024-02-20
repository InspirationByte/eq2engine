-- Google ShaderC as a usage
local VULKAN_SDK_LOCATION = os.getenv("VULKAN_SDK") or ""

usage "shaderc"
	includedirs {
		VULKAN_SDK_LOCATION.."/include"
	}
	libdirs { 
		VULKAN_SDK_LOCATION.."./lib",
	}
	links {
		"shaderc",
		"shaderc_combined",
	}
