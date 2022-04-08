//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: The triangle adjacency generator
//////////////////////////////////////////////////////////////////////////////////

#include "AdjacentTriangles.h"
#include "math/Vector.h"

namespace AdjacentTriangles
{

CAdjacentTriangleGraph::CAdjacentTriangleGraph(const DkList<int>& indices)
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

int CAdjacentTriangleGraph::FindTriangleByEdge(	const medge_t &edge,
												int& found_edge_idx, 
												bool ignore_order) const
{
	for(int i = 0; i < m_triangleList.numElem(); i++)
	{
		if(m_triangleList[i].IsOwnEdge(edge, found_edge_idx, ignore_order))
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
// OptiPoly
//----------------------------------------------------------------------------------------------

struct itriangle
{
	int idxs[3];
};

typedef DkList<itriangle> indxgroup_t;

// compares triangle indices
bool triangle_compare(itriangle &tri1, itriangle &tri2)
{
	return	(tri1.idxs[0] == tri2.idxs[0]) &&
			(tri1.idxs[1] == tri2.idxs[1]) && 
			(tri1.idxs[2] == tri2.idxs[2]);
}

// searches trinanle, returns index
int find_triangle(DkList<itriangle> *tris, itriangle &tofind)
{
	for(int i = 0; i < tris->numElem(); i++)
	{
		if(triangle_compare(tris->ptr()[i], tofind))
			return i;
	}

	return -1;
}

// adds triangle to index group recursively
void AddTriangleWithAllNeighbours_r(indxgroup_t *group, mtriangle_t* triangle)
{
	if(!triangle)
		return;

	itriangle first = {triangle->indices[0], triangle->indices[1], triangle->indices[2]};

	bool discard = false;

	for(int i = 0; i < group->numElem(); i++)
	{
		if(triangle_compare(first, group->ptr()[i]))
		{
			discard = true;
			break;
		}
	}

	// don't add this triangle again
	if(!discard)
	{
		group->append( first );

		// recurse to it's neighbours
		for(int i = 0; i < triangle->index_connections.numElem(); i++)
			AddTriangleWithAllNeighbours_r(group, triangle->index_connections[i]); 
	}
}

void CAdjacentTriangleGraph::GenOptimizedTriangleList( DkList<int>& output )
{
	// sort triangles to new groups
	DkList<indxgroup_t*> allgroups;

	indxgroup_t *main_group = new indxgroup_t;
	allgroups.append(main_group);

	// then using newly generated neighbour triangles divide on parts
	// add tri with all of it's neighbour's herarchy
	AddTriangleWithAllNeighbours_r(main_group, &m_triangleList[0]);

	for(int i = 1; i < m_triangleList.numElem(); i++)
	{
		itriangle triangle = {m_triangleList[i].indices[0], m_triangleList[i].indices[1], m_triangleList[i].indices[2]};

		bool found = false;

		// find this triangle in all previous groups
		for(int j = 0; j < allgroups.numElem(); j++)
		{
			if(find_triangle(allgroups[j], triangle) != -1)
			{
				found = true;
				break;
			}
		}

		// if not found, create new group and add triangle with all of it's neighbours
		if(!found)
		{
			indxgroup_t *new_group = new indxgroup_t;
			allgroups.append(new_group);

			// add tri with all of it's neighbour's herarchy
			AddTriangleWithAllNeighbours_r(new_group, &m_triangleList[i]);
		}
	}

	// next we need to join groups
	for(int i = 0; i < allgroups.numElem(); i++)
	{
		output.resize(output.numElem() + allgroups[i]->numElem()*3);

		for(int j = 0; j < allgroups[i]->numElem(); j++)
		{
			output.append(allgroups[i]->ptr()[j].idxs[0]);
			output.append(allgroups[i]->ptr()[j].idxs[1]);
			output.append(allgroups[i]->ptr()[j].idxs[2]);
		}

		delete allgroups[i];
	}

	// done
}

void CAdjacentTriangleGraph::GenOptimizedStrips( DkList<int>& output, bool usePrimRestart )
{
	ASSERTMSG(false, "CAdjacentTriangleGraph::GenOptimizedStrips not implemented");
}

// builds neigbourhood table for specified triangle, also using a vertex information
void CAdjacentTriangleGraph::BuildTriangleAdjacencyConnections( mtriangle_t* pTri, int tri_id )
{
	for(int i = 0; i < m_triangleList.numElem(); i++)
	{
		mtriangle_t* pCheckTriangle = &m_triangleList[i];

		// skip this triangle
		if(tri_id == i)
			continue;

		// check each index for corresponding neighbourhood
		for(int j = 0; j < 3; j++)
		{
			bool next_build = false;

			for(int k = 0; k < 3; k++)
			{
				// if we have same index, so we have a 
				if(pTri->indices[j] == pCheckTriangle->indices[k])
				{
					// add it as unique
					pTri->index_connections.addUnique( pCheckTriangle );
					next_build = true;

					break; // k
				}
			}

			if(next_build)
				break; // j
		}

		// is edges connected by this triangle?
		for(int j = 0; j < 3; j++)
		{
			int edge_idx;

			if(pTri->IsOwnEdge( pCheckTriangle->edges[j], edge_idx))
			{
				pCheckTriangle->edge_connections[j] = pTri;
			}
		}
	}
}

inline Vector3D UTIL_VertexAtPos(const ubyte* vertexBaseOffset, int stride, int vertexIndex)
{
	ubyte* pDataVertex = (ubyte*)vertexBaseOffset + vertexIndex * stride;
	return *(Vector3D*)pDataVertex;
}

void CAdjacentTriangleGraph::BuildTriangleAdjacencyConnectionsByVerts(mtriangle_t* pTri, 
																		int tri_id,
																		const ubyte* input_verts, 
																		int vert_stride, 
																		int vert_ofs)
{
	for(int i = 0; i < m_triangleList.numElem(); i++)
	{
		mtriangle_t* pCheckTriangle = &m_triangleList[i];

		if(tri_id == i)
			continue;

		// check each index for corresponding neighbourhood
		for(int j = 0; j < 3; j++)
		{
			bool next_build = false;

			for(int k = 0; k < 3; k++)
			{
				// compare verts
				Vector3D vertexJ = UTIL_VertexAtPos(input_verts+vert_ofs, vert_stride, pTri->indices[j]);
				Vector3D vertexK = UTIL_VertexAtPos(input_verts+vert_ofs, vert_stride, pCheckTriangle->indices[k]);

				float dist = length(vertexJ-vertexK);

				// if we have same index, or same position, make it as neighbour
				if(pTri->indices[j] == pCheckTriangle->indices[k] || (dist < F_EPS))
				{
					// add it as unique
					pTri->index_connections.addUnique( pCheckTriangle );
					next_build = true;

					break; // k
				}
			}

			if(next_build)
				break; // j

			// setup neighbour edges connection
			for(int k = 0; k < 3; k++)
			{
				int edge_idx;

				if(pTri->IsOwnEdge( pCheckTriangle->edges[k], edge_idx))
					pCheckTriangle->edge_connections[k] = pTri;
			}
		}
	}
}

// builds adjacency and neigbourhood data for triangles using only indices
void CAdjacentTriangleGraph::Build( const int* in_trilistidxs, int num_indices)
{
	int tri_id = 0;

	m_triangleList.resize(num_indices / 3);

	// first, divide all indices on triangles
	for(int i = 0; i < num_indices; i+=3, tri_id++)
	{
		mtriangle_t new_triangle;

		new_triangle.indices[0] = in_trilistidxs[i];
		new_triangle.indices[1] = in_trilistidxs[i+1];
		new_triangle.indices[2] = in_trilistidxs[i+2];

		new_triangle.BuildEdge(0, new_triangle.edges[0]);
		new_triangle.BuildEdge(1, new_triangle.edges[1]);
		new_triangle.BuildEdge(2, new_triangle.edges[2]);

		new_triangle.tri_id = tri_id;

		// add new triangle
		m_triangleList.append(new_triangle);
	}

	// next build neighbour table for each triangle
	for(int i = 0; i < m_triangleList.numElem(); i++)
		BuildTriangleAdjacencyConnections( &m_triangleList[i], i );

	// all done
}

// builds adjacency and neigbourhood data for triangles with vertex position comparison
void CAdjacentTriangleGraph::BuildWithVertices(	const ubyte* input_verts, 
																			int vert_stride, 
																			int vert_ofs, 
																			const int* in_trilistidxs, 
																			int num_indices)
{
	int tri_id = 0;

	m_triangleList.resize(num_indices / 3);

	// first, divide all indices on triangles
	for(int i = 0; i < num_indices; i+=3, tri_id++)
	{
		mtriangle_t new_triangle;

		new_triangle.indices[0] = in_trilistidxs[i];
		new_triangle.indices[1] = in_trilistidxs[i+1];
		new_triangle.indices[2] = in_trilistidxs[i+2];

		new_triangle.BuildEdge(0, new_triangle.edges[0]);
		new_triangle.BuildEdge(1, new_triangle.edges[1]);
		new_triangle.BuildEdge(2, new_triangle.edges[2]);

		new_triangle.tri_id = tri_id;

		// add new triangle
		m_triangleList.append(new_triangle);
	}

	// next build neighbour table for each triangle
	for(int i = 0; i < m_triangleList.numElem(); i++)
		BuildTriangleAdjacencyConnectionsByVerts( &m_triangleList[i], i, input_verts, vert_stride, vert_ofs );

	// all done
}

}; // namespace AdjacentTriangles