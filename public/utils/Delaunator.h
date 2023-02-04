#pragma once

namespace delaunator {

struct DelaunatorPoint {
	int i;
	double x;
	double y;
	int t;
	int prev;
	int next;
	bool removed;
};

class Delaunator 
{
public:
	ArrayCRef<Vector2D>		m_coords;
	Array<int>				m_triangles{ PP_SL };
	Array<int>				m_halfedges{ PP_SL };
	Array<int>				m_hull_prev{ PP_SL };
	Array<int>				m_hull_next{ PP_SL };
	Array<int>				m_hull_tri{ PP_SL };
	int						m_hull_start{ 0 };

	Delaunator(ArrayCRef<Vector2D> in_coords);

	double					GetHullArea();

private:
	Array<int>				m_hash{ PP_SL };
	Array<int>				m_edge_stack{ PP_SL };

	TVec2D<double>			m_center{ 0.0 };
	int						m_hash_size{ 0 };

	int						Legalize(int a);
	int						Hash(double x, double y) const;
	int						AddTriangle(int i0, int i1, int i2, int a, int b, int c);
	bool					Link(int a, int b);
};

} //namespace delaunator
