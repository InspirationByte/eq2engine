//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Constant types for Equilibrium renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <d3d9.h>

#include "imaging/textureformats.h"
#include "renderers/ShaderAPI_defs.h"

#define FOURCC_INTZ ((D3DFORMAT)(MAKEFOURCC('I','N','T','Z')))
#define FOURCC_NULL ((D3DFORMAT)(MAKEFOURCC('N','U','L','L')))

// those tables are mapped to the EQ enums
extern const D3DBLEND				g_d3d9_blendingConsts[];
extern const D3DBLENDOP				g_d3d9_blendingModes[];
extern const D3DCMPFUNC				g_d3d9_depthConst[];
extern const D3DSTENCILOP			g_d3d9_stencilConst[];
extern const D3DCULL				g_d3d9_cullConst[];
extern const D3DFILLMODE			g_d3d9_fillConst[];
extern D3DFORMAT					g_d3d9_imageFormats[];
extern const D3DDECLTYPE			g_d3d9_decltypes[][4];
extern const D3DDECLUSAGE			g_d3d9_vertexUsage[];
extern const DWORD					g_d3d9_bufferUsages[];
extern const D3DPRIMITIVETYPE		g_d3d9_primType[];
extern const D3DTEXTUREFILTERTYPE	g_d3d9_texFilterType[];
extern const D3DTEXTUREADDRESS		g_d3d9_texAddressMode[];
extern const D3DTRANSFORMSTATETYPE	g_d3d9_matrixModes[];

inline static RECT IRectangleToD3DRECT(const IAARectangle& rect)
{
	RECT rekt;
	rekt.top = rect.leftTop.y;
	rekt.left = rect.leftTop.x;
	rekt.right = rect.rightBottom.x;
	rekt.bottom = rect.rightBottom.y;
	return rekt;
}

inline static D3DBOX IBoundingBoxToD3DBOX(const IBoundingBox& bbox)
{
	D3DBOX box;
	box.Left = bbox.minPoint.x;
	box.Top = bbox.maxPoint.y;
	box.Right = bbox.maxPoint.x;
	box.Bottom = bbox.minPoint.y;
	box.Front = bbox.maxPoint.z;
	box.Back = bbox.minPoint.z;
	return box;
}