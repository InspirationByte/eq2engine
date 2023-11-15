//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: ShaderAPI capabilities
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "imaging/textureformats.h"

static constexpr const int MAX_RENDERTARGETS = 8;
static constexpr const int MAX_VERTEXSTREAM = 8;
static constexpr const int MAX_TEXTUREUNIT = 16;
static constexpr const int MAX_VERTEXTEXTURES = 4;
static constexpr const int MAX_SAMPLERSTATE = 16;
static constexpr const int MAX_GENERIC_ATTRIB= 8;
static constexpr const int MAX_TEXCOORD_ATTRIB = 8;

enum EShaderAPIType : int
{
	SHADERAPI_EMPTY = 0,
	SHADERAPI_OPENGL,
	SHADERAPI_DIRECT3D9,
	SHADERAPI_DIRECT3D10,
	SHADERAPI_WEBGPU
};

enum EShaderSupportFlags
{
	SHADER_CAPS_VERTEX_SUPPORTED	= (1 << 0),
	SHADER_CAPS_PIXEL_SUPPORTED		= (1 << 1),
	SHADER_CAPS_GEOMETRY_SUPPORTED	= (1 << 2),
	SHADER_CAPS_COMPUTE_SUPPORTED	= (1 << 3),
};

//-------------------------------------------------------------------------------------

struct ShaderAPICaps
{
	bool			textureFormatsSupported[FORMAT_COUNT]{ false };
	bool			renderTargetFormatsSupported[FORMAT_COUNT]{ false };

	// FIXME: deprecated after D3D9 removal
	bool			INTZSupported{ false };		// Direct3D9 INTZ (internal Z buffer) sampling as texture is supported
	ETextureFormat	INTZFormat{ FORMAT_NONE };

	bool			NULLSupported{ false };		// Direct3D9 NULL sampling as texture is supported
	ETextureFormat	NULLFormat{ FORMAT_NONE };

	bool			isInstancingSupported{ 0 };
	bool			isHardwareOcclusionQuerySupported{ 0 };

	int				maxVertexStreams{ 0 };
	int				maxVertexGenericAttributes{ 0 };
	int				maxVertexTexcoordAttributes{ 0 };
	int				maxTextureSize{ 0 };
	int				maxTextureArrayLayers{ 0 };
	int				maxTextureUnits{ 0 };
	int				maxVertexTextureUnits{ 0 };
	int				maxTextureAnisotropicLevel{ 0 };
	int				maxBindGroups{ 0 };
	int				maxBindingsPerBindGroup{ 0 };
	int				maxRenderTargets{ 0 };

	int				shadersSupportedFlags{ 0 };		// EShaderSupportFlags
};
