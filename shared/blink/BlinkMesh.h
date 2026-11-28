//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: BLINK mesh: triangles with material batches sent to and from Blender
//////////////////////////////////////////////////////////////////////////////////

#pragma once
struct KVSection;

// A triangle mesh in engine coordinates; the Blender side converts it (blink.coords).
// Write puts the batches in the KeyValues result and the vertex arrays in the payload:
// positions, normals, texcoords, indices, colors (each one an int count and the items,
// IFileStream::WriteObj). Read takes the same from Blender and checks it.
struct BlinkMesh
{
	struct Batch
	{
		EqString	material;
		int			firstIndex{ 0 };	// the batch ends at the next one
	};

	Array<Vector3D>		positions{ PP_SL };
	Array<Vector3D>		normals{ PP_SL };
	Array<Vector2D>		texcoords{ PP_SL };
	Array<int>			indices{ PP_SL };
	Array<TVec4D<ubyte>>	colors{ PP_SL };	// empty, or a color of each position (the application gives it a meaning)
	Array<Batch>		batches{ PP_SL };

	// starts a batch at the current index count; the indices added after it belong to it
	void				BeginBatch(const char* material);

	void				Write(KVSection& out, IFileStream& payload) const;

	// the batch sections of params and the arrays of payload; false with the error when they are not
	// a valid mesh (array sizes, index range, batches covering the indices in order)
	bool				Read(const KVSection& params, IFileStream& payload, EqString& error);
};
