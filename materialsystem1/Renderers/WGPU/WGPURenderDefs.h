//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer constants
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <webgpu/webgpu.h>

// ETextureFormat
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

// EBufferBindType
static WGPUBufferBindingType g_wgpuBufferBindingType[] = {
	WGPUBufferBindingType_Uniform,
	WGPUBufferBindingType_Storage,
	WGPUBufferBindingType_ReadOnlyStorage,
};

// ESamplerBindType
static WGPUSamplerBindingType g_wgpuSamplerBindingType[] = {
	WGPUSamplerBindingType_Undefined,
	WGPUSamplerBindingType_Filtering,
	WGPUSamplerBindingType_NonFiltering,
	WGPUSamplerBindingType_Comparison,
};

// ETextureSampleType
static WGPUTextureSampleType g_wgpuTexSampleType[] = {
	WGPUTextureSampleType_Float,
	WGPUTextureSampleType_UnfilterableFloat,
	WGPUTextureSampleType_Depth,
	WGPUTextureSampleType_Sint,
	WGPUTextureSampleType_Uint,
};

// ETextureDimension
static WGPUTextureViewDimension g_wgpuTexViewDimensions[] = {
	WGPUTextureViewDimension_1D,
	WGPUTextureViewDimension_2D,
	WGPUTextureViewDimension_2DArray,
	WGPUTextureViewDimension_Cube,
	WGPUTextureViewDimension_CubeArray,
	WGPUTextureViewDimension_3D,
};

// EShaderVisibility
static WGPUStorageTextureAccess g_wgpuStorageTexAccess[] = {
	WGPUStorageTextureAccess_WriteOnly,
	WGPUStorageTextureAccess_ReadOnly,
	WGPUStorageTextureAccess_ReadWrite,
};

// EVertAttribFormat
static WGPUVertexFormat g_wgpuVertexFormats[][4] = {
	{
		WGPUVertexFormat_Undefined, WGPUVertexFormat_Undefined, WGPUVertexFormat_Undefined, WGPUVertexFormat_Undefined
	},
	{
		WGPUVertexFormat_Undefined, WGPUVertexFormat_Uint8x2, WGPUVertexFormat_Undefined, WGPUVertexFormat_Uint8x4
	},
	{
		WGPUVertexFormat_Undefined, WGPUVertexFormat_Float16x2, WGPUVertexFormat_Undefined, WGPUVertexFormat_Float16x4
	},
	{
		WGPUVertexFormat_Float32, WGPUVertexFormat_Float32x2, WGPUVertexFormat_Float32x3, WGPUVertexFormat_Float32x4
	},
};

// EVertexStepMode
static WGPUVertexStepMode g_wgpuVertexStepMode[] = {
	WGPUVertexStepMode_Vertex,
	WGPUVertexStepMode_Instance,
};

// ECompareFunc
static WGPUCompareFunction g_wgpuCompareFunc[] = {
	WGPUCompareFunction_Undefined,
	WGPUCompareFunction_Never,
	WGPUCompareFunction_Less,	
	WGPUCompareFunction_Equal,	
	WGPUCompareFunction_LessEqual,	
	WGPUCompareFunction_Greater,
	WGPUCompareFunction_NotEqual,
	WGPUCompareFunction_GreaterEqual,	
	WGPUCompareFunction_Always,	
};

// EStencilFunc
static WGPUStencilOperation g_wgpuStencilOp[] = {
	WGPUStencilOperation_Keep,
	WGPUStencilOperation_Zero,
	WGPUStencilOperation_Replace,
	WGPUStencilOperation_Invert,
	WGPUStencilOperation_IncrementWrap,
	WGPUStencilOperation_DecrementWrap,
	WGPUStencilOperation_IncrementClamp,
	WGPUStencilOperation_DecrementClamp
};

// EBlendFunc
static WGPUBlendOperation g_wgpuBlendOp[] = {
	WGPUBlendOperation_Add,
	WGPUBlendOperation_Subtract,		
	WGPUBlendOperation_ReverseSubtract,
	WGPUBlendOperation_Min,			
	WGPUBlendOperation_Max,			
};

// EBlendFactor
static WGPUBlendFactor g_wgpuBlendFactor[] = {
	WGPUBlendFactor_Zero,
	WGPUBlendFactor_One,
	WGPUBlendFactor_Src,
	WGPUBlendFactor_OneMinusSrc,
	WGPUBlendFactor_Dst,
	WGPUBlendFactor_OneMinusDst,
	WGPUBlendFactor_SrcAlpha,
	WGPUBlendFactor_OneMinusSrcAlpha,
	WGPUBlendFactor_DstAlpha,
	WGPUBlendFactor_OneMinusDstAlpha,
	WGPUBlendFactor_SrcAlphaSaturated,	
};

// ECullMode
static WGPUCullMode g_wgpuCullMode[] = {
	WGPUCullMode_None,
	WGPUCullMode_Back,
	WGPUCullMode_Front,
};

// EPrimTopology
static WGPUPrimitiveTopology g_wgpuPrimTopology[] = {
	WGPUPrimitiveTopology_PointList,
	WGPUPrimitiveTopology_LineList,
	WGPUPrimitiveTopology_LineStrip,
	WGPUPrimitiveTopology_TriangleList,
	WGPUPrimitiveTopology_TriangleStrip
};

// EStripIndexFormat
static WGPUIndexFormat g_wgpuStripIndexFormat[] = {
	WGPUIndexFormat_Undefined,
	WGPUIndexFormat_Uint16,
	WGPUIndexFormat_Uint32,
};

// ETexAddressMode
static WGPUAddressMode g_wgpuAddressMode[] = {
	WGPUAddressMode_Repeat,
	WGPUAddressMode_ClampToEdge,
	WGPUAddressMode_MirrorRepeat
};

//  ETexFilterMode
static WGPUFilterMode g_wgpuFilterMode[] = {
	WGPUFilterMode_Nearest,
	WGPUFilterMode_Linear,
	WGPUFilterMode_Linear, // everything else is anisotropic I guess...
	WGPUFilterMode_Linear,
	WGPUFilterMode_Linear,
	WGPUFilterMode_Linear,
};

// ETexFilterMode
static WGPUMipmapFilterMode g_wgpuMipmapFilterMode[] = {
	WGPUMipmapFilterMode_Nearest,
	WGPUMipmapFilterMode_Linear,
	WGPUMipmapFilterMode_Linear,
	WGPUMipmapFilterMode_Linear,
	WGPUMipmapFilterMode_Linear,
	WGPUMipmapFilterMode_Linear,
};

// ELoadFunc
static WGPULoadOp g_wgpuLoadOp[] = {
	WGPULoadOp_Load,
	WGPULoadOp_Clear
};

// EStoreFunc
static WGPUStoreOp g_wgpuStoreOp[] = {
	WGPUStoreOp_Store,
	WGPUStoreOp_Discard,
};

// EIndexFormat
static WGPUIndexFormat g_wgpuIndexFormat[] = {
	WGPUIndexFormat_Uint16,
	WGPUIndexFormat_Uint32,
};

inline static void FillWGPUSamplerDescriptor(const SamplerStateParams& samplerParams, WGPUSamplerDescriptor& rhiSamplerDesc)
{
	rhiSamplerDesc.addressModeU = g_wgpuAddressMode[samplerParams.addressU];
	rhiSamplerDesc.addressModeV = g_wgpuAddressMode[samplerParams.addressV];
	rhiSamplerDesc.addressModeW = g_wgpuAddressMode[samplerParams.addressW];
	rhiSamplerDesc.compare = g_wgpuCompareFunc[samplerParams.compareFunc];
	rhiSamplerDesc.minFilter = g_wgpuFilterMode[samplerParams.minFilter];
	rhiSamplerDesc.magFilter = g_wgpuFilterMode[samplerParams.magFilter];
	rhiSamplerDesc.mipmapFilter = g_wgpuMipmapFilterMode[samplerParams.mipmapFilter];
	rhiSamplerDesc.maxAnisotropy = samplerParams.maxAnisotropy;
}

inline static void FillWGPUBlendComponent(const BlendStateParams& blendParams, WGPUBlendComponent& rhiBlendComponent)
{
	rhiBlendComponent.operation = g_wgpuBlendOp[blendParams.blendFunc];
	rhiBlendComponent.srcFactor = g_wgpuBlendFactor[blendParams.srcFactor];
	rhiBlendComponent.dstFactor = g_wgpuBlendFactor[blendParams.dstFactor];
}