//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: ShaderAPI capabilities
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "imaging/textureformats.h"

static constexpr const int MAX_BINDGROUPS = 4;
static constexpr const int MAX_RENDERTARGETS = 8;
static constexpr const int MAX_VERTEXSTREAM = 8;
static constexpr const int MAX_TEXTUREUNIT = 16;
static constexpr const int MAX_VERTEXTEXTURES = 4;
static constexpr const int MAX_SAMPLERSTATE = 16;
static constexpr const int MAX_GENERIC_ATTRIB = 8;
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

struct ShaderAPICapabilities
{
	bool	textureFormatsSupported[FORMAT_COUNT]{ false };
	bool	renderTargetFormatsSupported[FORMAT_COUNT]{ false };

	bool	isInstancingSupported{ 0 };
	bool	isHardwareOcclusionQuerySupported{ 0 };
	
	int		minUniformBufferOffsetAlignment{ 1 };
	int		minStorageBufferOffsetAlignment{ 1 };
	int		maxDynamicUniformBuffersPerPipelineLayout{ 0 };
	int		maxDynamicStorageBuffersPerPipelineLayout{ 0 };
	int		maxVertexStreams{ 0 };
	int		maxVertexAttributes{ 0 };
	int		maxTextureSize{ 0 };
	int		maxTextureArrayLayers{ 0 };
	int		maxTextureUnits{ 0 };
	int		maxVertexTextureUnits{ 0 };
	int		maxTextureAnisotropicLevel{ 0 };
	int		maxBindGroups{ 0 };
	int		maxBindingsPerBindGroup{ 0 };
	int		maxRenderTargets{ 0 };

	int		maxComputeInvocationsPerWorkgroup{ 0 };
	int		maxComputeWorkgroupSizeX{ 0 };
	int		maxComputeWorkgroupSizeY{ 0 };
	int		maxComputeWorkgroupSizeZ{ 0 };
	int		maxComputeWorkgroupsPerDimension{ 0 };

	int		shadersSupportedFlags{ 0 };		// EShaderSupportFlags
};
