//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer constants
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/nvrhi.h>
#include "renderers/ShaderAPI_defs.h"

// ETextureFormat
static nvrhi::Format g_nvrhiTexFormats[] = {
	nvrhi::Format::UNKNOWN,

	nvrhi::Format::R8_UNORM,
	nvrhi::Format::RG8_UNORM,
	nvrhi::Format::UNKNOWN, // RGB8 not directly supported, threat as RGBA8
	nvrhi::Format::RGBA8_UNORM,

	nvrhi::Format::R16_UNORM,
	nvrhi::Format::RG16_UNORM,
	nvrhi::Format::UNKNOWN, // RGB16 not directly supported
	nvrhi::Format::RGBA16_UNORM,

	nvrhi::Format::R8_SNORM,
	nvrhi::Format::RG8_SNORM,
	nvrhi::Format::UNKNOWN, // RGB8S not directly supported
	nvrhi::Format::RGBA8_SNORM,

	nvrhi::Format::R16_SNORM,
	nvrhi::Format::RG16_SNORM,
	nvrhi::Format::UNKNOWN, // RGB16S not directly supported
	nvrhi::Format::RGBA16_SNORM,

	nvrhi::Format::R16_FLOAT,
	nvrhi::Format::RG16_FLOAT,
	nvrhi::Format::UNKNOWN, // RGB16F not directly supported
	nvrhi::Format::RGBA16_FLOAT,

	nvrhi::Format::R32_FLOAT,
	nvrhi::Format::RG32_FLOAT,
	nvrhi::Format::RGB32_FLOAT, 
	nvrhi::Format::RGBA32_FLOAT,

	nvrhi::Format::R16_SINT,
	nvrhi::Format::RG16_SINT,
	nvrhi::Format::UNKNOWN, // RGB16I not directly supported
	nvrhi::Format::RGBA16_SINT,

	nvrhi::Format::R32_SINT,
	nvrhi::Format::RG32_SINT,
	nvrhi::Format::RGB32_SINT,
	nvrhi::Format::RGBA32_SINT,

	nvrhi::Format::R16_UINT,
	nvrhi::Format::RG16_UINT,
	nvrhi::Format::UNKNOWN, // RGB16UI not directly supported
	nvrhi::Format::RGBA16_UINT,

	nvrhi::Format::R32_UINT,
	nvrhi::Format::RG32_UINT,
	nvrhi::Format::RGB32_UINT,
	nvrhi::Format::RGBA32_UINT,

	nvrhi::Format::UNKNOWN, // RGBE8 not directly supported
	nvrhi::Format::UNKNOWN,
	nvrhi::Format::UNKNOWN,
	nvrhi::Format::UNKNOWN,
	nvrhi::Format::UNKNOWN, // RGBA4 not directly supported
	nvrhi::Format::UNKNOWN,	// RGB10A2 not directly supported

	nvrhi::Format::D16,
	nvrhi::Format::D24S8,
	nvrhi::Format::D24S8,
	nvrhi::Format::D32,

	nvrhi::Format::BC1_UNORM,
	nvrhi::Format::BC2_UNORM,
	nvrhi::Format::BC3_UNORM,
	nvrhi::Format::BC4_UNORM,
	nvrhi::Format::BC5_UNORM,

	// TODO: more BC

	nvrhi::Format::UNKNOWN,
	nvrhi::Format::UNKNOWN,
	nvrhi::Format::UNKNOWN,
	nvrhi::Format::UNKNOWN,
	nvrhi::Format::UNKNOWN,
	nvrhi::Format::UNKNOWN,
	nvrhi::Format::UNKNOWN,
	nvrhi::Format::UNKNOWN,
};

static nvrhi::Format GetNVRHIFormatSRGB(nvrhi::Format baseFormat)
{
	switch (baseFormat)
	{
	case nvrhi::Format::RGBA8_UNORM:
		return nvrhi::Format::SRGBA8_UNORM;
	case nvrhi::Format::BGRA8_UNORM:
		return nvrhi::Format::SBGRA8_UNORM;
	case nvrhi::Format::BC1_UNORM:
		return nvrhi::Format::BC1_UNORM_SRGB;
	case nvrhi::Format::BC2_UNORM:
		return nvrhi::Format::BC2_UNORM_SRGB;
	case nvrhi::Format::BC3_UNORM:
		return nvrhi::Format::BC3_UNORM_SRGB;
	}

	// TODO: ASTC
	return baseFormat;
}

static nvrhi::Format GetNVRHISwappedChannelsFormat(nvrhi::Format baseFormat)
{
	switch (baseFormat)
	{
	case nvrhi::Format::RGBA8_UNORM:
		return nvrhi::Format::BGRA8_UNORM;
	}

	return baseFormat;
}

static nvrhi::Format GetNVRHITextureFormat(ETextureFormat formatWithFlags)
{
	nvrhi::Format format = g_nvrhiTexFormats[GetTexFormat(formatWithFlags)];
	if (HasTexFormatFlags(formatWithFlags, TEXFORMAT_FLAG_SWAP_RB))
		format = GetNVRHISwappedChannelsFormat(format);

	if (HasTexFormatFlags(formatWithFlags, TEXFORMAT_FLAG_SRGB))
		format = GetNVRHIFormatSRGB(format);

	return format;
}

// EIndexFormat
static nvrhi::Format g_nvrhiIndexFormat[] = {
	nvrhi::Format::UNKNOWN,
	nvrhi::Format::R16_UINT,
	nvrhi::Format::R32_UINT,
};

// ETextureDimension
static nvrhi::TextureDimension g_nvrhiTexViewDimensions[] = {
	nvrhi::TextureDimension::Texture1D,
	nvrhi::TextureDimension::Texture2D,
	nvrhi::TextureDimension::Texture2DArray,
	nvrhi::TextureDimension::TextureCube,
	nvrhi::TextureDimension::TextureCubeArray,
	nvrhi::TextureDimension::Texture3D,
};

// EVertAttribFormat
static nvrhi::Format g_nvrhiVertexFormats[][4] = {
	{
		nvrhi::Format::UNKNOWN, nvrhi::Format::UNKNOWN, nvrhi::Format::UNKNOWN, nvrhi::Format::UNKNOWN
	},
	{
		nvrhi::Format::R8_UINT, nvrhi::Format::RG8_UINT, nvrhi::Format::UNKNOWN, nvrhi::Format::R32_UINT
	},
	{
		nvrhi::Format::R16_FLOAT, nvrhi::Format::RG16_FLOAT, nvrhi::Format::UNKNOWN, nvrhi::Format::RGBA16_FLOAT
	},
	{
		nvrhi::Format::R32_FLOAT, nvrhi::Format::RG32_FLOAT, nvrhi::Format::RGB32_FLOAT, nvrhi::Format::RGBA32_FLOAT
	},
};

// ECompareFunc
static nvrhi::ComparisonFunc g_nvrhiCompareFunc[] = {
	static_cast<nvrhi::ComparisonFunc>(0),
	nvrhi::ComparisonFunc::Never,
	nvrhi::ComparisonFunc::Less,	
	nvrhi::ComparisonFunc::Equal,	
	nvrhi::ComparisonFunc::LessOrEqual,	
	nvrhi::ComparisonFunc::Greater,
	nvrhi::ComparisonFunc::NotEqual,
	nvrhi::ComparisonFunc::GreaterOrEqual,	
	nvrhi::ComparisonFunc::Always,	
};

// EStencilFunc
static nvrhi::StencilOp g_nvrhiStencilOp[] = {
	nvrhi::StencilOp::Keep,
	nvrhi::StencilOp::Zero,
	nvrhi::StencilOp::Replace,
	nvrhi::StencilOp::Invert,
	nvrhi::StencilOp::IncrementAndWrap,
	nvrhi::StencilOp::DecrementAndWrap,
	nvrhi::StencilOp::IncrementAndClamp,
	nvrhi::StencilOp::DecrementAndClamp
};

// EBlendFunc
static nvrhi::BlendOp g_nvrhiBlendOp[] = {
	nvrhi::BlendOp::Add,
	nvrhi::BlendOp::Subtract,
	nvrhi::BlendOp::ReverseSubtract,
	nvrhi::BlendOp::Min,			
	nvrhi::BlendOp::Max,			
};

// EBlendFactor
static nvrhi::BlendFactor g_nvrhiBlendFactor[] = {
	nvrhi::BlendFactor::Zero,
	nvrhi::BlendFactor::One,
	nvrhi::BlendFactor::SrcColor,
	nvrhi::BlendFactor::OneMinusSrcColor,
	nvrhi::BlendFactor::DstColor,
	nvrhi::BlendFactor::OneMinusDstColor,
	nvrhi::BlendFactor::SrcAlpha,
	nvrhi::BlendFactor::OneMinusSrcAlpha,
	nvrhi::BlendFactor::DstAlpha,
	nvrhi::BlendFactor::OneMinusDstAlpha,
	nvrhi::BlendFactor::SrcAlphaSaturate,	
};

// ECullMode
static nvrhi::RasterCullMode g_nvrhiCullMode[] = {
	nvrhi::RasterCullMode::None,
	nvrhi::RasterCullMode::Back,
	nvrhi::RasterCullMode::Front,
};

// EPrimTopology

static nvrhi::PrimitiveType g_nvrhiPrimitiveType[] = {
	nvrhi::PrimitiveType::PointList,
	nvrhi::PrimitiveType::LineList,
	nvrhi::PrimitiveType::LineList, // TEMP, original: nvrhi::PrimitiveType::LineStrip,
	nvrhi::PrimitiveType::TriangleList,
	nvrhi::PrimitiveType::TriangleStrip
};

// ETexAddressMode
static nvrhi::SamplerAddressMode g_nvrhiAddressMode[] = {
	nvrhi::SamplerAddressMode::Repeat,
	nvrhi::SamplerAddressMode::ClampToEdge,
	nvrhi::SamplerAddressMode::MirroredRepeat
};

