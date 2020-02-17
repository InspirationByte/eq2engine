//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: The MTriangle adjacency generator
//////////////////////////////////////////////////////////////////////////////////

#ifndef MTRIANGLE_FRAMEWORK_H
#define MTRIANGLE_FRAMEWORK_H

#include "utils/GeomTools.h"

namespace MTriangle
{

// edge
struct medge_t
{
	medge_t()
	{
		idx[0] = -1;
		idx[1] = -1;
	}

	medge_t(int i0, int i1)
	{
		idx[0] = i0;
		idx[1] = i1;
	}

	int idx[2];

	void Swap()
	{
		int tmp = idx[0];
		idx[0] = idx[1];
		idx[1] = tmp;
	}

	bool CompareWith( const medge_t& other ) const
	{
		return (idx[0] == other.idx[0] && idx[1] == other.idx[1]);
	}

	bool IsConnected(const medge_t& other, bool &swap) const
	{
		swap = true;

		bool connected = ((idx[0] == other.idx[0]) || (idx[1] == other.idx[1]));
		if(connected)
			return true;

		swap = false;

		medge_t swapped_edge = other;
		swapped_edge.Swap();

		return ((idx[0] == swapped_edge.idx[0]) || (idx[1] == swapped_edge.idx[1]));
	}
};

inline bool operator ==(const medge_t& a,const medge_t& b)
{
	return (a.idx[0] == b.idx[0] && a.idx[1] == b.idx[1]);
}

// triangle
struct mtriangle_t
{
	mtriangle_t()
	{
		memset(edge_connections, 0, sizeof(edge_connections));
	}

	int indices[3];
	int tri_id;
	
	void BuildEdge(int i, medge_t& e)
	{
		e.idx[0] = indices[i];

		int i2 = i+1;

		if(i2 > 2)
			i2 -= 3;

		e.idx[1] = indices[i2];
	}

	bool IsOwnEdge(const medge_t& edge, int &edge_idx, bool ignoreOrder = true) const
	{
		medge_t swapped_edge = edge;
		swapped_edge.Swap();

		for(int i = 0; i < 3; i++)
		{
			if(edges[i].CompareWith(edge) || (ignoreOrder && edges[i].CompareWith(swapped_edge)) )
			{
				edge_idx = i;
				return true;
			}
		}

		edge_idx = -1;

		return false;
	}

	// quad helper
	int GetOppositeVertexIndex(int edge_idx) const
	{
		ASSERT(edge_idx < 3);

		int eidx = edge_idx + 2;

		if(eidx > 2)
			eidx -= 3;

		return indices[eidx];
	}

	// promoted to self-remove, as this totally useless function
	bool HasDuplicates() const
	{
		// look around and try find same edges
		for(int i = 0; i < index_connections.numElem(); i++)
		{
			int nEdgesOwned = 0;

			for(int j = 0; j < 3; j++)
			{
				int edge_idx;

				if(IsOwnEdge( index_connections[i]->edges[j], edge_idx ))
					nEdgesOwned++;
			}

			if(nEdgesOwned >= 3)
				return true;
		}

		return false;
	}

	bool Compare(int v1, int v2, int v3) const
	{
		return	(indices[0] == v1 && indices[1] == v2 && indices[2] == v3 ) ||
				(indices[1] == v1 && indices[2] == v2 && indices[0] == v3 ) ||
				(indices[2] == v1 && indices[0] == v2 && indices[1] == v3 );
	}

	medge_t					edges[3];
	mtriangle_t*			edge_connections[3];	// adjacent triangles

	DkList<mtriangle_t*>	index_connections;
};

//-------------------------------------------------------------------------
// Triangle adjacency graph
// It has neighbours and edge lists
//-------------------------------------------------------------------------
class CAdjacentTriangleGraph
{
public:
							CAdjacentTriangleGraph() {}

							CAdjacentTriangleGraph( const DkList<int>& indices );
							CAdjacentTriangleGraph( const int* indices, int num_indices );

							CAdjacentTriangleGraph(	const ubyte* input_verts,
													int vert_stride, 
													int vert_ofs, 
													const int* indices, 
													int num_indices);

	void					Clear();

	// builds adjacency and neigbourhood data for triangles using only indices
	void					Build( const int* in_trilistidxs, int num_indices);

	// builds adjacency and neigbourhood data for triangles with vertex position comparison
	void					BuildWithVertices(	const ubyte* input_verts, 
												int vert_stride, 
												int vert_ofs, 
												const int* in_trilistidxs, 
												int num_indices);

	int						FindTriangleByEdge(	const medge_t &edge,
												int& found_edge_idx, 
												bool ignore_order) const;

	int						FindTriangle(int v1, int v2, int v3, bool ignore_order) const;

	void					GenOptimizedTriangleList( DkList<int>& output );
	void					GenOptimizedStrips( DkList<int>& output, bool usePrimRestart = false );

	DkList<mtriangle_t>*	GetTriangles() {return &m_triangleList;}

protected:

	// builds neigbourhood table for specified triangle, also using a vertex information
	void					BuildTriangleAdjacencyConnections( mtriangle_t* pTri, int tri_id );

	void					BuildTriangleAdjacencyConnectionsByVerts(	mtriangle_t* pTri, 
																		int tri_id,
																		const ubyte* input_verts, 
																		int vert_stride, 
																		int vert_ofs);

	DkList<mtriangle_t>		m_triangleList;
};

}; // namespace MTriangle

#endif // MTRIANGLE_FRAMEWORK_H