//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: ShaderAPI capabilities
//////////////////////////////////////////////////////////////////////////////////

#pragma once

enum EShaderSupportFlags
{
	SHADER_CAPS_VERTEX_SUPPORTED	= (1 << 0),
	SHADER_CAPS_PIXEL_SUPPORTED		= (1 << 1),
	SHADER_CAPS_GEOMETRY_SUPPORTED	= (1 << 2),
	SHADER_CAPS_DOMAIN_SUPPORTED	= (1 << 3),
	SHADER_CAPS_HULL_SUPPORTED		= (1 << 4),
};

//-------------------------------------------------------------------------------------

struct ShaderAPICaps_t
{
	bool				textureFormatsSupported[FORMAT_COUNT]{ false };
	bool				renderTargetFormatsSupported[FORMAT_COUNT]{ false };

	bool				INTZSupported{ false };		// Direct3D9 INTZ (internal Z buffer) sampling as texture is supported
	ETextureFormat		INTZFormat{ FORMAT_NONE };

	bool				NULLSupported{ false };		// Direct3D9 NULL sampling as texture is supported
	ETextureFormat		NULLFormat{ FORMAT_NONE };

	bool				isInstancingSupported{ 0 };
	bool				isHardwareOcclusionQuerySupported{ 0 };

	int					maxTextureSize{ 0 };
	int					maxRenderTargets{ 0 };
	
	int					maxTextureUnits{ 0 };
	int					maxVertexTextureUnits{ 0 };

	int					maxTextureAnisotropicLevel{ 0 };

	int					maxSamplerStates{ 0 };

	int					maxVertexStreams{ 0 };
	int					maxVertexGenericAttributes{ 0 };
	int					maxVertexTexcoordAttributes{ 0 };

	int					shadersSupportedFlags{ 0 };			// EShaderSupportFlags

	int					shaderVersions[4]{ 0 };
};
