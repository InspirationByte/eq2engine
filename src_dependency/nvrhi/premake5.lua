project "nvrhi"
	properties {
		"staticlib",
		"thirdpartylib"
	}
	
	defines {
		"NOMINMAX",
		"VK_NO_PROTOTYPES",
		-- no NVRHI_WITH_RTXMU
		-- no NVRHI_WITH_AFTERMATH
	}
	files {
		"NVRHI/include/**.h",
		"NVRHI/src/common/**.cpp",
		"NVRHI/src/common/**.h",
		"NVRHI/src/vulkan/**.cpp",
		"NVRHI/src/vulkan/**.h"
	}
	removefiles {
		"NVRHI/src/common/dxgi*.cpp",
		"NVRHI/src/common/dxgi*.h",
	}
	includedirs {
		"NVRHI/include",
		"NVRHI/thirdparty/Vulkan-Headers/include"
	}	
	filter "system:windows"
		defines {
			"NVRHI_D3D12_WITH_D3D12MA",
			"VK_USE_PLATFORM_WIN32_KHR",
			"D3D12MA_USING_DIRECTX_HEADERS"
		}
		includedirs {
			"NVRHI/thirdparty/D3D12MA/include",
			"NVRHI/thirdparty/DirectX-Headers/include"
		}
		files {
			"NVRHI/src/d3d11/**.cpp",
			"NVRHI/src/d3d11/**.h",
			"NVRHI/src/d3d12/**.cpp",
			"NVRHI/src/d3d12/**.h",
			"NVRHI/src/common/dxgi*.cpp",
			"NVRHI/src/common/dxgi*.h",
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
	rtti "On" -- gmake generator kind None is kinda unfinished - behaviour is inconsistent between msvc generators
	files {
		"NVRHI/src/validation/**.cpp",
		"NVRHI/src/validation/**.h",
	}

usage "nvrhi"
	includedirs { 
		"NVRHI/include",
		"NVRHI/thirdparty/Vulkan-Headers/include",
		"NVRHI/thirdparty/DirectX-Headers/include"
	}
	links "nvrhi"