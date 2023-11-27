//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: dynamic mesh interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct RenderDrawCmd;
struct VertexLayoutDesc;
class IGPUCommandBuffer;
using IGPUCommandBufferPtr = CRefPtr<IGPUCommandBuffer>;
enum EPrimTopology : int;

//
// The dynamic mesh interface
//
class IDynamicMesh
{
public:
	virtual ~IDynamicMesh() {}

	// returns a pointer to vertex format description
	virtual ArrayCRef<VertexLayoutDesc>	GetVertexLayoutDesc() const = 0;

	// sets the primitive type (chooses the way how to allocate geometry parts)
	virtual void					SetPrimitiveType( EPrimTopology primType ) = 0;
	virtual EPrimTopology			GetPrimitiveType() const = 0;

	// allocates geometry chunk. Returns the start index. Will return -1 if failed
	// addStripBreak is for PRIM_TRIANGLE_STRIP. Set it false to work with current strip
	virtual int						AllocateGeom( int nVertices, int nIndices, void** verts, uint16** indices, bool addStripBreak = true ) = 0;
	virtual void					AddStripBreak() = 0;

	virtual bool					FillDrawCmd(RenderDrawCmd& drawCmd, int firstIndex = 0, int numIndices = -1) = 0;

	// get vertex & index data submit command buffer
	virtual IGPUCommandBufferPtr	GetSubmitBuffer() = 0;

	// resets the buffer offsets
	virtual void					Reset() = 0;
};

template<typename T>
static T NextBufferOffset(T writeSize, T& offset, T bufferSize, int alignment = 4)
{
	ASSERT(writeSize < bufferSize);
	if (offset + writeSize > bufferSize)
	{ 
		ASSERT_FAIL("Exceeded buffer size %d, needed %d", bufferSize, offset + writeSize);
	}

	offset = (offset + (alignment - 1)) & ~(alignment - 1);

	const T writeOffset = offset;
	offset += (writeSize + (alignment - 1)) & ~(alignment - 1);
	return writeOffset;
}