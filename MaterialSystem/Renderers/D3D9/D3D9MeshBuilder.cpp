//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D9 Mesh builder
//////////////////////////////////////////////////////////////////////////////////

#include "D3D9MeshBuilder.h"

#include <d3d9.h>
#include <d3dx9.h>

#include "d3dx9_def.h"

#include "utils/dklist.h"

#define FVF_LISTVERTEX   (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)// | D3DFVF_TEXCOORDSIZE1(1) | D3DFVF_TEXCOORDSIZE2(0)

#define MIN_VERTEX_LIST_SIZE 2048

extern LPDIRECT3DDEVICE9 pXDevice;

struct ListVertex
{
	float x, y, z;
	float nx, ny, nz;
	D3DCOLOR Diffuse;
	float tu, tv;//, tr;
};

static BOOL bRenderBegun =  FALSE;

static D3DPRIMITIVETYPE PrimitiveType = D3DPT_TRIANGLELIST;
static PrimitiveType_e type;

static ListVertex *pVertList = NULL;
ListVertex fill_vertex;
static int nVerts = 0;
static int nAllocated = MIN_VERTEX_LIST_SIZE;

//-----------------------------------------------------------------------------
// "glBegin()"

void D3D9MeshBuilder::Begin( PrimitiveType_e Type)
{
	if( pXDevice == NULL )
		return;

	pXDevice->AddRef();

	type = Type;
	PrimitiveType = d3dPrim[Type];

	nAllocated = MIN_VERTEX_LIST_SIZE;
	pVertList = (ListVertex*)realloc(pVertList, sizeof(ListVertex)*nAllocated);
	nVerts = 0;

	fill_vertex.Diffuse = D3DCOLOR_COLORVALUE( 1, 1, 1, 1 );

	fill_vertex.x = 0;
	fill_vertex.y = 0;
	fill_vertex.z = 0;

	fill_vertex.nx = 0;
	fill_vertex.ny = 0;
	fill_vertex.nz = 0;

	fill_vertex.tu = 0;
	fill_vertex.tv = 0;

	bRenderBegun = TRUE;
}
//-----------------------------------------------------------------------------
void D3D9MeshBuilder::Position3f( float x, float y, float z )
{
	if( !bRenderBegun )
		return;

	fill_vertex.x = x;
	fill_vertex.y = y;
	fill_vertex.z = z;
}
//-----------------------------------------------------------------------------
void D3D9MeshBuilder::Position3fv( const float* v )
{
	Position3f( v[0], v[1], v[2] );
}

//-----------------------------------------------------------------------------
void D3D9MeshBuilder::AdvanceVertex()
{
	int nVertsAfter = nVerts+1;

	if(nVertsAfter > nAllocated)
	{
		nAllocated += MIN_VERTEX_LIST_SIZE;
		pVertList = (ListVertex*)realloc(pVertList, sizeof(ListVertex)*nAllocated);
	}

	pVertList[nVerts] = fill_vertex;

	nVerts++;
}

//-----------------------------------------------------------------------------
void D3D9MeshBuilder::Color4f(float r, float g, float b, float a )
{
	fill_vertex.Diffuse = D3DCOLOR_COLORVALUE( r, g, b, a );
}
//-----------------------------------------------------------------------------
void D3D9MeshBuilder::Color4fv( const float* pColor )
{
	Color4f( pColor[0], pColor[1], pColor[2], pColor[3] );
}

//-----------------------------------------------------------------------------
void D3D9MeshBuilder::Color3f( float r, float g, float b )
{
	fill_vertex.Diffuse = D3DCOLOR_COLORVALUE( r, g, b, 1.0f );
}

//-----------------------------------------------------------------------------
void D3D9MeshBuilder::Color3fv( float const *rgb )
{
	Color3f(rgb[0],rgb[1],rgb[2]);
}

//-----------------------------------------------------------------------------
void D3D9MeshBuilder::Normal3f( float nx, float ny, float nz )
{
	fill_vertex.nx = nx;
	fill_vertex.ny = ny;
	fill_vertex.nz = nz;
}

//-----------------------------------------------------------------------------
void D3D9MeshBuilder::Normal3fv( const float *v )
{
	fill_vertex.nx = v[0];
	fill_vertex.ny = v[1];
	fill_vertex.nz = v[2];
}

//-----------------------------------------------------------------------------
void D3D9MeshBuilder::TexCoord2f( float tu, float tv )
{
	fill_vertex.tu = tu;
	fill_vertex.tv = tv;
	//fill_vertex.tr = 0;
}

//-----------------------------------------------------------------------------
void D3D9MeshBuilder::TexCoord2fv( const float *v )
{
	fill_vertex.tu = v[0];
	fill_vertex.tv = v[1];
	//fill_vertex.tr = v[2];
}

void D3D9MeshBuilder::TexCoord3f( float s, float t, float r )
{
	fill_vertex.tu = s;
	fill_vertex.tv = t;
	//fill_vertex.tr = r;
}

void D3D9MeshBuilder::TexCoord3fv( const float *v )
{
	fill_vertex.tu = v[0];
	fill_vertex.tv = v[1];
	//fill_vertex.tr = v[2];
}

//-----------------------------------------------------------------------------
void D3D9MeshBuilder::End()
{
	if( !bRenderBegun ) return;

	int NumPrimitives = 0;

	switch( PrimitiveType )
	{
	case D3DPT_POINTLIST:
		NumPrimitives = nVerts;
		break;
	case D3DPT_LINELIST:
		NumPrimitives = nVerts / 2;
		break;
	case D3DPT_LINESTRIP:
		NumPrimitives = nVerts - 1;
		break;
	case D3DPT_TRIANGLELIST:
		NumPrimitives = nVerts / 3;
		break;
	case D3DPT_TRIANGLESTRIP:
		NumPrimitives = nVerts - 2;
		break;
	case D3DPT_TRIANGLEFAN:
		NumPrimitives = nVerts - 2;
	} 

	if(NumPrimitives > 0)
	{
		pXDevice->SetFVF( FVF_LISTVERTEX );

		HRESULT r = pXDevice->DrawPrimitiveUP( PrimitiveType, NumPrimitives, pVertList, sizeof(ListVertex) );

		pXDevice->Release();
	}

	free(pVertList);

	pVertList = NULL;
	nAllocated = 0;
	nVerts = 0;

	bRenderBegun = FALSE;
}