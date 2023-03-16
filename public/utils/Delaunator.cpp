// based on https://github.com/mapbox/delaunator
//		and https://github.com/delfrrr/delaunator-cpp

#include "core/core_common.h"
#include "ds/sort.h"

#include "Delaunator.h"

namespace delaunator {

//@see https://stackoverflow.com/questions/33333363/built-in-mod-vs-custom-mod-function-improve-the-performance-of-modulus-op/33333636#33333636
static int fast_mod(const int i, const int c)
{
	return i >= c ? i % c : i;
}

// Kahan and Babuska summation, Neumaier variant; accumulates less FP error
static double sum(const Array<double>& x)
{
	double sum = x[0];
	double err = 0.0;

	for (int i = 1; i < x.numElem(); i++)
	{
		const double k = x[i];
		const double m = sum + k;
		err += fabs(sum) >= fabs(k) ? sum - m + k : k - m + sum;
		sum = m;
	}
	return sum + err;
}

static double dist(
	const double ax, const double ay,
	const double bx, const double by) 
{
	const double dx = ax - bx;
	const double dy = ay - by;
	return dx * dx + dy * dy;
}

static double circumRadius(
	const double ax, const double ay,
	const double bx, const double by,
	const double cx, const double cy) 
{
	const double dx = bx - ax;
	const double dy = by - ay;
	const double ex = cx - ax;
	const double ey = cy - ay;

	const double bl = dx * dx + dy * dy;
	const double cl = ex * ex + ey * ey;
	const double d = dx * ey - dy * ex;

	const double x = (ey * bl - dy * cl) * 0.5 / d;
	const double y = (dx * cl - ex * bl) * 0.5 / d;

	if ((bl > 0.0 || bl < 0.0) && (cl > 0.0 || cl < 0.0) && (d > 0.0 || d < 0.0)) 
	{
		return x * x + y * y;
	} 
	else 
	{
		return DBL_MAX;
	}
}

static bool orient(
	const double px, const double py,
	const double qx, const double qy,
	const double rx, const double ry) 
{
	return (qy - py) * (rx - qx) - (qx - px) * (ry - qy) < 0.0;
}

static TVec2D<double> circumCenter(
	const double ax, const double ay,
	const double bx, const double by,
	const double cx, const double cy) 
{
	const double dx = bx - ax;
	const double dy = by - ay;
	const double ex = cx - ax;
	const double ey = cy - ay;

	const double bl = dx * dx + dy * dy;
	const double cl = ex * ex + ey * ey;
	const double d = dx * ey - dy * ex;

	const double x = ax + (ey * bl - dy * cl) * 0.5 / d;
	const double y = ay + (dx * cl - ex * bl) * 0.5 / d;

	return TVec2D<double>(x,y);
}

struct compare 
{
	ArrayCRef<Vector2D> coords;
	TVec2D<double> center;

	int operator()(int i, int j)
	{
		const double d1 = dist(coords[i].x, coords[i].y, center.x, center.y);
		const double d2 = dist(coords[j].x, coords[j].y, center.x, center.y);
		const double diff1 = d1 - d2;
		const double diff2 = coords[i].x - coords[j].x;
		const double diff3 = coords[i].y - coords[j].y;

		if (diff1 > 0.0 || diff1 < 0.0)
		{
			return diff1;
		}
		else if (diff2 > 0.0 || diff2 < 0.0)
		{
			return diff2;
		}
		else
		{
			return diff3;
		}
	}
};

static bool inCircle(
	const double ax, const double ay,
	const double bx, const double by,
	const double cx, const double cy,
	const double px, const double py)
{
	const double dx = ax - px;
	const double dy = ay - py;
	const double ex = bx - px;
	const double ey = by - py;
	const double fx = cx - px;
	const double fy = cy - py;

	const double ap = dx * dx + dy * dy;
	const double bp = ex * ex + ey * ey;
	const double cp = fx * fx + fy * fy;

	return (dx * (ey * cp - bp * fy) -
			dy * (ex * cp - bp * fx) +
			ap * (ex * fy - ey * fx)) < 0.0;
}

constexpr double EPSILON = DBL_EPSILON;
constexpr int INVALID_INDEX = -1;

static bool checkPointsEqual(double x1, double y1, double x2, double y2)
{
	return fabs(x1 - x2) <= EPSILON && fabs(y1 - y2) <= EPSILON;
}

// monotonically increases with real angle, but doesn't need expensive trigonometry
static double pseudoAngle(const double dx, const double dy) 
{
	const double p = dx / (abs(dx) + abs(dy));
	return (dy > 0.0 ? 3.0 - p : 1.0 + p) / 4.0; // [0..1)
}

Delaunator::Delaunator(ArrayCRef<Vector2D> in_coords)
	: m_coords(in_coords)
{
	const int n = m_coords.numElem();

	double max_x = DBL_MIN;
	double max_y = DBL_MIN;
	double min_x = DBL_MAX;
	double min_y = DBL_MAX;

	Array<int> ids(PP_SL);
	ids.reserve(n);
	for (int i = 0; i < n; i++)
	{
		const double x = m_coords[i].x;
		const double y = m_coords[i].y;

		min_x = min(x, min_x);
		min_y = min(y, min_y);
		max_x = max(x, max_x);
		max_x = max(x, max_x);

		ids.append(i);
	}

	const double cx = (min_x + max_x) * 0.5;
	const double cy = (min_y + max_y) * 0.5;
	double min_dist = DBL_MAX;

	int i0 = INVALID_INDEX;
	int i1 = INVALID_INDEX;
	int i2 = INVALID_INDEX;

	// pick a seed point close to the centroid
	for (int i = 0; i < n; i++)
	{
		const double d = dist(cx, cy, m_coords[i].x, m_coords[i].y);
		if (d < min_dist)
		{
			i0 = i;
			min_dist = d;
		}
	}

	const double i0x = m_coords[i0].x;
	const double i0y = m_coords[i0].y;

	min_dist = DBL_MAX;

	// find the point closest to the seed
	for (int i = 0; i < n; i++)
	{
		if (i == i0) 
			continue;

		const double d = dist(i0x, i0y, m_coords[i].x, m_coords[i].y);
		if (d < min_dist && d > 0.0)
		{
			i1 = i;
			min_dist = d;
		}
	}

	double i1x = m_coords[i1].x;
	double i1y = m_coords[i1].y;

	double min_radius = DBL_MAX;

	// find the third point which forms the smallest circumcircle with the first two
	for (int i = 0; i < n; i++) 
	{
		if (i == i0 || i == i1) 
			continue;

		const double r = circumRadius(i0x, i0y, i1x, i1y, m_coords[i].x, m_coords[i].y);

		if (r < min_radius) 
		{
			i2 = i;
			min_radius = r;
		}
	}

	if (!(min_radius < DBL_MAX)) {
		ASSERT_FAIL("not triangulation");
	}

	double i2x = m_coords[i2].x;
	double i2y = m_coords[i2].y;

	if (orient(i0x, i0y, i1x, i1y, i2x, i2y)) 
	{
		QuickSwap(i1, i2);
		QuickSwap(i1x, i2x);
		QuickSwap(i1y, i2y);
	}

	m_center = circumCenter(i0x, i0y, i1x, i1y, i2x, i2y);

	// sort the points by distance from the seed triangle circumcenter
	quickSort(ids, compare{ m_coords, m_center });

	// initialize a hash table for storing edges of the advancing convex hull
	m_hash_size = static_cast<int>(llround(ceil(sqrt(n))));
	m_hash.assureSizeEmplace(m_hash_size, INVALID_INDEX);

	// initialize arrays for tracking the edges of the advancing convex hull
	m_hull_prev.setNum(n);
	m_hull_next.setNum(n);
	m_hull_tri.setNum(n);

	m_hull_start = i0;

	int hull_size = 3;

	m_hull_next[i0] = m_hull_prev[i2] = i1;
	m_hull_next[i1] = m_hull_prev[i0] = i2;
	m_hull_next[i2] = m_hull_prev[i1] = i0;

	m_hull_tri[i0] = 0;
	m_hull_tri[i1] = 1;
	m_hull_tri[i2] = 2;

	m_hash[Hash(i0x, i0y)] = i0;
	m_hash[Hash(i1x, i1y)] = i1;
	m_hash[Hash(i2x, i2y)] = i2;

	const int max_triangles = n < 3 ? 1 : 2 * n - 5;
	m_triangles.reserve(max_triangles * 3);
	m_halfedges.reserve(max_triangles * 3);
	AddTriangle(i0, i1, i2, INVALID_INDEX, INVALID_INDEX, INVALID_INDEX);
	double xp = NAN;
	double yp = NAN;

	for (int k = 0; k < n; k++) 
	{
		const int i = ids[k];
		const double x = m_coords[i].x;
		const double y = m_coords[i].y;

		// skip near-duplicate points
		if (k > 0 && checkPointsEqual(x, y, xp, yp))
			continue;

		xp = x;
		yp = y;

		// skip seed triangle points
		if (checkPointsEqual(x, y, i0x, i0y) ||
			checkPointsEqual(x, y, i1x, i1y) ||
			checkPointsEqual(x, y, i2x, i2y))
			continue;

		// find a visible edge on the convex hull using edge hash
		int start = 0;

		const int key = Hash(x, y);
		for (int j = 0; j < m_hash_size; j++)
		{
			start = m_hash[fast_mod(key + j, m_hash_size)];
			if (start != INVALID_INDEX && start != m_hull_next[start])
				break;
		}

		start = m_hull_prev[start];
		int e = start;
		int q;

		while (q = m_hull_next[e], !orient(x, y, m_coords[e].x, m_coords[e].y, m_coords[q].x, m_coords[q].y))
		{ 
			e = q;
			if (e == start)
			{
				e = INVALID_INDEX;
				break;
			}
		}

		if (e == INVALID_INDEX)
			continue; // likely a near-duplicate point; skip it

		// add the first triangle from the point
		{
			const int t = AddTriangle(e, i, m_hull_next[e], INVALID_INDEX, INVALID_INDEX, m_hull_tri[e]);

			m_hull_tri[i] = Legalize(t + 2);
			m_hull_tri[e] = t;
			hull_size++;
		}

		// walk forward through the hull, adding more triangles and flipping recursively
		int next = m_hull_next[e];
		while (
			q = m_hull_next[next],
			orient(x, y, m_coords[next].x, m_coords[next].y, m_coords[q].x, m_coords[q].y)) 
		{
			const int t = AddTriangle(next, i, q, m_hull_tri[i], INVALID_INDEX, m_hull_tri[next]);
			m_hull_tri[i] = Legalize(t + 2);
			m_hull_next[next] = next; // mark as removed
			hull_size--;
			next = q;
		}

		// walk backward from the other side, adding more triangles and flipping
		if (e == start)
		{
			while (
				q = m_hull_prev[e],
				orient(x, y, m_coords[q].x, m_coords[q].y, m_coords[e].x, m_coords[e].y)) 
			{
				const int t = AddTriangle(q, i, e, INVALID_INDEX, m_hull_tri[e], m_hull_tri[q]);
				Legalize(t + 2);
				m_hull_tri[q] = t;
				m_hull_next[e] = e; // mark as removed
				hull_size--;
				e = q;
			}
		}

		// update the hull indices
		m_hull_prev[i] = e;
		m_hull_start = e;
		m_hull_prev[next] = i;
		m_hull_next[e] = i;
		m_hull_next[i] = next;

		m_hash[Hash(x, y)] = i;
		m_hash[Hash(m_coords[e].x, m_coords[e].y)] = e;
	}
}

double Delaunator::GetHullArea()
{
	Array<double> hull_area(PP_SL);
	int e = m_hull_start;
	do {
		hull_area.append((m_coords[e].x - m_coords[m_hull_prev[e]].x) * (m_coords[e].y + m_coords[m_hull_prev[e]].y));
		e = m_hull_next[e];
	} while (e != m_hull_start);

	return sum(hull_area);
}

int Delaunator::Legalize(int a)
{
	int i = 0;
	int ar = 0;
	m_edge_stack.clear();

	// recursion eliminated with a fixed-size stack
	while (true)
	{
		const int b = m_halfedges[a];

		/* if the pair of triangles doesn't satisfy the Delaunay condition
		* (p1 is inside the circumcircle of [p0, pl, pr]), flip them,
		* then do the same check/flip recursively for the new pair of triangles
		*
		*           pl                    pl
		*          /||\                  /  \
		*       al/ || \bl            al/    \a
		*        /  ||  \              /      \
		*       /  a||b  \    flip    /___ar___\
		*     p0\   ||   /p1   =>   p0\---bl---/p1
		*        \  ||  /              \      /
		*       ar\ || /br             b\    /br
		*          \||/                  \  /
		*           pr                    pr
		*/
		const int a0 = 3 * (a / 3);
		ar = a0 + (a + 2) % 3;

		if (b == INVALID_INDEX) {
			if (i > 0) {
				i--;
				a = m_edge_stack[i];
				continue;
			}
			else {
				//i = INVALID_INDEX;
				break;
			}
		}

		const int b0 = 3 * (b / 3);
		const int al = a0 + (a + 1) % 3;
		const int bl = b0 + (b + 2) % 3;

		const int p0 = m_triangles[ar];
		const int pr = m_triangles[a];
		const int pl = m_triangles[al];
		const int p1 = m_triangles[bl];

		const bool illegal = inCircle(
			m_coords[p0].x, m_coords[p0].y,
			m_coords[pr].x, m_coords[pr].y,
			m_coords[pl].x, m_coords[pl].y,
			m_coords[p1].x, m_coords[p1].y);

		if (illegal)
		{
			m_triangles[a] = p1;
			m_triangles[b] = p0;

			auto hbl = m_halfedges[bl];

			// edge swapped on the other side of the hull (rare); fix the halfedge reference
			if (hbl == INVALID_INDEX)
			{
				int e = m_hull_start;
				do {
					if (m_hull_tri[e] == bl)
					{
						m_hull_tri[e] = a;
						break;
					}
					e = m_hull_next[e];
				} while (e != m_hull_start);
			}

			Link(a, hbl);
			Link(b, m_halfedges[ar]);
			Link(ar, bl);
			const int br = b0 + (b + 1) % 3;

			if (i < m_edge_stack.numElem())
			{
				m_edge_stack[i] = br;
			}
			else
			{
				m_edge_stack.append(br);
			}
			i++;

		}
		else if(i > 0)
		{
			i--;
			a = m_edge_stack[i];
			continue;
		}
		else
		{
			break;
		}
	}
	return ar;
}

int Delaunator::Hash(const double x, const double y) const 
{
	const double dx = x - m_center.x;
	const double dy = y - m_center.y;
	return fast_mod(static_cast<int>(llround(floor(pseudoAngle(dx, dy) * static_cast<double>(m_hash_size)))), m_hash_size);
}

int Delaunator::AddTriangle(
	int i0, int i1, int i2,
	int a, int b, int c) 
{
	int t = m_triangles.numElem();
	m_triangles.append(i0);
	m_triangles.append(i1);
	m_triangles.append(i2);
	Link(t, a);
	Link(t + 1, b);
	Link(t + 2, c);
	return t;
}

bool Delaunator::Link(const int a, const int b)
{
	int s = m_halfedges.numElem();

	if (a == s)
	{
		m_halfedges.append(b);
	}
	else if (a < s) 
	{
		m_halfedges[a] = b;
	}
	else 
	{
		ASSERT_FAIL("Cannot link edge");
		return false;
	}

	if (b != INVALID_INDEX) 
	{
		const int s2 = m_halfedges.numElem();

		if (b == s2)
		{
			m_halfedges.append(a);
		}
		else if (b < s2) 
		{
			m_halfedges[b] = a;
		}
		else 
		{
			ASSERT_FAIL("Cannot link edge");
			return false;
		}
	}
	return true;
}

} //namespace delaunator
