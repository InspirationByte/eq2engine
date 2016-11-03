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

#endif // MESHBUILDER_H