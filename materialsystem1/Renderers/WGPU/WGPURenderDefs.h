//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Constant types for Equilibrium renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <webgpu/webgpu.h>

static WGPUTextureFormat g_wgpuTexFormats[] = {
	WGPUTextureFormat_Undefined,

	WGPUTextureFormat_R8Unorm,
	WGPUTextureFormat_RG8Unorm,
	WGPUTextureFormat_RGBA8Unorm, // RGB8 not directly supported, threat as RGBA8
	WGPUTextureFormat_RGBA8Unorm,

	WGPUTextureFormat_R16Unorm,
	WGPUTextureFormat_RG16Unorm,
	WGPUTextureFormat_Undefined, // RGB16 not directly supported
	WGPUTextureFormat_RGBA16Unorm,

	WGPUTextureFormat_R8Snorm,
	WGPUTextureFormat_RG8Snorm,
	WGPUTextureFormat_Undefined, // RGB8S not directly supported
	WGPUTextureFormat_RGBA8Snorm,

	WGPUTextureFormat_R16Snorm,
	WGPUTextureFormat_RG16Snorm,
	WGPUTextureFormat_Undefined, // RGB16S not directly supported
	WGPUTextureFormat_RGBA16Snorm,

	WGPUTextureFormat_R16Float,
	WGPUTextureFormat_RG16Float,
	WGPUTextureFormat_Undefined, // RGB16F not directly supported
	WGPUTextureFormat_RGBA16Float,

	WGPUTextureFormat_R32Float,
	WGPUTextureFormat_RG32Float,
	WGPUTextureFormat_Undefined, // RGB32F not directly supported
	WGPUTextureFormat_RGBA32Float,

	WGPUTextureFormat_R16Sint,
	WGPUTextureFormat_RG16Sint,
	WGPUTextureFormat_Undefined, // RGB16I not directly supported
	WGPUTextureFormat_RGBA16Sint,

	WGPUTextureFormat_R32Sint,
	WGPUTextureFormat_RG32Sint,
	WGPUTextureFormat_Undefined, // RGB32S not directly supported
	WGPUTextureFormat_RGBA32Sint,

	WGPUTextureFormat_R16Uint,
	WGPUTextureFormat_RG16Uint,
	WGPUTextureFormat_Undefined, // RGB16UI not directly supported
	WGPUTextureFormat_RGBA16Uint,

	WGPUTextureFormat_R32Uint,
	WGPUTextureFormat_RG32Uint,
	WGPUTextureFormat_Undefined,
	WGPUTextureFormat_RGBA32Uint ,

	WGPUTextureFormat_Undefined, // RGBE8 not directly supported
	WGPUTextureFormat_Undefined,
	WGPUTextureFormat_Undefined,
	WGPUTextureFormat_Undefined,
	WGPUTextureFormat_Undefined, // RGBA4 not directly supported
	WGPUTextureFormat_RGB10A2Unorm,

	WGPUTextureFormat_Depth16Unorm,
	WGPUTextureFormat_Depth24Plus,
	WGPUTextureFormat_Depth24PlusStencil8,
	WGPUTextureFormat_Depth32Float,

	WGPUTextureFormat_BC1RGBAUnorm,
	WGPUTextureFormat_BC2RGBAUnorm,
	WGPUTextureFormat_BC3RGBAUnorm,
	WGPUTextureFormat_BC4RUnorm,
	WGPUTextureFormat_BC5RGUnorm,

	// TODO: more BC

	WGPUTextureFormat_Undefined, // TODO: remove, replace with other formats
	WGPUTextureFormat_ETC2RGB8Unorm,
	WGPUTextureFormat_ETC2RGB8A1Unorm,
	WGPUTextureFormat_ETC2RGBA8Unorm,
	WGPUTextureFormat_Undefined, // TODO: ASTC support
	WGPUTextureFormat_Undefined,
	WGPUTextureFormat_Undefined,
	WGPUTextureFormat_Undefined,
};


/*
static const D3D10_BLEND blendingConsts[] = {
	D3D10_BLEND_ZERO,
	D3D10_BLEND_ONE,
	D3D10_BLEND_SRC_COLOR,
	D3D10_BLEND_INV_SRC_COLOR,
	D3D10_BLEND_DEST_COLOR,
	D3D10_BLEND_INV_DEST_COLOR,
	D3D10_BLEND_SRC_ALPHA,
	D3D10_BLEND_INV_SRC_ALPHA,
	D3D10_BLEND_DEST_ALPHA,
	D3D10_BLEND_INV_DEST_ALPHA,
	D3D10_BLEND_SRC_ALPHA_SAT,
};

static const D3D10_BLEND blendingConstsAlpha[] = {
	D3D10_BLEND_ZERO,
	D3D10_BLEND_ONE,
	D3D10_BLEND_SRC_ALPHA,
	D3D10_BLEND_INV_DEST_ALPHA,
	D3D10_BLEND_DEST_ALPHA,
	D3D10_BLEND_INV_DEST_ALPHA,
	D3D10_BLEND_SRC_ALPHA,
	D3D10_BLEND_INV_SRC_ALPHA,
	D3D10_BLEND_DEST_ALPHA,
	D3D10_BLEND_INV_DEST_ALPHA,
	D3D10_BLEND_SRC_ALPHA_SAT,
};

static const D3D10_BLEND_OP blendingModes[] = {
	D3D10_BLEND_OP_ADD,
	D3D10_BLEND_OP_SUBTRACT,
	D3D10_BLEND_OP_REV_SUBTRACT,
	D3D10_BLEND_OP_MIN,
	D3D10_BLEND_OP_MAX,
};

static const D3D10_COMPARISON_FUNC comparisonConst[] = {
	D3D10_COMPARISON_NEVER,
	D3D10_COMPARISON_LESS,
	D3D10_COMPARISON_EQUAL,
	D3D10_COMPARISON_LESS_EQUAL,
	D3D10_COMPARISON_GREATER,
	D3D10_COMPARISON_NOT_EQUAL,
	D3D10_COMPARISON_GREATER_EQUAL,
	D3D10_COMPARISON_ALWAYS,
};

static const D3D10_STENCIL_OP stencilConst[] = {
	D3D10_STENCIL_OP_KEEP,
	D3D10_STENCIL_OP_ZERO,
	D3D10_STENCIL_OP_REPLACE,
	D3D10_STENCIL_OP_INVERT,
	D3D10_STENCIL_OP_INCR,
	D3D10_STENCIL_OP_DECR,
	D3D10_STENCIL_OP_INCR_SAT,
	D3D10_STENCIL_OP_DECR_SAT,
};

static const D3D10_CULL_MODE cullConst[] = {
	D3D10_CULL_NONE,
	D3D10_CULL_BACK,
	D3D10_CULL_FRONT,
};

static const D3D10_FILL_MODE fillConst[] = {
	D3D10_FILL_SOLID,
	D3D10_FILL_WIREFRAME,
	D3D10_FILL_WIREFRAME,
};

static const DXGI_FORMAT d3ddecltypes[][4] = {
	DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,
	DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_UNKNOWN,         DXGI_FORMAT_R16G16B16A16_FLOAT,
	DXGI_FORMAT_R8_UNORM,  DXGI_FORMAT_R8G8_UNORM,   DXGI_FORMAT_UNKNOWN,         DXGI_FORMAT_R8G8B8A8_UNORM,
};


static const D3D10_USAGE g_d3d9_bufferUsages[] = {
	D3D10_USAGE_DEFAULT,
	D3D10_USAGE_IMMUTABLE,
	D3D10_USAGE_DYNAMIC,
};

const D3D10_PRIMITIVE_TOPOLOGY d3dPrim[] = {
	D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
	D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED, // Triangle fans not supported
	D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED, // Quads not supported
	D3D10_PRIMITIVE_TOPOLOGY_LINELIST,
	D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP,
	D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED, // Line loops not supported
	D3D10_PRIMITIVE_TOPOLOGY_POINTLIST,
};

const D3D10_FILTER d3dFilterType[] = {
	D3D10_FILTER_MIN_MAG_MIP_POINT,
	D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT,
	D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT,
	D3D10_FILTER_MIN_MAG_MIP_LINEAR,
	D3D10_FILTER_ANISOTROPIC,
	D3D10_FILTER_ANISOTROPIC,
};

const D3D10_TEXTURE_ADDRESS_MODE d3dAddressMode[] = {
	D3D10_TEXTURE_ADDRESS_WRAP,
	D3D10_TEXTURE_ADDRESS_CLAMP,
	D3D10_TEXTURE_ADDRESS_MIRROR,
};*/