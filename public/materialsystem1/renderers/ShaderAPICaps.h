//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: ShaderAPI capabilities
//////////////////////////////////////////////////////////////////////////////////


#ifndef SHADERAPICAPS_H
#define SHADERAPICAPS_H

#include "imaging/textureformats.h"
#include "ShaderAPI_defs.h"

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
	bool				textureFormatsSupported[FORMAT_COUNT];
	bool				renderTargetFormatsSupported[FORMAT_COUNT];

	bool				INTZSupported;		// Direct3D9 INTZ (internal Z buffer) sampling as texture is supported
	ETextureFormat		INTZFormat;

	bool				NULLSupported;		// Direct3D9 NULL sampling as texture is supported
	ETextureFormat		NULLFormat;

	bool				isInstancingSupported;
	bool				isHardwareOcclusionQuerySupported;

	int					maxTextureSize;
	int					maxRenderTargets;
	
	int					maxTextureUnits;
	int					maxVertexTextureUnits;

	int					maxTextureAnisotropicLevel;

	int					maxSamplerStates;

	int					maxVertexStreams;
	int					maxVertexGenericAttributes;
	int					maxVertexTexcoordAttributes;

	int					shadersSupportedFlags;			// EShaderSupportFlags

	int					shaderVersions[4];
};

#endif // SHADERAPICAPS_H