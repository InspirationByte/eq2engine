//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: dynamic mesh interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef IDYNAMICMESH_H
#define IDYNAMICMESH_H

#include "renderers/ShaderAPI_defs.h"

// standard vertex format used by the material system's dynamic mesh instance
static VertexFormatDesc_t g_standardVertexFormatDesc[] = {
	{0, 4, VERTEXATTRIB_POSITION,	ATTRIBUTEFORMAT_FLOAT, "position"},
	{0, 4, VERTEXATTRIB_TEXCOORD,	ATTRIBUTEFORMAT_HALF, "texcoord"},
	{0, 4, VERTEXATTRIB_NORMAL,		ATTRIBUTEFORMAT_HALF, "normal"},
	{0, 4, VERTEXATTRIB_COLOR,		ATTRIBUTEFORMAT_HALF, "color"},
};

//
// The dynamic mesh interface
//
class IDynamicMesh
{
public:
	virtual ~IDynamicMesh() {}

	// sets the primitive type (chooses the way how to allocate geometry parts)
	virtual void			SetPrimitiveType( ER_PrimitiveType primType ) = 0;
	virtual ER_PrimitiveType	GetPrimitiveType() const = 0;

	// returns a pointer to vertex format description
	virtual void			GetVertexFormatDesc(VertexFormatDesc_t** desc, int& numAttribs) = 0;

	// allocates geometry chunk. Returns the start index. Will return -1 if failed
	// addStripBreak is for PRIM_TRIANGLE_STRIP. Set it false to work with current strip
	//
	// note that if you use materials->GetDynamicMesh() you should pass the StdDynMeshVertex_t vertices
	//
	// FIXME: subdivide on streams???
	virtual int				AllocateGeom( int nVertices, int nIndices, void** verts, uint16** indices, bool addStripBreak = true ) = 0;
	virtual void			AddStripBreak() = 0;

	// uploads buffers and renders the mesh. Note that you has been set material and adjusted RTs
	virtual void			Render() = 0;

	// resets the dynamic mesh
	virtual void			Reset() = 0;
};

#endif // IDYNAMICMESH_H