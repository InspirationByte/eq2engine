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
	m_begun(false)
{
	m_mesh = mesh;

	// get the format offsets
	int numAttribs;
	m_mesh->GetVertexFormatDesc(&m_formatDesc, numAttribs);

	int vertexSize = 0;
	for(int i = 0; i < numAttribs; i++)
	{
		AttributeFormat_e format = m_formatDesc[i].m_nFormat;
		VertexType_e type = m_formatDesc[i].m_nType;
		int vecCount = m_formatDesc[i].m_nSize;
		int attribSize = vecCount * attributeFormatSize[format];

		if(type == VERTEXTYPE_VERTEX)
		{
			m_position.offset = vertexSize;
			m_position.count = vecCount;
			m_position.format = format;
		}
		else if(type == VERTEXTYPE_NORMAL)
		{
			m_normal.offset = vertexSize;
			m_normal.count = vecCount;
			m_normal.format = format;
		}
		else if(type == VERTEXTYPE_TEXCOORD)
		{
			m_texcoord.offset = vertexSize;
			m_texcoord.count = vecCount;
			m_texcoord.format = format;
		}
		else if(type == VERTEXTYPE_COLOR)
		{
			m_color.offset = vertexSize;
			m_color.count = vecCount;
			m_color.format = format;
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

	m_position.value = Vector4D(0,0,0,1.0f);
	m_texcoord.value = vec4_zero;
	m_normal.value = Vector4D(0,1,0,0);
	m_color.value = color4_white;

	m_begun = true;
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
	m_position.value.x = x;
	m_position.value.y = y;
	m_position.value.z = z;
	m_position.value.w = 1.0f;
}

void CMeshBuilder::Position3fv(const float *v)
{
	Position3f(v[0], v[1], v[2]);
}

// normal setup
void CMeshBuilder::Normal3f(float nx, float ny, float nz)
{
	m_normal.value.x = nx;
	m_normal.value.y = ny;
	m_normal.value.z = nz;
	m_normal.value.w = 0.0f;
}

void CMeshBuilder::Normal3fv(const float *v)
{
	Normal3f(v[0], v[1], v[2]);
}

void CMeshBuilder::TexCoord2f(float s, float t)
{
	m_texcoord.value.x = s;
	m_texcoord.value.y = t;
	m_texcoord.value.z = 0.0f;
	m_texcoord.value.w = 0.0f;
}

void CMeshBuilder::TexCoord2fv(const float *v)
{
	TexCoord2f(v[0],v[1]);
}

void CMeshBuilder::TexCoord3f(float s, float t, float r)
{
	m_texcoord.value.x = s;
	m_texcoord.value.y = t;
	m_texcoord.value.z = r;
	m_texcoord.value.w = 0.0f;
}

void CMeshBuilder::TexCoord3fv(const float *v)
{
	TexCoord3f(v[0], v[1], v[2]);
}

// color setup
void CMeshBuilder::Color3f( float r, float g, float b )
{
	m_color.value.x = r;
	m_color.value.y = g;
	m_color.value.z = b;
	m_color.value.w = 1.0f;
}

void CMeshBuilder::Color3fv( float const *rgb )
{
	Color3f(rgb[0], rgb[1], rgb[2]);
}

void CMeshBuilder::Color4f( float r, float g, float b, float a )
{
	GotoNextVertex();

	m_color.value.x = r;
	m_color.value.y = g;
	m_color.value.z = b;
	m_color.value.w = a;
}

void CMeshBuilder::Color4fv( float const *rgba )
{
	Color4f(rgba[0], rgba[1], rgba[2], rgba[3]);
}

void CMeshBuilder::GotoNextVertex()
{

}

// advances vertex
void CMeshBuilder::AdvanceVertex()
{
	if(!m_begun)
		return;

	if(m_mesh->AllocateGeom(1, 0, &m_curVertex, NULL, false) == -1)
	{
		MsgError("AdvanceVertex failed\n");
		return;
	}

	CopyVertData(m_position);
	CopyVertData(m_texcoord);
	CopyVertData(m_normal);
	CopyVertData(m_color);
}

void CMeshBuilder::CopyVertData(vertdata_t& vert)
{
	if(!vert.count)
		return;

	ubyte* dest = (ubyte*)m_curVertex + vert.offset;

	int size = min(vert.count, 4) * attributeFormatSize[vert.format];

	switch(vert.format)
	{
		case ATTRIBUTEFORMAT_FLOAT:
		{
			memcpy(dest, &vert.value, size);
			break;
		}
		case ATTRIBUTEFORMAT_HALF:
		{
			TVec4D<half> val(vert.value);
			memcpy(dest, &val, size);
			break;
		}
		case ATTRIBUTEFORMAT_UBYTE:
		{
			TVec4D<ubyte> val(vert.value / 255.0f);
			memcpy(dest, &val, size);
			break;
		}
	}
}
