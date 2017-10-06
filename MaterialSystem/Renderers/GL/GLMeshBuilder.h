//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Mesh builder
//////////////////////////////////////////////////////////////////////////////////

#ifndef GLMESHBUILDER_H
#define GLMESHBUILDER_H

#include "IMeshBuilder.h"
#include "IShaderAPI.h"

struct ListVertex_t
{
	PPMEM_MANAGED_OBJECT();

	TVec3D<half> pos;
	TVec2D<half> texCoord;
	TVec3D<half> normal;
	TVec4D<half> color;
};

class CGLMeshBuilder : public IMeshBuilder
{
public:
	CGLMeshBuilder();
	~CGLMeshBuilder();

	void		Begin(ER_PrimitiveType type);
	void		End();

	// color setting
	void		Color3f( float r, float g, float b );
	void		Color3fv( float const *rgb );
	void		Color4f( float r, float g, float b, float a );
	void		Color4fv( float const *rgba );

	// position setting
	void		Position3f	(float x, float y, float z);
	void		Position3fv	(const float *v);

	void		Normal3f	(float nx, float ny, float nz);
	void		Normal3fv	(const float *v);

	void		TexCoord2f	(float s, float t);
	void		TexCoord2fv	(const float *v);
	void		TexCoord3f	(float s, float t, float r);
	void		TexCoord3fv	(const float *v);

	void		AdvanceVertex();	// advances vertexx

protected:
	bool					m_renderBegun;

	ER_PrimitiveType			m_primType;

	DkList<ListVertex_t>	m_vertList;

	ListVertex_t			m_curVertex;

	IVertexBuffer*			m_vertexBuffer;
	IIndexBuffer*			m_indexBuffer;
	IVertexFormat*			m_vertexFormat;
};

#endif // GLMESHBUILDER_H