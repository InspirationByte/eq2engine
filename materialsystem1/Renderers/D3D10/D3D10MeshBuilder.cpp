//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D9 Mesh builder
//////////////////////////////////////////////////////////////////////////////////

#include "D3D10MeshBuilder.h"
#include "DebugInterface.h"
#include "ShaderAPID3DX10.h"

/*
#include <d3d10.h>
#include <d3dx10.h>
#include "d3dx10_def.h"
#include "Renderer/ShaderAPI_defs.h"
*/
#include <malloc.h>


// source-code based on this article:
// http://www.gamedev.net/reference/programming/features/imind3d/default.asp
// [concept ported to DirectX10]

//#define FVF_LISTVERTEX   (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)// | D3DFVF_TEXCOORDSIZE1(1) | D3DFVF_TEXCOORDSIZE2(0)

#define MIN_VERTEX_LIST_SIZE	2048

#define MAX_VBO_VERTS			8192

//extern ID3D10Device* pXDevice;

struct ListVertex
{
	float x, y, z;
	float tu, tv;//, tr;
	ColorRGBA Diffuse;
	float nx, ny, nz;
};

static bool bRenderBegun =  FALSE;

//static D3D10_PRIMITIVE_TOPOLOGY PrimitiveType = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
static ER_PrimitiveType type;

static ListVertex *pVertList = NULL;
ListVertex fill_vertex;
static int nVerts = 0;
static int nAllocated = MIN_VERTEX_LIST_SIZE;

static IVertexBuffer*		s_pBuffer			= NULL;
static IIndexBuffer*		s_pIndexBuffer		= NULL;
static IVertexFormat*		s_pVertexFormat	= NULL;

//-----------------------------------------------------------------------------
// "glBegin()"

static VertexFormatDesc_t g_meshBuilder_format[] = {
	0, 3, VERTEXATTRIB_POSITION,	ATTRIBUTEFORMAT_FLOAT,
	0, 2, VERTEXATTRIB_TEXCOORD,	ATTRIBUTEFORMAT_FLOAT,
	0, 4, VERTEXATTRIB_COLOR,		ATTRIBUTEFORMAT_FLOAT,
	0, 3, VERTEXATTRIB_NORMAL,	ATTRIBUTEFORMAT_FLOAT,
};

CD3D10MeshBuilder::~CD3D10MeshBuilder()
{
	ASSERT(!bRenderBegun);
}

void CD3D10MeshBuilder::Begin( ER_PrimitiveType Type)
{
	//if( pXDevice == NULL )
	//	return;

	//pXDevice->AddRef();

	type = Type;
	//PrimitiveType = d3dPrim[Type];

	nAllocated = MIN_VERTEX_LIST_SIZE;
	pVertList = (ListVertex*)realloc(pVertList, sizeof(ListVertex)*nAllocated);
	nVerts = 0;

	fill_vertex.Diffuse = ColorRGBA( 1, 1, 1, 1 );

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
void CD3D10MeshBuilder::Position3f( float x, float y, float z )
{
	if( !bRenderBegun )
		return;

	fill_vertex.x = x;
	fill_vertex.y = y;
	fill_vertex.z = z;
}
//-----------------------------------------------------------------------------
void CD3D10MeshBuilder::Position3fv( const float* v )
{
	Position3f( v[0], v[1], v[2] );
}

//-----------------------------------------------------------------------------
void CD3D10MeshBuilder::AdvanceVertex()
{
	int nVertsAfter = nVerts+1;

	if(nVertsAfter > MAX_VBO_VERTS)
	{
		MsgWarning("CD3D10MeshBuilder::AdvanceVertex(): OVERFLOW...\n");
		return;
	}

	if(nVertsAfter > nAllocated)
	{
		nAllocated += MIN_VERTEX_LIST_SIZE;
		pVertList = (ListVertex*)realloc(pVertList, sizeof(ListVertex)*nAllocated);
	}

	pVertList[nVerts] = fill_vertex;

	nVerts++;
}

//-----------------------------------------------------------------------------
void CD3D10MeshBuilder::Color4f(float r, float g, float b, float a )
{
	fill_vertex.Diffuse = ColorRGBA( r, g, b, a );
}

//-----------------------------------------------------------------------------
void CD3D10MeshBuilder::Color4fv( const float* pColor )
{
	Color4f( pColor[0], pColor[1], pColor[2], pColor[3] );
}

//-----------------------------------------------------------------------------
void CD3D10MeshBuilder::Color3f( float r, float g, float b )
{
	fill_vertex.Diffuse = ColorRGBA( r, g, b, 1.0f );
}

//-----------------------------------------------------------------------------
void CD3D10MeshBuilder::Color3fv( float const *rgb )
{
	Color3f(rgb[0],rgb[1],rgb[2]);
}

//-----------------------------------------------------------------------------
void CD3D10MeshBuilder::Normal3f( float nx, float ny, float nz )
{
	fill_vertex.nx = nx;
	fill_vertex.ny = ny;
	fill_vertex.nz = nz;
}

//-----------------------------------------------------------------------------
void CD3D10MeshBuilder::Normal3fv( const float *v )
{
	fill_vertex.nx = v[0];
	fill_vertex.ny = v[1];
	fill_vertex.nz = v[2];
}

//-----------------------------------------------------------------------------
void CD3D10MeshBuilder::TexCoord2f( float tu, float tv )
{
	fill_vertex.tu = tu;
	fill_vertex.tv = tv;
	//fill_vertex.tr = 0;
}

//-----------------------------------------------------------------------------
void CD3D10MeshBuilder::TexCoord2fv( const float *v )
{
	fill_vertex.tu = v[0];
	fill_vertex.tv = v[1];
	//fill_vertex.tr = v[2];
}

void CD3D10MeshBuilder::TexCoord3f( float s, float t, float r )
{
	fill_vertex.tu = s;
	fill_vertex.tv = t;
	//fill_vertex.tr = r;
}

void CD3D10MeshBuilder::TexCoord3fv( const float *v )
{
	fill_vertex.tu = v[0];
	fill_vertex.tv = v[1];
	//fill_vertex.tr = v[2];
}

//-----------------------------------------------------------------------------
void CD3D10MeshBuilder::End()
{
	if( !bRenderBegun)
		return;

	if(!nVerts)
	{
		bRenderBegun = FALSE;
		return;
	}

	g_pShaderAPI->Reset(STATE_RESET_VBO);
	g_pShaderAPI->ApplyBuffers();

	if(!s_pBuffer)
	{
		s_pBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_VBO_VERTS, sizeof(ListVertex), nullptr);
		s_pIndexBuffer = g_pShaderAPI->CreateIndexBuffer(MAX_VBO_VERTS*3, sizeof(int), BUFFER_DYNAMIC, nullptr);

		s_pVertexFormat = g_pShaderAPI->CreateVertexFormat(g_meshBuilder_format, elementsOf(g_meshBuilder_format));
	}

	// WARNING!
	// FANS NOT SUPPORTED
	// YES! ACTUALLY FANS! :P

	void* pLockData = NULL;

	if(s_pBuffer->Lock(0, nVerts, &pLockData, false))
	{
		memcpy( pLockData, pVertList, sizeof(ListVertex)*nVerts);
		s_pBuffer->Unlock();
	}
	else
		MsgError("CD3D10MeshBuilder lock of VB failed!\n");

	int nIndices = 0;

	if(type == PRIM_TRIANGLE_FAN)
	{
		// build a new index list
		type = PRIM_TRIANGLES;

		int num_triangles = ((nVerts < 4) ? 1 : (2 + nVerts - 4));
		int num_indices = num_triangles*3;

		if(nVerts < 3)
		{
			num_triangles = 0;
			num_indices = 0;
		}

		int* pIndices = NULL;
		if(s_pIndexBuffer->Lock(0, num_indices, (void**)&pIndices, false))
		{
			// fill out buffer
			for(int i = 0; i < num_triangles; i++)
			{
				int idx0 = 0;
				int idx1 = i+1;
				int idx2 = i+2;

				pIndices[i*3] = idx0;
				pIndices[i*3 +1] = idx1;
				pIndices[i*3 +2] = idx2;
			}

			s_pIndexBuffer->Unlock();
		}

		nIndices = num_indices;

		g_pShaderAPI->SetIndexBuffer(s_pIndexBuffer);
	}
	else
		g_pShaderAPI->SetIndexBuffer(NULL);

	g_pShaderAPI->SetVertexFormat( s_pVertexFormat );
	g_pShaderAPI->SetVertexBuffer( s_pBuffer, 0 );


	// this call allows to use shaders internally
	((ShaderAPID3DX10*)g_pShaderAPI)->DrawMeshBufferPrimitives(type, nVerts, nIndices);

	//pXDevice->IASetVertexBuffers();
	//pXDevice->IASetInputLayout();
	//pXDevice->IASetPrimitiveTopology( PrimitiveType );
	//pXDevice->Draw( nVerts, 0);

	//pXDevice->Release();

	free(pVertList);

	pVertList = NULL;
	nAllocated = 0;
	nVerts = 0;

	bRenderBegun = FALSE;
}