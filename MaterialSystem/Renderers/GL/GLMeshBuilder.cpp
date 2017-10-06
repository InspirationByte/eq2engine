//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: GL Mesh builder
//////////////////////////////////////////////////////////////////////////////////

#include "GLMeshBuilder.h"
#include "DebugInterface.h"
#include "ShaderAPIGL.h"

#define MAX_VBO_VERTS		(32767)

//-----------------------------------------------------------------------------

static VertexFormatDesc_t g_meshBuilder_format[] = {
	0, 3, VERTEXATTRIB_POSITION,	ATTRIBUTEFORMAT_HALF,
	0, 2, VERTEXATTRIB_TEXCOORD,	ATTRIBUTEFORMAT_HALF,
	0, 3, VERTEXATTRIB_NORMAL,	ATTRIBUTEFORMAT_HALF,
	0, 4, VERTEXATTRIB_COLOR,		ATTRIBUTEFORMAT_HALF,
};

CGLMeshBuilder::CGLMeshBuilder()
{
	m_renderBegun = false;
	m_vertexBuffer	= NULL;
	m_indexBuffer	= NULL;
	m_vertexFormat	= NULL;

	//m_vertexBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_VBO_VERTS, sizeof(ListVertex_t), NULL);
	//m_indexBuffer = g_pShaderAPI->CreateIndexBuffer(MAX_VBO_VERTS*3, sizeof(int), BUFFER_DYNAMIC, NULL);
	//m_vertexFormat = g_pShaderAPI->CreateVertexFormat(g_meshBuilder_format, elementsOf(g_meshBuilder_format));
}

CGLMeshBuilder::~CGLMeshBuilder()
{
	m_vertList.clear();

	//g_pShaderAPI->DestroyVertexBuffer(m_vertexBuffer);
	//g_pShaderAPI->DestroyIndexBuffer(m_indexBuffer);
	//g_pShaderAPI->DestroyVertexFormat(m_vertexFormat);
}

//-----------------------------------------------------------------------------
// "glBegin()"

void CGLMeshBuilder::Begin( ER_PrimitiveType type)
{
	m_primType = type;

	m_vertList.setNum(0, false);

	m_curVertex.color = TVec4D<half>( 1.0f, 1.0f, 1.0f, 1.0f );
	m_curVertex.pos = vec3_zero;
	m_curVertex.normal = Vector3D(0,1,0);
	m_curVertex.texCoord = vec2_zero;

	m_renderBegun = true;
}
//-----------------------------------------------------------------------------
void CGLMeshBuilder::Position3f( float x, float y, float z )
{
	m_curVertex.pos.x = x;
	m_curVertex.pos.y = y;
	m_curVertex.pos.z = z;
}
//-----------------------------------------------------------------------------
void CGLMeshBuilder::Position3fv( const float* v )
{
	m_curVertex.pos.x = v[0];
	m_curVertex.pos.y = v[1];
	m_curVertex.pos.z = v[2];
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::AdvanceVertex()
{
	if(!m_renderBegun)
		return;

	m_vertList.append(m_curVertex);

	if(m_vertList.numElem() > MAX_VBO_VERTS)
	{
		m_vertList.setNum(0, false);
		MsgWarning("CGLMeshBuilder vertex list overflow!\n");
	}
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::Color4f(float r, float g, float b, float a )
{
	m_curVertex.color = TVec4D<half>( r, g, b, a );
}
//-----------------------------------------------------------------------------
void CGLMeshBuilder::Color4fv( const float* pColor )
{
	m_curVertex.color = TVec4D<half>( pColor[0], pColor[1], pColor[2], pColor[3] );
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::Color3f( float r, float g, float b )
{
	m_curVertex.color = TVec4D<half>( r, g, b, 1.0f );
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::Color3fv( float const *rgb )
{
	Color3f(rgb[0],rgb[1],rgb[2]);
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::Normal3f( float nx, float ny, float nz )
{
	m_curVertex.normal.x = nx;
	m_curVertex.normal.y = ny;
	m_curVertex.normal.z = nz;
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::Normal3fv( const float *v )
{
	m_curVertex.normal.x = v[0];
	m_curVertex.normal.y = v[1];
	m_curVertex.normal.z = v[2];
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::TexCoord2f( float tu, float tv )
{
	m_curVertex.texCoord.x = tu;
	m_curVertex.texCoord.y = tv;
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::TexCoord2fv( const float *v )
{
	m_curVertex.texCoord.x = v[0];
	m_curVertex.texCoord.y = v[1];
}

void CGLMeshBuilder::TexCoord3f( float s, float t, float r )
{
	m_curVertex.texCoord.x = s;
	m_curVertex.texCoord.y = t;
}

void CGLMeshBuilder::TexCoord3fv( const float *v )
{
	m_curVertex.texCoord.x = v[0];
	m_curVertex.texCoord.y = v[1];
}

//-----------------------------------------------------------------------------
void CGLMeshBuilder::End()
{
	if( !m_renderBegun )
		return;

	int numVerts = m_vertList.numElem();

	if(numVerts > 0)
	{
		g_pShaderAPI->Reset(STATE_RESET_VBO);
		g_pShaderAPI->ApplyBuffers();

		m_vertexBuffer->Update(m_vertList.ptr(), numVerts, 0, true);

		int nIndices = 0;

		g_pShaderAPI->SetVertexFormat( m_vertexFormat );
		g_pShaderAPI->SetVertexBuffer( m_vertexBuffer, 0 );
		g_pShaderAPI->SetIndexBuffer(NULL);

		// this call allows to use shaders internally
		((ShaderAPIGL*)g_pShaderAPI)->DrawMeshBufferPrimitives(m_primType, numVerts, nIndices);
	}

	m_renderBegun = false;
}