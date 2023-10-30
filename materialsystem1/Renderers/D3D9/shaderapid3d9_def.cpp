//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Constant types for Equilibrium renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/core_common.h"
#include "shaderapid3d9_def.h"

const D3DBLEND g_d3d9_blendingConsts[] = {
	D3DBLEND_ZERO,
	D3DBLEND_ONE,
	D3DBLEND_SRCCOLOR,
	D3DBLEND_INVSRCCOLOR,
	D3DBLEND_DESTCOLOR,
	D3DBLEND_INVDESTCOLOR,
	D3DBLEND_SRCALPHA,
	D3DBLEND_INVSRCALPHA,
	D3DBLEND_DESTALPHA,
	D3DBLEND_INVDESTALPHA,
	D3DBLEND_SRCALPHASAT,
};

const D3DBLENDOP g_d3d9_blendingModes[] = {
	D3DBLENDOP_ADD,
	D3DBLENDOP_SUBTRACT,
	D3DBLENDOP_REVSUBTRACT,
	D3DBLENDOP_MIN,
	D3DBLENDOP_MAX,
};

const D3DCMPFUNC g_d3d9_depthConst[] = {
	D3DCMP_NEVER,
	D3DCMP_LESS,
	D3DCMP_EQUAL,
	D3DCMP_LESSEQUAL,
	D3DCMP_GREATER,
	D3DCMP_NOTEQUAL,
	D3DCMP_GREATEREQUAL,
	D3DCMP_ALWAYS,
};

const D3DSTENCILOP g_d3d9_stencilConst[] = {
	D3DSTENCILOP_KEEP,
	D3DSTENCILOP_ZERO,
	D3DSTENCILOP_REPLACE,
	D3DSTENCILOP_INVERT,
	D3DSTENCILOP_INCR,
	D3DSTENCILOP_DECR,
	D3DSTENCILOP_INCRSAT,
	D3DSTENCILOP_DECRSAT,
};

const D3DCULL g_d3d9_cullConst[] = {
	D3DCULL_NONE,
	D3DCULL_CCW,
	D3DCULL_CW,
};

const D3DFILLMODE g_d3d9_fillConst[] = {
	D3DFILL_SOLID,
	D3DFILL_WIREFRAME,
	D3DFILL_POINT,
};

D3DFORMAT g_d3d9_imageFormats[] = {
	D3DFMT_UNKNOWN,

	// Unsigned formats
	D3DFMT_L8,
	D3DFMT_A8L8,
	D3DFMT_X8R8G8B8,
	D3DFMT_A8R8G8B8,

	D3DFMT_L16,
	D3DFMT_G16R16,
	D3DFMT_UNKNOWN, // RGB16 not directly supported
	D3DFMT_A16B16G16R16,

	// Signed formats
	D3DFMT_UNKNOWN,
	D3DFMT_V8U8,
	D3DFMT_UNKNOWN,
	D3DFMT_Q8W8V8U8,

	D3DFMT_UNKNOWN,
	D3DFMT_V16U16,
	D3DFMT_UNKNOWN,
	D3DFMT_Q16W16V16U16,

	// Float formats
	D3DFMT_R16F,
	D3DFMT_G16R16F,
	D3DFMT_UNKNOWN, // RGB16F not directly supported
	D3DFMT_A16B16G16R16F,

	D3DFMT_R32F,
	D3DFMT_G32R32F,
	D3DFMT_UNKNOWN, // RGB32F not directly supported
	D3DFMT_A32B32G32R32F,

	// Signed integer formats
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,

	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,

	// Unsigned integer formats
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,

	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,

	// Packed formats
	D3DFMT_UNKNOWN, // RGBE8 not directly supported
	D3DFMT_UNKNOWN, // RGB9E5 not supported
	D3DFMT_UNKNOWN, // RG11B10F not supported
	D3DFMT_R5G6B5,
	D3DFMT_A4R4G4B4,
	D3DFMT_A2B10G10R10,

	// Depth formats
	D3DFMT_D16,
	D3DFMT_D24X8,
	D3DFMT_D24S8,
	D3DFMT_D32F_LOCKABLE,

	// Compressed formats
	D3DFMT_DXT1,
	D3DFMT_DXT3,
	D3DFMT_DXT5,
	(D3DFORMAT)'1ITA', // 3Dc 1 channel
	(D3DFORMAT)'2ITA', // 3Dc 2 channels

	// other compressed formats are unsupported
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN
};

const D3DDECLTYPE g_d3d9_decltypes[][4] = {
	{ // ATTRIBUTEFORMAT_NONE
		D3DDECLTYPE_UNUSED, D3DDECLTYPE_UNUSED, D3DDECLTYPE_UNUSED, D3DDECLTYPE_UNUSED
	},
	{ // ATTRIBUTEFORMAT_UINT8
		D3DDECLTYPE_UNUSED, D3DDECLTYPE_UNUSED,    D3DDECLTYPE_UNUSED, D3DDECLTYPE_UBYTE4N
	},
	{ // ATTRIBUTEFORMAT_HALF
		D3DDECLTYPE_UNUSED, D3DDECLTYPE_FLOAT16_2, D3DDECLTYPE_UNUSED, D3DDECLTYPE_FLOAT16_4
	},
	{ // ATTRIBUTEFORMAT_FLOAT
		D3DDECLTYPE_FLOAT1, D3DDECLTYPE_FLOAT2,    D3DDECLTYPE_FLOAT3, D3DDECLTYPE_FLOAT4
	}
};

const D3DDECLUSAGE g_d3d9_vertexUsage[] = {
	(D3DDECLUSAGE) (-1), 
	D3DDECLUSAGE_COLOR,
	D3DDECLUSAGE_POSITION, 
	D3DDECLUSAGE_TEXCOORD,
	D3DDECLUSAGE_NORMAL,
	D3DDECLUSAGE_TANGENT,
	D3DDECLUSAGE_BINORMAL,
};

const DWORD g_d3d9_bufferUsages[] = {
	D3DUSAGE_DYNAMIC,
	0,
	D3DUSAGE_DYNAMIC,
};

const D3DPRIMITIVETYPE g_d3d9_primType[] = {
	D3DPT_POINTLIST,
	D3DPT_LINELIST,
	D3DPT_LINESTRIP,
	D3DPT_TRIANGLELIST,
	D3DPT_TRIANGLESTRIP
};

const D3DTEXTUREFILTERTYPE g_d3d9_texFilterType[] = {
    D3DTEXF_POINT,
    D3DTEXF_LINEAR,
	D3DTEXF_LINEAR,
	D3DTEXF_LINEAR,
    D3DTEXF_ANISOTROPIC,
	D3DTEXF_ANISOTROPIC,
};

const D3DTEXTUREADDRESS g_d3d9_texAddressMode[] = {
	D3DTADDRESS_WRAP,
	D3DTADDRESS_CLAMP,
	D3DTADDRESS_MIRROR
};

const D3DTRANSFORMSTATETYPE g_d3d9_matrixModes[] = {
	D3DTS_VIEW,
	D3DTS_PROJECTION,
	D3DTS_WORLD,
	D3DTS_WORLD1,
	D3DTS_TEXTURE0
};