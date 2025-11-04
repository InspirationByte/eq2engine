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
		"NVRHI/src/common/**.cpp",
	}
	removefiles {
		"NVRHI/src/common/dxgi*.cpp",	
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
			"NVRHI/thirdparty/Vulkan-Headers/include"
		}
		libdirs { 
			VULKAN_SDK_LOCATION.."./lib",
		}
	end
	
	filter "system:windows"
		defines {
			"NVRHI_D3D12_WITH_D3D12MA",
			"D3D12MA_USING_DIRECTX_HEADERS"
		}
		includedirs {
			"NVRHI/thirdparty/D3D12MA/include",
			"NVRHI/thirdparty/DirectX-Headers/include"
		}
		files {
			"NVRHI/src/d3d11/**.cpp",
			"NVRHI/src/d3d12/**.cpp",
			"NVRHI/src/common/dxgi*.cpp",
			"NVRHI/thirdparty/D3D12MA/src/D3D12MemAlloc.cpp",
			"NVRHI/thirdparty/D3D12MA/src/Common.*",
			"NVRHI/thirdparty/D3D12MA/include/**.h"
		}

project "nvrhi-validation"
	kind "StaticLib"
	properties {
		"thirdpartylib"
	}
	uses {
		"nvrhi"
	}
	files {
		"NVRHI/src/validation/**.cpp",	
	}
	filter "configurations:Retail or configurations:Profile"
		kind "None"

usage "nvrhi"
	includedirs { 
		"NVRHI/include",
		"NVRHI/thirdparty/DirectX-Headers/include"
	}
	links "nvrhi"