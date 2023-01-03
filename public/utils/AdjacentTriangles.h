//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: The triangle adjacency generator
//////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace AdjacentTriangles
{

// edge
struct Edge
{
	int		idx[2]{ -1,-1 };

	Edge	Swap() const;
	bool	IsConnected(const Edge& other, bool& swap) const;

	friend bool operator==(const Edge& a, const Edge& b)
	{
		return a.idx[0] == b.idx[0] && a.idx[1] == b.idx[1];
	}
};

// triangle
struct Triangle
{
	Array<Triangle*>	vertexCon{ PP_SL };
	Edge				edges[3];
	Triangle*			edgeCon[3]{ nullptr };
	int					indices[3];


	bool				GetValidEdge(const Edge& edge, int& edge_idx, bool ignoreOrder = true) const;
	int					GetOppositeVertexIndex(int edge_idx) const;
	bool				Compare(int v1, int v2, int v3) const;

	void				InitEdge(int i, Edge& e);
};

//-------------------------------------------------------------------------
// Triangle adjacency graph
// It has neighbours and edge lists
//-------------------------------------------------------------------------
class CAdjacentTriangleGraph
{
public:
							CAdjacentTriangleGraph() {}

							CAdjacentTriangleGraph( const Array<int>& indices );
							CAdjacentTriangleGraph( const int* indices, int num_indices );
							CAdjacentTriangleGraph(	const ubyte* input_verts, int vert_stride, int vert_ofs, const int* indices, int num_indices);

	void					Clear();

	// builds adjacency and neigbourhood data for triangles using only indices
	void					Build( const int* in_trilistidxs, int num_indices);

	// builds adjacency and neigbourhood data for triangles with vertex position comparison
	void					BuildWithVertices(	const ubyte* input_verts, 
												int vert_stride, 
												int vert_ofs, 
												const int* in_trilistidxs, 
												int num_indices);

	int						FindTriangleByEdge(	const Edge& edge,
												int& found_edge_idx, 
												bool ignore_order) const;

	int						FindTriangle(int v1, int v2, int v3, bool ignore_order) const;

	void					GenOptimizedTriangleList( Array<int>& output );
	void					GenOptimizedStrips( Array<int>& output, bool usePrimRestart = false );

	const Array<Triangle>&	GetTriangles() {return m_triangleList;}

protected:

	// builds neigbourhood table for specified triangle, also using a vertex information
	void					BuildConnections( Triangle& forTri, int forTriIdx);
	void					BuildConnectionsUsingVerts(Triangle& forTri, int forTriIdx, const ubyte* input_verts, int vert_stride, int vert_ofs);

	Array<Triangle>			m_triangleList{ PP_SL };
};

}; // namespace AdjacentTriangles
