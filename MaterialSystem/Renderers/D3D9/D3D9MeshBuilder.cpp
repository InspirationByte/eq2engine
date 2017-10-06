//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D9 Mesh builder
//////////////////////////////////////////////////////////////////////////////////

#include "D3D9MeshBuilder.h"

#define FVF_LISTVERTEX   (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)// | D3DFVF_TEXCOORDSIZE1(1) | D3DFVF_TEXCOORDSIZE2(0)

CD3D9MeshBuilder::CD3D9MeshBuilder(LPDIRECT3DDEVICE9 device) : m_vertList(1024)
{
	m_renderBegun = false;
	m_device = device;
}

CD3D9MeshBuilder::~CD3D9MeshBuilder()
{
	m_vertList.clear();
	if(m_renderBegun) m_device->Release();
}

//-----------------------------------------------------------------------------
// "glBegin()"

void CD3D9MeshBuilder::Begin( ER_PrimitiveType type)
{
	m_primType = type;

	m_vertList.setNum(0, false);

	m_curVertex.color = D3DCOLOR_COLORVALUE( 1, 1, 1, 1 );
	m_curVertex.pos = vec3_zero;
	m_curVertex.normal = Vector3D(0,1,0);
	m_curVertex.texCoord = vec2_zero;

	m_renderBegun = true;

	m_device->AddRef();
}
//-----------------------------------------------------------------------------
void CD3D9MeshBuilder::Position3f( float x, float y, float z )
{
	m_curVertex.pos.x = x;
	m_curVertex.pos.y = y;
	m_curVertex.pos.z = z;
}
//-----------------------------------------------------------------------------
void CD3D9MeshBuilder::Position3fv( const float* v )
{
	m_curVertex.pos.x = v[0];
	m_curVertex.pos.y = v[1];
	m_curVertex.pos.z = v[2];
}

//-----------------------------------------------------------------------------
void CD3D9MeshBuilder::AdvanceVertex()
{
	if(!m_renderBegun)
		return;

	m_vertList.append(m_curVertex);
}

//-----------------------------------------------------------------------------
void CD3D9MeshBuilder::Color4f(float r, float g, float b, float a )
{
	m_curVertex.color = D3DCOLOR_COLORVALUE( r, g, b, a );
}
//-----------------------------------------------------------------------------
void CD3D9MeshBuilder::Color4fv( const float* pColor )
{
	m_curVertex.color = D3DCOLOR_COLORVALUE( pColor[0], pColor[1], pColor[2], pColor[3] );
}

//-----------------------------------------------------------------------------
void CD3D9MeshBuilder::Color3f( float r, float g, float b )
{
	m_curVertex.color = D3DCOLOR_COLORVALUE( r, g, b, 1.0f );
}

//-----------------------------------------------------------------------------
void CD3D9MeshBuilder::Color3fv( float const *rgb )
{
	Color3f(rgb[0],rgb[1],rgb[2]);
}

//-----------------------------------------------------------------------------
void CD3D9MeshBuilder::Normal3f( float nx, float ny, float nz )
{
	m_curVertex.normal.x = nx;
	m_curVertex.normal.y = ny;
	m_curVertex.normal.z = nz;
}

//-----------------------------------------------------------------------------
void CD3D9MeshBuilder::Normal3fv( const float *v )
{
	m_curVertex.normal.x = v[0];
	m_curVertex.normal.y = v[1];
	m_curVertex.normal.z = v[2];
}

//-----------------------------------------------------------------------------
void CD3D9MeshBuilder::TexCoord2f( float tu, float tv )
{
	m_curVertex.texCoord.x = tu;
	m_curVertex.texCoord.y = tv;
}

//-----------------------------------------------------------------------------
void CD3D9MeshBuilder::TexCoord2fv( const float *v )
{
	m_curVertex.texCoord.x = v[0];
	m_curVertex.texCoord.y = v[1];
}

void CD3D9MeshBuilder::TexCoord3f( float s, float t, float r )
{
	m_curVertex.texCoord.x = s;
	m_curVertex.texCoord.y = t;
}

void CD3D9MeshBuilder::TexCoord3fv( const float *v )
{
	m_curVertex.texCoord.x = v[0];
	m_curVertex.texCoord.y = v[1];
}

//-----------------------------------------------------------------------------
void CD3D9MeshBuilder::End()
{
	if( !m_renderBegun )
		return;

	int nTris = s_DX9PrimitiveCounterFunctions[m_primType](m_vertList.numElem());

	if(nTris > 0)
	{
		m_device->SetFVF( FVF_LISTVERTEX );

		m_device->DrawPrimitiveUP( d3dPrim[m_primType], nTris, m_vertList.ptr(), sizeof(ListVertex_t) );

		m_device->SetFVF( 0 );
	}

	m_device->Release();

	m_renderBegun = false;
}