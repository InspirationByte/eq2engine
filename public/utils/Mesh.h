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
			: startVertex(0), startIndex(0), numVertices(0), numIndices(0), materialIndex(-1), flags(0)
		{}

		INDEX_TYPE	startVertex;
		INDEX_TYPE	startIndex;
		INDEX_TYPE	numVertices;
		INDEX_TYPE	numIndices;

		short		materialIndex;
		ushort		flags;
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
		hdr.numBatches = m_batches.numElem();
		hdr.numVertices = m_vertices.numElem();
		hdr.numIndices = m_indices.numElem();

		stream->Write(&hdr, 1, sizeof(hdr));

		// write batches
		stream->Write(m_batches.ptr(), m_batches.numElem(), sizeof(Batch));

		// write vertices
		for (int i = 0; i < m_vertices.numElem(); i++)
		{
			STORE_VERTEX_TYPE vtx = convertVertexFunc(m_vertices[i]);
			stream->Write(&vtx, 1, sizeof(STORE_VERTEX_TYPE));
		}

		// write indices
		stream->Write(m_indices.ptr(), m_indices.numElem(), sizeof(INDEX_TYPE));
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
		m_batches.setNum(hdr.numBatches);
		m_vertices.setNum(hdr.numVertices);
		m_indices.setNum(hdr.numIndices);

		// read batches
		stream->Read(m_batches.ptr(), hdr.numBatches, sizeof(Batch));

		// read vertices
		for (int i = 0; i < hdr.numVertices; i++)
		{
			STORE_VERTEX_TYPE vert;
			stream->Read(&vert, 1, sizeof(STORE_VERTEX_TYPE));

			m_vertices[i] = convertVertexFunc(vert);
		}

		// read indices
		stream->Read(m_indices.ptr(), hdr.numIndices, sizeof(INDEX_TYPE));
	}

	DkList<Batch>			m_batches;
	DkList<VERTEX_TYPE>		m_vertices;
	DkList<INDEX_TYPE>		m_indices;
};

#endif // MESH_H