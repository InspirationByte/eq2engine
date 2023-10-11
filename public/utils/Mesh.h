//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EqEngine mutex storage
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// generic indexed mesh. File-friendly
template <typename VERTEX_TYPE, typename INDEX_TYPE>
struct BatchedIndexedMesh
{
public:
	virtual ~BatchedIndexedMesh() = default;

	struct Batch
	{
		INDEX_TYPE	startVertex{ 0 };
		INDEX_TYPE	startIndex{ 0 };
		INDEX_TYPE	numVertices{ 0 };
		INDEX_TYPE	numIndices{ 0 };

		short		materialIndex{ -1 };
		ushort		flags{ 0 };
	};

	struct FileHeader
	{
		int			numBatches{ 0 };
		INDEX_TYPE	numVertices{ 0 };
		INDEX_TYPE	numIndices{ 0 };
	};

	Array<Batch>		batches{ PP_SL };
	Array<VERTEX_TYPE>	vertices{ PP_SL };
	Array<INDEX_TYPE>	indices{ PP_SL };

	// creates new batch in this instance
	// also will fill start vertex and start index
	Batch& New_Batch() 
	{
		Batch& newBatch = batches.append();

		newBatch.startVertex = vertices.numElem();
		newBatch.startIndex = indices.numElem();
		return newBatch;
	}

	Batch& FindOrAdd_Batch(short materialIndex)
	{
		for (int i = 0; i < batches.numElem(); ++i)
		{
			if (batches[i].materialIndex == materialIndex) 
				return batches[i];
		}

		Batch& newBatch = New_Batch();
		newBatch.materialIndex = materialIndex;
		return newBatch;
	}

	// deletes all data
	void Clear()
	{
		batches.clear();
		vertices.clear();
		indices.clear();
	}

	template <typename DEST_VERTEX, typename SOURCE_VERTEX>
	using VertexConvertFunc = DEST_VERTEX(*)(const SOURCE_VERTEX& from);

	static VERTEX_TYPE DefaultConvertVertexFunc(const VERTEX_TYPE& from)
	{
		return from;
	}

	// serializes data
	template <class STORE_VERTEX_TYPE = VERTEX_TYPE>
	void Save(
		IVirtualStream* stream, 
		VertexConvertFunc<STORE_VERTEX_TYPE, VERTEX_TYPE> convertVertexFunc = DefaultConvertVertexFunc) const
	{
		// write header
		FileHeader hdr;
		hdr.numBatches = batches.numElem();
		hdr.numVertices = vertices.numElem();
		hdr.numIndices = indices.numElem();

		stream->Write(&hdr, 1, sizeof(hdr));

		// write batches
		stream->Write(batches.ptr(), batches.numElem(), sizeof(Batch));

		// write vertices
		for (int i = 0; i < vertices.numElem(); i++)
		{
			STORE_VERTEX_TYPE vtx = convertVertexFunc(vertices[i]);
			stream->Write(&vtx, 1, sizeof(STORE_VERTEX_TYPE));
		}

		// write indices
		stream->Write(indices.ptr(), indices.numElem(), sizeof(INDEX_TYPE));
	}

	// loads all data
	template <class STORE_VERTEX_TYPE = VERTEX_TYPE>
	void Load(IVirtualStream* stream,
		VertexConvertFunc<VERTEX_TYPE, STORE_VERTEX_TYPE> convertVertexFunc = DefaultConvertVertexFunc)
	{
		// read header
		FileHeader hdr;
		stream->Read(&hdr, 1, sizeof(hdr));

		// resize arrays
		batches.setNum(hdr.numBatches);
		vertices.setNum(hdr.numVertices);
		indices.setNum(hdr.numIndices);

		// read batches
		stream->Read(batches.ptr(), hdr.numBatches, sizeof(Batch));

		// read vertices
		for (int i = 0; i < hdr.numVertices; i++)
		{
			STORE_VERTEX_TYPE vert;
			stream->Read(&vert, 1, sizeof(STORE_VERTEX_TYPE));

			vertices[i] = convertVertexFunc(vert);
		}

		// read indices
		stream->Read(indices.ptr(), hdr.numIndices, sizeof(INDEX_TYPE));
	}
};