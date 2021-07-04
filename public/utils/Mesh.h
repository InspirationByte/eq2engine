//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EqEngine mutex storage
//////////////////////////////////////////////////////////////////////////////////

#ifndef MESH_H
#define MESH_H

#include "DkList.h"
#include "IVirtualStream.h"

// generic indexed mesh. File-friendly
template <typename VERTEX_TYPE, typename INDEX_TYPE>
class CBatchedIndexedMesh
{
public:
	CBatchedIndexedMesh()
	{}

	virtual ~CBatchedIndexedMesh()
	{}

	struct Batch
	{
		Batch() 
			: startVertex(0), startIndex(0), numVertices(0), numIndices(0), materialIndex(-1)
		{}

		INDEX_TYPE	startVertex;
		INDEX_TYPE	startIndex;
		INDEX_TYPE	numVertices;
		INDEX_TYPE	numIndices;
		int			materialIndex;
	};

	struct FileHeader
	{
		int			numBatches;
		INDEX_TYPE	numVertices;
		INDEX_TYPE	numIndices;
	};

	// creates new batch in this instance
	// also will fill start vertex and start index
	Batch& New_Batch() 
	{
		Batch newBatch;
		newBatch.startVertex = m_vertices.numElem();
		newBatch.startIndex = m_indices.numElem();

		int idx = m_batches.append(newBatch);
		return m_batches[idx];
	}

	// deletes all data
	void Clear()
	{
		m_batches.clear();
		m_vertices.clear();
		m_indices.clear();
	}

	// serializes data
	void Save(IVirtualStream* stream)
	{
		// write header
		FileHeader hdr;
		hdr.numBatches = m_batches.numElem();
		hdr.numVertices = m_vertices.numElem();
		hdr.numIndices = m_indices.numElem();

		stream->Write(&hdr, 1, sizeof(hdr));

		// write batches
		stream->Write(m_batches.ptr(), m_batches.numElem(), sizeof(Batch));

		// write vertices
		stream->Write(m_vertices.ptr(), m_vertices.numElem(), sizeof(VERTEX_TYPE));

		// write indices
		stream->Write(m_indices.ptr(), m_indices.numElem(), sizeof(INDEX_TYPE));
	}

	// loads all data
	void Load(IVirtualStream* stream)
	{
		// read header
		FileHeader hdr;
		stream->Read(&hdr, 1, sizeof(hdr));

		// resize arrays
		m_batches.setNum(hdr.numBatches);
		m_vertices.setNum(hdr.numVertices);
		m_indices.setNum(hdr.numIndices);

		// read batches
		stream->Read(m_batches.ptr(), hdr.numBatches, sizeof(Batch));

		// read vertices
		stream->Read(m_vertices.ptr(), hdr.numVertices, sizeof(VERTEX_TYPE));

		// read indices
		stream->Read(m_indices.ptr(), hdr.numIndices, sizeof(INDEX_TYPE));
	}

	DkList<Batch>			m_batches;
	DkList<VERTEX_TYPE>		m_vertices;
	DkList<INDEX_TYPE>		m_indices;
};

#endif // MESH_H