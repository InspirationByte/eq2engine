//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: GL Mesh builder
//////////////////////////////////////////////////////////////////////////////////

#include "GLMeshBuilder.h"
#include "DebugInterface.h"
#include "ShaderAPIGL.h"

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

static bool bRenderBegun = false;

//static D3D10_PRIMITIVE_TOPOLOGY PrimitiveType = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
static PrimitiveType_e type;

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
	0, 3, VERTEXTYPE_VERTEX,	ATTRIBUTEFORMAT_FLOAT,
	0, 2, VERTEXTYPE_TEXCOORD,	ATTRIBUTEFORMAT_FLOAT,
	0, 4, VERTEXTYPE_COLOR,		ATTRIBUTEFORMAT_FLOAT,
	0, 3, VERTEXTYPE_NORMAL,	ATTRIBUTEFORMAT_FLOAT,
};

CGLMeshBuilder::~CGLMeshBuilder()
{
	ASSERT(!bRenderBegun);
}

void CGLMeshBuilder::Begin( PrimitiveType_e Type)
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

	bRenderBegun = true;
}
//-----------------------------------------------------------------------------
void CGLMeshBuilder::Position3f( float x, float y, float z )
{
	if( !bRenderBegun )
		return;

	fill_vertex.x = x;
	fill_vertex.y = y;
	fill_vertex.z = z;
}
//-----------------------------------------------------------------------------
void CGLMeshBuilder::Position3fv( const float* v )
{
	Position3f( v[0], v[1], v[2] );
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::AdvanceVertex()
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
void CGLMeshBuilder::Color4f(float r, float g, float b, float a )
{
	fill_vertex.Diffuse = ColorRGBA( r, g, b, a );
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::Color4fv( const float* pColor )
{
	Color4f( pColor[0], pColor[1], pColor[2], pColor[3] );
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::Color3f( float r, float g, float b )
{
	fill_vertex.Diffuse = ColorRGBA( r, g, b, 1.0f );
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::Color3fv( float const *rgb )
{
	Color3f(rgb[0],rgb[1],rgb[2]);
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::Normal3f( float nx, float ny, float nz )
{
	fill_vertex.nx = nx;
	fill_vertex.ny = ny;
	fill_vertex.nz = nz;
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::Normal3fv( const float *v )
{
	fill_vertex.nx = v[0];
	fill_vertex.ny = v[1];
	fill_vertex.nz = v[2];
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::TexCoord2f( float tu, float tv )
{
	fill_vertex.tu = tu;
	fill_vertex.tv = tv;
	//fill_vertex.tr = 0;
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::TexCoord2fv( const float *v )
{
	fill_vertex.tu = v[0];
	fill_vertex.tv = v[1];
	//fill_vertex.tr = v[2];
}

void CGLMeshBuilder::TexCoord3f( float s, float t, float r )
{
	fill_vertex.tu = s;
	fill_vertex.tv = t;
	//fill_vertex.tr = r;
}

void CGLMeshBuilder::TexCoord3fv( const float *v )
{
	fill_vertex.tu = v[0];
	fill_vertex.tv = v[1];
	//fill_vertex.tr = v[2];
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::End()
{
	if( !bRenderBegun)
		return;

	if(!nVerts)
	{
		bRenderBegun = false;
		return;
	}

	bRenderBegun = false;

	g_pShaderAPI->Reset(STATE_RESET_VBO);
	g_pShaderAPI->ApplyBuffers();

	if(!s_pBuffer)
	{
		//ubyte* pTemp = (ubyte*)malloc(MAX_VBO_VERTS*sizeof(ListVertex));

		s_pBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_VBO_VERTS, sizeof(ListVertex), NULL);
		s_pIndexBuffer = g_pShaderAPI->CreateIndexBuffer(MAX_VBO_VERTS*3, sizeof(int), BUFFER_DYNAMIC, NULL);

		s_pVertexFormat = g_pShaderAPI->CreateVertexFormat(g_meshBuilder_format, elementsOf(g_meshBuilder_format));
	}

	if(!s_pBuffer)
		return;

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
		MsgError("CGLMeshBuilder lock of VB failed!\n");

	int nIndices = 0;
	
	/*
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
	else*/
		g_pShaderAPI->SetIndexBuffer(NULL);

	g_pShaderAPI->SetVertexFormat( s_pVertexFormat );
	g_pShaderAPI->SetVertexBuffer( s_pBuffer, 0 );

	// this call allows to use shaders internally
	((ShaderAPIGL*)g_pShaderAPI)->DrawMeshBufferPrimitives(type, nVerts, nIndices);

	free(pVertList);

	pVertList = NULL;
	nAllocated = 0;
	nVerts = 0;

	
}