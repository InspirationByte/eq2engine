//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: a mesh builder designed for dynamic meshes (material system)
//////////////////////////////////////////////////////////////////////////////////

#include "MeshBuilder.h"
#include "DebugInterface.h"

CMeshBuilder::CMeshBuilder(IDynamicMesh* mesh) :
	m_mesh(NULL),
	m_curVertex(NULL),
	m_formatDesc(NULL),
	m_begun(false),
	m_gotoNext(false)
{
	m_mesh = mesh;

	// get the format offsets
	int numAttribs;
	m_mesh->GetVertexFormatDesc(&m_formatDesc, numAttribs);

	int vertexSize = 0;
	for(int i = 0; i < numAttribs; i++)
	{
		//int stream = m_formatDesc[i].m_nStream;
		AttributeFormat_e format = m_formatDesc[i].m_nFormat;
		VertexType_e type = m_formatDesc[i].m_nType;
		int vecCount = m_formatDesc[i].m_nSize;
		int attribSize = vecCount * attributeFormatSize[format];

		if(type == VERTEXTYPE_VERTEX)
		{
			m_fmtPosition.offset = vertexSize;
			m_fmtPosition.count = vecCount;
			m_fmtPosition.format = format;
		}
		else if(type == VERTEXTYPE_NORMAL)
		{
			m_fmtNormal.offset = vertexSize;
			m_fmtNormal.count = vecCount;
			m_fmtNormal.format = format;
		}
		else if(type == VERTEXTYPE_TEXCOORD)
		{
			m_fmtTexcoord.offset = vertexSize;
			m_fmtTexcoord.count = vecCount;
			m_fmtTexcoord.format = format;
		}
		else if(type == VERTEXTYPE_COLOR)
		{
			m_fmtColor.offset = vertexSize;
			m_fmtColor.count = vecCount;
			m_fmtColor.format = format;
		}

		// TODO: add Tangent and Binormal

		vertexSize += attribSize;
	}
}

CMeshBuilder::~CMeshBuilder()
{

}

// begins the mesh
void CMeshBuilder::Begin(PrimitiveType_e type)
{
	m_mesh->Reset();
	m_mesh->SetPrimitiveType(type);

	m_begun = true;
	m_gotoNext = true;
}

// ends building and renders the mesh
void CMeshBuilder::End()
{
	m_mesh->Render();
	m_begun = false;
}

// position setup
void CMeshBuilder::Position3f(float x, float y, float z)
{
	GotoNextVertex();

	if(!m_begun || m_fmtPosition.offset < 0|| m_fmtPosition.count < 3)
		return;

	ubyte* dest = (ubyte*)m_curVertex + m_fmtPosition.offset;

	switch(m_fmtPosition.format)
	{
		case ATTRIBUTEFORMAT_FLOAT:
		{
			float* val = (float*)dest;
			*val++ = x;
			*val++ = y;
			*val++ = z;
			break;
		}
		case ATTRIBUTEFORMAT_HALF:
		{
			half* val = (half*)dest;
			*val++ = x;
			*val++ = y;
			*val++ = z;
			break;
		}
		default:
			// unsupported
			break;
	}
}

void CMeshBuilder::Position3fv(const float *v)
{
	Position3f(v[0], v[1], v[2]);
}

// normal setup
void CMeshBuilder::Normal3f(float nx, float ny, float nz)
{
	GotoNextVertex();

	if(!m_begun || m_fmtNormal.offset < 0 || m_fmtNormal.count < 3)
		return;

	ubyte* dest = (ubyte*)m_curVertex + m_fmtNormal.offset;

	switch(m_fmtNormal.format)
	{
		case ATTRIBUTEFORMAT_FLOAT:
		{
			float* val = (float*)dest;
			*val++ = nx;
			*val++ = ny;
			*val++ = nz;
			break;
		}
		case ATTRIBUTEFORMAT_HALF:
		{
			half* val = (half*)dest;
			*val++ = nx;
			*val++ = ny;
			*val++ = nz;
			break;
		}
		case ATTRIBUTEFORMAT_UBYTE:
		{
			ubyte* val = (ubyte*)dest;
			*val++ = nx / 255.0f;
			*val++ = ny / 255.0f;
			*val++ = nz / 255.0f;
			break;
		}
	}
}

void CMeshBuilder::Normal3fv(const float *v)
{
	Normal3f(v[0], v[1], v[2]);
}

void CMeshBuilder::TexCoord2f(float s, float t)
{
	GotoNextVertex();

	if(!m_begun || m_fmtTexcoord.offset < 0 || m_fmtTexcoord.count < 2)
		return;

	ubyte* dest = (ubyte*)m_curVertex + m_fmtTexcoord.offset;

	switch(m_fmtTexcoord.format)
	{
		case ATTRIBUTEFORMAT_FLOAT:
		{
			float* val = (float*)dest;
			*val++ = s;
			*val++ = t;
			break;
		}
		case ATTRIBUTEFORMAT_HALF:
		{
			half* val = (half*)dest;
			*val++ = s;
			*val++ = t;
			break;
		}
		case ATTRIBUTEFORMAT_UBYTE:
		{
			ubyte* val = (ubyte*)dest;
			*val++ = s / 255.0f;
			*val++ = t / 255.0f;
			break;
		}
	}
}

void CMeshBuilder::TexCoord2fv(const float *v)
{
	TexCoord2f(v[0],v[1]);
}

void CMeshBuilder::TexCoord3f(float s, float t, float r)
{
	GotoNextVertex();

	if(!m_begun || m_fmtTexcoord.offset < 0 || m_fmtTexcoord.count < 3)
		return;

	ubyte* dest = (ubyte*)m_curVertex + m_fmtTexcoord.offset;

	switch(m_fmtTexcoord.format)
	{
		case ATTRIBUTEFORMAT_FLOAT:
		{
			float* val = (float*)dest;
			*val++ = s;
			*val++ = t;
			*val++ = r;
			break;
		}
		case ATTRIBUTEFORMAT_HALF:
		{
			half* val = (half*)dest;
			*val++ = s;
			*val++ = t;
			*val++ = r;
			break;
		}
		case ATTRIBUTEFORMAT_UBYTE:
		{
			ubyte* val = (ubyte*)dest;
			*val++ = s / 255.0f;
			*val++ = t / 255.0f;
			*val++ = r / 255.0f;
			break;
		}
	}
}

void CMeshBuilder::TexCoord3fv(const float *v)
{
	TexCoord3f(v[0], v[1], v[2]);
}

// color setup
void CMeshBuilder::Color3f( float r, float g, float b )
{
	GotoNextVertex();

	if(!m_begun || m_fmtColor.offset < 0 || m_fmtColor.count < 3)
		return;

	ubyte* dest = (ubyte*)m_curVertex + m_fmtColor.offset;

	switch(m_fmtColor.format)
	{
		case ATTRIBUTEFORMAT_FLOAT:
		{
			float* val = (float*)dest;
			*val++ = r;
			*val++ = g;
			*val++ = b;
			break;
		}
		case ATTRIBUTEFORMAT_HALF:
		{
			half* val = (half*)dest;
			*val++ = r;
			*val++ = g;
			*val++ = b;
			break;
		}
		case ATTRIBUTEFORMAT_UBYTE:
		{
			ubyte* val = (ubyte*)dest;
			*val++ = r / 255.0f;
			*val++ = g / 255.0f;
			*val++ = b / 255.0f;
			break;
		}
	}
}

void CMeshBuilder::Color3fv( float const *rgb )
{
	Color3f(rgb[0], rgb[1], rgb[2]);
}

void CMeshBuilder::Color4f( float r, float g, float b, float a )
{
	GotoNextVertex();

	if(!m_begun || m_fmtColor.offset < 0 || m_fmtColor.count < 4)
		return;

	ubyte* dest = (ubyte*)m_curVertex + m_fmtColor.offset;

	switch(m_fmtColor.format)
	{
		case ATTRIBUTEFORMAT_FLOAT:
		{
			float* val = (float*)dest;
			*val++ = r;
			*val++ = g;
			*val++ = b;
			*val++ = a;
			break;
		}
		case ATTRIBUTEFORMAT_HALF:
		{
			half* val = (half*)dest;
			*val++ = r;
			*val++ = g;
			*val++ = b;
			*val++ = a;
			break;
		}
		case ATTRIBUTEFORMAT_UBYTE:
		{
			ubyte* val = (ubyte*)dest;
			*val++ = r / 255.0f;
			*val++ = g / 255.0f;
			*val++ = b / 255.0f;
			*val++ = a / 255.0f;
			break;
		}
	}
}

void CMeshBuilder::Color4fv( float const *rgba )
{
	Color4f(rgba[0], rgba[1], rgba[2], rgba[3]);
}

void CMeshBuilder::GotoNextVertex()
{
	if(!m_gotoNext)
		return;

	m_gotoNext = false;

	if(m_mesh->AllocateGeom(1, 0, &m_curVertex, NULL, false) == -1)
	{
		MsgError("AdvanceVertex failed\n");
		return;
	}
}

// advances vertex
void CMeshBuilder::AdvanceVertex()
{
	if(!m_begun)
		return;

	m_gotoNext = true;
}