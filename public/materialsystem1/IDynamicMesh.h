//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: dynamic mesh interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

//
// The dynamic mesh interface
//
class IDynamicMesh
{
public:
	virtual ~IDynamicMesh() {}

	// sets the primitive type (chooses the way how to allocate geometry parts)
	virtual void				SetPrimitiveType( ER_PrimitiveType primType ) = 0;
	virtual ER_PrimitiveType	GetPrimitiveType() const = 0;

	// returns a pointer to vertex format description
	virtual void			GetVertexFormatDesc(const VertexFormatDesc_t** desc, int& numAttribs) = 0;

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
	virtual void			Render( int firstIndex, int numIndices ) = 0;

	// resets the dynamic mesh
	virtual void			Reset() = 0;
};