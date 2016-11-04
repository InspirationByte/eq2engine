//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: a mesh builder designed for dynamic meshes (material system)
//////////////////////////////////////////////////////////////////////////////////

#ifndef MESHBUILDER_H
#define MESHBUILDER_H

#include "IDynamicMesh.h"

class CMeshBuilder
{
public:
	CMeshBuilder(IDynamicMesh* mesh);
	~CMeshBuilder();

	// begins the mesh
	void		Begin(PrimitiveType_e type);

	// ends building and renders the mesh
	void		End();

	// position setup
	void		Position3f(float x, float y, float z);
	void		Position3fv(const float *v);

	// normal setup
	void		Normal3f(float nx, float ny, float nz);
	void		Normal3fv(const float *v);

	void		TexCoord2f(float s, float t);
	void		TexCoord2fv(const float *v);
	void		TexCoord3f(float s, float t, float r);
	void		TexCoord3fv(const float *v);

	// color setup
	void		Color3f( float r, float g, float b );
	void		Color3fv( float const *rgb );
	void		Color4f( float r, float g, float b, float a );
	void		Color4fv( float const *rgba );

	// advances vertex
	void		AdvanceVertex();

protected:

	void		GotoNextVertex();
	
	IDynamicMesh*		m_mesh;
	VertexFormatDesc_t* m_formatDesc;

	void*				m_curVertex;

	struct vertdata_t
	{
		vertdata_t(){
			offset = -1;
			count = 0;
		}

		int8				offset;
		int8				count;
		AttributeFormat_e	format;
		Vector4D			value;
	};

	void				CopyVertData(vertdata_t& vert);

	vertdata_t			m_position;
	vertdata_t			m_normal;
	vertdata_t			m_texcoord;
	vertdata_t			m_color;

	bool				m_begun;
};

//----------------------------------------------------------

inline CMeshBuilder::CMeshBuilder(IDynamicMesh* mesh) :
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

inline CMeshBuilder::~CMeshBuilder()
{

}

// begins the mesh
inline void CMeshBuilder::Begin(PrimitiveType_e type)
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
inline void CMeshBuilder::End()
{
	m_mesh->Render();
	m_begun = false;
}

// position setup
inline void CMeshBuilder::Position3f(float x, float y, float z)
{
	m_position.value.x = x;
	m_position.value.y = y;
	m_position.value.z = z;
	m_position.value.w = 1.0f;
}

inline void CMeshBuilder::Position3fv(const float *v)
{
	Position3f(v[0], v[1], v[2]);
}

// normal setup
inline void CMeshBuilder::Normal3f(float nx, float ny, float nz)
{
	m_normal.value.x = nx;
	m_normal.value.y = ny;
	m_normal.value.z = nz;
	m_normal.value.w = 0.0f;
}

inline void CMeshBuilder::Normal3fv(const float *v)
{
	Normal3f(v[0], v[1], v[2]);
}

inline void CMeshBuilder::TexCoord2f(float s, float t)
{
	m_texcoord.value.x = s;
	m_texcoord.value.y = t;
	m_texcoord.value.z = 0.0f;
	m_texcoord.value.w = 0.0f;
}

inline void CMeshBuilder::TexCoord2fv(const float *v)
{
	TexCoord2f(v[0],v[1]);
}

inline void CMeshBuilder::TexCoord3f(float s, float t, float r)
{
	m_texcoord.value.x = s;
	m_texcoord.value.y = t;
	m_texcoord.value.z = r;
	m_texcoord.value.w = 0.0f;
}

inline void CMeshBuilder::TexCoord3fv(const float *v)
{
	TexCoord3f(v[0], v[1], v[2]);
}

// color setup
inline void CMeshBuilder::Color3f( float r, float g, float b )
{
	m_color.value.x = r;
	m_color.value.y = g;
	m_color.value.z = b;
	m_color.value.w = 1.0f;
}

inline void CMeshBuilder::Color3fv( float const *rgb )
{
	Color3f(rgb[0], rgb[1], rgb[2]);
}

inline void CMeshBuilder::Color4f( float r, float g, float b, float a )
{
	GotoNextVertex();

	m_color.value.x = r;
	m_color.value.y = g;
	m_color.value.z = b;
	m_color.value.w = a;
}

inline void CMeshBuilder::Color4fv( float const *rgba )
{
	Color4f(rgba[0], rgba[1], rgba[2], rgba[3]);
}

inline void CMeshBuilder::GotoNextVertex()
{

}

// advances vertex
inline void CMeshBuilder::AdvanceVertex()
{
	if(!m_begun)
		return;

	if(m_mesh->AllocateGeom(1, 0, &m_curVertex, NULL, false) == -1)
		return;

	CopyVertData(m_position);
	CopyVertData(m_texcoord);
	CopyVertData(m_normal);
	CopyVertData(m_color);
}

inline void CMeshBuilder::CopyVertData(vertdata_t& vert)
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

#endif // MESHBUILDER_H