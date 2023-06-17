//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: The triangle adjacency generator
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "AdjacentTriangles.h"

namespace AdjacentTriangles
{

Edge Edge::Swap() const
{
	return Edge{ idx[1], idx[0] };
}

bool Edge::IsConnected(const Edge& other, bool &swap) const
{
	swap = false;

	const bool connected = ((idx[0] == other.idx[0]) || (idx[1] == other.idx[1]));
	if (connected)
	{
		swap = true;
		return true;
	}

	const Edge swapped_edge = other.Swap();
	return ((idx[0] == swapped_edge.idx[0]) || (idx[1] == swapped_edge.idx[1]));
}

void Triangle::InitEdge(int i, Edge& e)
{
	e.idx[0] = indices[i];
	e.idx[1] = indices[(i + 1) % 3];
}

bool Triangle::GetValidEdge(const Edge& edge, int &edge_idx, bool ignoreOrder) const
{
	edge_idx = -1;
	for(int i = 0; i < 3; i++)
	{
		if(edges[i] == edge || (ignoreOrder && edges[i] == edge.Swap()) )
		{
			edge_idx = i;
			return true;
		}
	}
	return false;
}

// quad helper
int Triangle::GetOppositeVertexIndex(int edge_idx) const
{
	return indices[(edge_idx + 2) % 3];
}

bool Triangle::Compare(int v1, int v2, int v3) const
{
	return	(indices[0] == v1 && indices[1] == v2 && indices[2] == v3 ) ||
			(indices[1] == v1 && indices[2] == v2 && indices[0] == v3 ) ||
			(indices[2] == v1 && indices[0] == v2 && indices[1] == v3 );
}

//--------------------------------------------

CAdjacentTriangleGraph::CAdjacentTriangleGraph(const Array<int>& indices)
{
	Build(indices.ptr(), indices.numElem());
}

CAdjacentTriangleGraph::CAdjacentTriangleGraph(const int* indices, int num_indices)
{
	Build(indices, num_indices);
}

CAdjacentTriangleGraph::CAdjacentTriangleGraph(	const ubyte* input_verts,
												int vert_stride, 
												int vert_ofs, 
												const int* indices, 
												int num_indices)
{
	BuildWithVertices(input_verts, vert_stride, vert_ofs, indices, num_indices);
}

void CAdjacentTriangleGraph::Clear()
{
	m_triangleList.clear();
}

int CAdjacentTriangleGraph::FindTriangleByEdge(	const Edge& edge,
												int& found_edge_idx, 
												bool ignore_order) const
{
	for(int i = 0; i < m_triangleList.numElem(); i++)
	{
		if(m_triangleList[i].GetValidEdge(edge, found_edge_idx, ignore_order))
			return i;
	}

	return -1;
}

int CAdjacentTriangleGraph::FindTriangle(int v1, int v2, int v3, bool ignore_order) const
{
	for(int i = 0; i < m_triangleList.numElem(); i++)
	{
		if(m_triangleList[i].Compare(v1,v2,v3) || (ignore_order && m_triangleList[i].Compare(v3,v2,v1)))
			return i;
	}

	return -1;
}

//----------------------------------------------------------------------------------------------

// adds triangle to index island recursively
static void FloodFillIsland(Island& destIsland, const Triangle& startTriangle, Set<const Triangle*>& visited)
{
	Array<const Triangle*> openSet(PP_SL);
	openSet.append(&startTriangle);

	while (openSet.numElem())
	{
		const Triangle* tri = openSet.popBack();
		destIsland.append({ tri->indices[0], tri->indices[1], tri->indices[2] });

		for (Triangle* con : tri->vertexCon)
		{
			if (!visited.find(con).atEnd())
				continue;
			openSet.append(con);
			visited.insert(con);
		}
	}
}

void CAdjacentTriangleGraph::GetIslands(Array<Island>& islands) const
{
	// sort triangles to new islands
	Set<const Triangle*> visited(PP_SL);

	for (int i = 0; i < m_triangleList.numElem(); i++)
	{
		const Triangle& tri = m_triangleList[i];

		if (!visited.find(&tri).atEnd())
			continue;

		Island& island = islands.append();
		FloodFillIsland(island, tri, visited);
	}
}

void CAdjacentTriangleGraph::GenOptimizedTriangleList( Array<int>& output ) const
{
	// sort triangles to new islands
	Array<Island> islands(PP_SL);
	GetIslands(islands);

	// next we need to join islands
	for(int i = 0; i < islands.numElem(); i++)
	{
		const Island& island = islands[i];
		output.resize(output.numElem() + island.numElem()*3);

		for(const ITriangle& tri : island)
		{
			output.append(tri[0]);
			output.append(tri[1]);
			output.append(tri[2]);
		}
	}
}

void CAdjacentTriangleGraph::GenOptimizedStrips( Array<int>& output, bool usePrimRestart ) const
{
	ASSERT_FAIL("CAdjacentTriangleGraph::GenOptimizedStrips not implemented");
}

// builds neigbourhood table for specified triangle, also using a vertex information
void CAdjacentTriangleGraph::BuildConnections( Triangle& forTri, int forTriIdx )
{
	for(int i = 0; i < m_triangleList.numElem(); i++)
	{
		Triangle& checkTri = m_triangleList[i];

		// skip this triangle
		if(forTriIdx == i)
			continue;

		// index/vertex connections
		for(int j = 0; j < 3; j++)
		{
			bool foundCon = false;
			for(int k = 0; k < 3; k++)
			{
				// if we have same index, so we have a connection
				if(forTri.indices[j] == checkTri.indices[k])
				{
					// add it as unique
					forTri.vertexCon.addUnique(&checkTri);
					foundCon = true;
					break; // k
				}
			}

			if(foundCon)
				break; // j
		}

		// edge connections
		for(int j = 0; j < 3; j++)
		{
			int edge_idx;
			if(forTri.GetValidEdge(checkTri.edges[j], edge_idx))
				checkTri.edgeCon[j] = &forTri;
		}
	}
}

void CAdjacentTriangleGraph::BuildConnectionsUsingVerts(Triangle& forTri, int forTriIdx, const ubyte* input_verts, int vert_stride, int vert_ofs)
{
	for(int i = 0; i < m_triangleList.numElem(); i++)
	{
		Triangle& checkTri = m_triangleList[i];

		if(forTriIdx == i)
			continue;

		// index/vertex connections
		for(int j = 0; j < 3; j++)
		{
			bool foundCon = false;
			for(int k = 0; k < 3; k++)
			{
				auto vertexAtOfs = [](const ubyte* vertexBaseOffset, int stride, int vertexIndex) -> Vector3D
				{
					ubyte* pDataVertex = (ubyte*)vertexBaseOffset + vertexIndex * stride;
					return *(Vector3D*)pDataVertex;
				};

				// compare verts
				const Vector3D vertexJ = vertexAtOfs(input_verts+vert_ofs, vert_stride, forTri.indices[j]);
				const Vector3D vertexK = vertexAtOfs(input_verts+vert_ofs, vert_stride, checkTri.indices[k]);

				// if we have same index, or same position, make it as neighbour
				if(forTri.indices[j] == checkTri.indices[k] || (lengthSqr(vertexJ - vertexK) < F_EPS))
				{
					// add it as unique
					forTri.vertexCon.addUnique(&checkTri);
					foundCon = true;

					break; // k
				}
			}

			if(foundCon)
				break; // j
		}

		// edge connections
		for (int k = 0; k < 3; k++)
		{
			int edge_idx;
			if (forTri.GetValidEdge(checkTri.edges[k], edge_idx))
				checkTri.edgeCon[k] = &forTri;
		}
	}
}

// builds adjacency and neigbourhood data for triangles using only indices
void CAdjacentTriangleGraph::Build( const int* in_trilistidxs, int num_indices)
{
	m_triangleList.resize(num_indices / 3);

	// first, divide all indices on triangles
	for(int tri_id = 0, i = 0; i < num_indices; i+=3, ++tri_id)
	{
		Triangle& newTri = m_triangleList.append();

		newTri.indices[0] = in_trilistidxs[i];
		newTri.indices[1] = in_trilistidxs[i+1];
		newTri.indices[2] = in_trilistidxs[i+2];

		newTri.InitEdge(0, newTri.edges[0]);
		newTri.InitEdge(1, newTri.edges[1]);
		newTri.InitEdge(2, newTri.edges[2]);
	}

	// next build neighbour table for each triangle
	for(int i = 0; i < m_triangleList.numElem(); i++)
		BuildConnections( m_triangleList[i], i );

	// all done
}

// builds adjacency and neigbourhood data for triangles with vertex position comparison
void CAdjacentTriangleGraph::BuildWithVertices(	const ubyte* input_verts, int vert_stride, int vert_ofs, const int* in_trilistidxs, int num_indices)
{
	m_triangleList.resize(num_indices / 3);

	// first, divide all indices on triangles
	for(int i = 0; i < num_indices; i+=3)
	{
		Triangle& newTri = m_triangleList.append();

		newTri.indices[0] = in_trilistidxs[i];
		newTri.indices[1] = in_trilistidxs[i+1];
		newTri.indices[2] = in_trilistidxs[i+2];
		newTri.InitEdge(0, newTri.edges[0]);
		newTri.InitEdge(1, newTri.edges[1]);
		newTri.InitEdge(2, newTri.edges[2]);
	}

	// next build neighbour table for each triangle
	for(int i = 0; i < m_triangleList.numElem(); i++)
		BuildConnectionsUsingVerts( m_triangleList[i], i, input_verts, vert_stride, vert_ofs );

	// all done
}

}; // namespace AdjacentTriangles