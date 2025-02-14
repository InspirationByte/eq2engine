local VULKAN_SDK_LOCATION = os.getenv("VULKAN_SDK") or ""

project "nvrhi"
	properties {
		"staticlib",
		"thirdpartylib"
	}
	
	defines {
		"NOMINMAX",
		-- no NVRHI_WITH_RTXMU
		-- no NVRHI_WITH_AFTERMATH
	}
	
	files
	{
		"NVRHI/include/**.h",
		"NVRHI/src/**.h",
		"NVRHI/src/common/**.cpp",
		"NVRHI/src/validation/**.cpp"
	}
	includedirs {
		"./NVRHI/include"
	}
	
	if false then -- VULKAN_SDK_LOCATION ~= "" then
		defines {
			"NOMINMAX",
			"NVRHI_WITH_VULKAN",
			"VK_USE_PLATFORM_WIN32_KHR"
		}
		files {
			"NVRHI/src/vulkan/**.cpp"
		}
		includedirs {
			VULKAN_SDK_LOCATION.."/include",
		}
		libdirs { 
			VULKAN_SDK_LOCATION.."./lib",
		}
	end
	
	filter "configurations:Debug"
		defines { "NVRHI_WITH_VALIDATION" }
	
	filter "system:windows"
		defines { 
			"NVRHI_WITH_DX11",
			--"NVRHI_WITH_DX12",
		}
		files {
			"NVRHI/src/d3d11/**.cpp",
			"NVRHI/src/d3d12/**.cpp"
		}
		
usage "nvrhi"
	includedirs { 
		"./NVRHI/include"
	}
	links "nvrhi"