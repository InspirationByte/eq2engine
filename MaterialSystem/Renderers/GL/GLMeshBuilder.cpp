//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: GL Mesh builder
//////////////////////////////////////////////////////////////////////////////////

#include "GLMeshBuilder.h"
#include "DebugInterface.h"
#include "ShaderAPIGL.h"

#include <malloc.h>

// source-code based on this article:
// http://www.gamedev.net/reference/programming/features/imind3d/default.asp
// [concept ported to OpenGL]

#define MIN_VERTEX_LIST_SIZE	2048

#define MAX_VBO_VERTS			16384

struct ListVertex
{
	half x, y, z;
	half tu, tv;
	half nx, ny, nz;
	TVec4D<half> Diffuse;
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

static VertexFormatDesc_t g_meshBuilder_format[] = {
	0, 3, VERTEXTYPE_VERTEX,	ATTRIBUTEFORMAT_HALF,
	0, 2, VERTEXTYPE_TEXCOORD,	ATTRIBUTEFORMAT_HALF,
	0, 3, VERTEXTYPE_NORMAL,	ATTRIBUTEFORMAT_HALF,
	0, 4, VERTEXTYPE_COLOR,		ATTRIBUTEFORMAT_HALF,
};

CGLMeshBuilder::~CGLMeshBuilder()
{
	ASSERT(!bRenderBegun);
}

void CGLMeshBuilder::Begin( PrimitiveType_e Type)
{
	type = Type;

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
		MsgWarning("CGLMeshBuilder::AdvanceVertex(): OVERFLOW...\n");
		nVerts = 0;
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
		s_pBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_VBO_VERTS, sizeof(ListVertex), NULL);
		//s_pIndexBuffer = g_pShaderAPI->CreateIndexBuffer(MAX_VBO_VERTS*3, sizeof(int), BUFFER_DYNAMIC, NULL);

		s_pVertexFormat = g_pShaderAPI->CreateVertexFormat(g_meshBuilder_format, elementsOf(g_meshBuilder_format));
	}

	if(!s_pBuffer)
		return;

	s_pBuffer->Update(pVertList, nVerts, 0, true);

	int nIndices = 0;

	g_pShaderAPI->SetVertexFormat( s_pVertexFormat );
	g_pShaderAPI->SetVertexBuffer( s_pBuffer, 0 );
	g_pShaderAPI->SetIndexBuffer(NULL);

	// this call allows to use shaders internally
	((ShaderAPIGL*)g_pShaderAPI)->DrawMeshBufferPrimitives(type, nVerts, nIndices);

	free(pVertList);

	pVertList = NULL;
	nAllocated = 0;
	nVerts = 0;
}
