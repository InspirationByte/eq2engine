//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Lightmap UV generation and storing to the new surfaces
//
//				TODO:	Miltithreaded chart placement
//						Valid surfaceindex-optimized sort for lightmap pages
//////////////////////////////////////////////////////////////////////////////////

#include "eqwc.h"

//#include "lightmap_uv_cl.h"

#include "math/Rectangle.h"
#include "mtriangle_framework.h"
#include "Utils/rasterizer.h"
#include "threadedprocess.h"

using namespace MTriangle;

// FIXME: put this in the new Tri2D lib
bool IsPointInTriangle2D(const Vector2D& A, const Vector2D& B, const Vector2D& C, const Vector2D& P)
{
	// Compute vectors        
	Vector2D v0 = C - A;
	Vector2D v1 = B - A;
	Vector2D v2 = P - A;

	// Compute dot products
	float dot00 = dot(v0, v0);
	float dot01 = dot(v0, v1);
	float dot02 = dot(v0, v2);
	float dot11 = dot(v1, v1);
	float dot12 = dot(v1, v2);

	// Compute barycentric coordinates
	float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	// Check if point is in triangle
	return (u >= 0) && (v >= 0) && (u + v < 1);
}

bool IsTriangleIntersectsTriangle2D(const Vector2D* triA, const Vector2D* triB)
{
	// the double checkings
	if( IsPointInTriangle2D(triA[0], triA[1], triA[2], triB[0]) ||
		IsPointInTriangle2D(triA[0], triA[1], triA[2], triB[1]) ||
		IsPointInTriangle2D(triA[0], triA[1], triA[2], triB[2]) 
		||
		IsPointInTriangle2D(triB[0], triB[1], triB[2], triA[0]) ||
		IsPointInTriangle2D(triB[0], triB[1], triB[2], triA[1]) ||
		IsPointInTriangle2D(triB[0], triB[1], triB[2], triA[2]))
		return true;

	return false;
}

//--------------------------------

struct cwlightmap_t
{
	ubyte*	lightmap_pack_image;
	ubyte*	lightmap_pack_image_lo;	// low resolution map

	int		used_area;				// used pixels count to check chart area

	int		last_x;					// last pack positions
	int		last_y;
};

DkList<cwlitsurface_t*> g_litsurfaces;
DkList<cwlightmap_t*>	g_pLightmaps;	// generated lightmap images

cwlightmap_t* AllocLightmap()
{
	cwlightmap_t* pLightmap = new cwlightmap_t;

	int quarter_size = worldGlobals.lightmapSize / 4;

	pLightmap->lightmap_pack_image = (ubyte*)malloc(worldGlobals.lightmapSize * worldGlobals.lightmapSize);
	pLightmap->lightmap_pack_image_lo = (ubyte*)malloc(quarter_size * quarter_size);

	memset( pLightmap->lightmap_pack_image, 0, worldGlobals.lightmapSize * worldGlobals.lightmapSize );
	memset( pLightmap->lightmap_pack_image_lo, 0, quarter_size * quarter_size );

	pLightmap->used_area = 0;
	pLightmap->last_x = 0;
	pLightmap->last_y = 0;

	g_pLightmaps.append(pLightmap);

	return pLightmap;
}

void FreeLightmap(cwlightmap_t* pLightmap)
{
	free(pLightmap->lightmap_pack_image);
	free(pLightmap->lightmap_pack_image_lo);
	delete pLightmap;
}

struct lm_chart_t;

DkList<lm_chart_t*>		g_usedcharts;
DkList<lm_chart_t*>		g_charts;

// macro converts vertex position to luxel-space
#define LUXEL(x) (((x)*METERS_PER_UNIT) * worldGlobals.fLuxelsPerMeter)

struct lm_chart_t
{
	lm_chart_t()
	{
		image = NULL;
		image_lo = NULL;
		area = 0;
		paintarea = 0;
		used = false;
		lm_index = -1;
		x,y = 0;
	}

	DkList<int>		indices;

	Vector3D		normal;

	Vector3D		tangent;
	Vector3D		binormal;

	cwlitsurface_t*	pSurface;
	cwsurface_t*	pUnlitSurface;

	Rectangle_t		rect;

	ubyte*			image;
	uint			iwide,itall;

	ubyte*			image_lo;
	uint			iwide_lo,itall_lo;

	uint			area;
	uint			paintarea;

	int				lm_index;

	uint			x,y;

	bool			used;
};

bool IsChartIntersectsChart(lm_chart_t* pChartA, lm_chart_t* pChartB, const Vector2D& posA, const Vector2D& posB)
{
	Rectangle_t rA(posA, Vector2D(pChartA->iwide, pChartA->itall)+posA);
	Rectangle_t rB(posB, Vector2D(pChartB->iwide, pChartB->itall)+posB);

	if(!rA.IsIntersectsRectangle(rB))
		return false;

	Vector2D offsetA = (-pChartA->rect.vleftTop) + Vector2D(worldGlobals.nPackThreshold);

	Vector2D offsetB = (-pChartB->rect.vleftTop) + Vector2D(worldGlobals.nPackThreshold);

	for(int i = 0; i < pChartA->indices.numElem(); i += 3)
	{
		for(int j = 0; j < pChartB->indices.numElem(); j += 3)
		{
			int src_idxA[3] = {pChartA->indices[i], pChartA->indices[i]+1, pChartA->indices[i]+2};
			int src_idxB[3] = {pChartB->indices[j], pChartB->indices[j]+1, pChartB->indices[j]+2};

			Vector2D vertsA[3] = {	pChartA->pSurface->pVerts[src_idxA[0]].texcoord.zw(), 
									pChartA->pSurface->pVerts[src_idxA[1]].texcoord.zw(),
									pChartA->pSurface->pVerts[src_idxA[2]].texcoord.zw()};

			Vector2D vertsB[3] = {	pChartA->pSurface->pVerts[src_idxB[0]].texcoord.zw(), 
									pChartA->pSurface->pVerts[src_idxB[1]].texcoord.zw(),
									pChartA->pSurface->pVerts[src_idxB[2]].texcoord.zw()};

			vertsA[0] += posA + offsetA;
			vertsA[1] += posA + offsetA;
			vertsA[2] += posA + offsetA;

			vertsB[0] += posB + offsetB;
			vertsB[1] += posB + offsetB;
			vertsB[2] += posB + offsetB;

			if(IsTriangleIntersectsTriangle2D(vertsA, vertsB))
				return true;
		}
	}

	// the slower variant
	return false;
}

// allocates new chart
lm_chart_t* AllocChart()
{
	lm_chart_t* pChart = new lm_chart_t;
	g_charts.append(pChart);

	return pChart;
}

// frees chart
void FreeChart(lm_chart_t* pChart)
{
	g_charts.remove(pChart);

	if(pChart->image)
		free(pChart->image);

	if(pChart->image_lo)
		free(pChart->image_lo);

	delete pChart;
}

// frees all charts
void FreeAllCharts()
{
	for(int i = 0; i < g_charts.numElem(); i++)
	{
		if(g_charts[i]->image != NULL)
			free( g_charts[i]->image );

		delete g_charts[i];
	}

	g_charts.clear();
}

void ChartMakeVertsAndCoords(cwsurface_t* pSurface, DkList<lm_chart_t> &charts, DkList<eqlevelvertexlm_t> &verts, DkList<int> &indices);

// TODO: next code must be in polylib

struct itriangle
{
	int idxs[3];
};

typedef DkList<itriangle> indxgroup_t;

// compares triangle indices
bool triangle_compare(const itriangle& tri1, const itriangle& tri2)
{
	return	(tri1.idxs[0] == tri2.idxs[0]) &&
			(tri1.idxs[1] == tri2.idxs[1]) && 
			(tri1.idxs[2] == tri2.idxs[2]);
}

// searches trinanle, returns index
int find_triangle(DkList<itriangle>* tris, const itriangle& tofind)
{
	for(int i = 0; i < tris->numElem(); i++)
	{
		if(triangle_compare(tris->ptr()[i], tofind))
			return i;
	}

	return -1;
}

// adds triangle to index group
void AddTriangleWithAllNeighbours_r(indxgroup_t *group, mtriangle_t* triangle)
{
	if(!triangle)
		return;

	itriangle first = {triangle->indices[0], triangle->indices[1], triangle->indices[2]};

	bool discard = false;

	for(int i = 0; i < group->numElem(); i++)
	{
		if( triangle_compare(first, group->ptr()[i]) )
		{
			discard = true;
			break;
		}
	}

	if(!discard)
	{
		group->append(first);

		// recurse to it's neighbours
		for(int i = 0; i < triangle->index_connections.numElem(); i++)
			AddTriangleWithAllNeighbours_r(group, triangle->index_connections[i]);

		// Do edge check instead vertex
		//for(int i = 0; i < 3; i++)
		//	AddTriangleWithAllNeighbours_r(group, triangle->edge_connections[i]);
	}
}

// polylib end

//------------------------------------------------------------------------------
// ChartNightbourDivision - divides chart on many charts based on neighbour tris
//------------------------------------------------------------------------------
void ChartNeighbourDivision(lm_chart_t *chart, DkList<lm_chart_t*> &charts, eqlevelvertex_t *verts)
{

	CAdjacentTriangleGraph triangles_graph;

	triangles_graph.BuildWithVertices(	(ubyte*)verts, 
										sizeof(eqlevelvertex_t), 
										offsetOf(eqlevelvertex_t, position),
										chart->indices.ptr(),
										chart->indices.numElem() );

	DkList<indxgroup_t*>	allgroups;
	DkList<mtriangle_t>*	triangles = triangles_graph.GetTriangles();

	//
	// sort triangles to new groups
	//

	indxgroup_t* main_group = new indxgroup_t;
	allgroups.append( main_group );

	// then using newly generated neighbour triangles divide on parts
	// add tri with all of it's neighbour's herarchy
	AddTriangleWithAllNeighbours_r(main_group, &triangles->ptr()[0]);

	for(int i = 1; i < triangles->numElem(); i++)
	{
		itriangle triangle = {	triangles->ptr()[i].indices[0], 
								triangles->ptr()[i].indices[1], 
								triangles->ptr()[i].indices[2]};

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
			indxgroup_t* new_group = new indxgroup_t;
			allgroups.append(new_group);

			// add tri with all of it's neighbour's herarchy
			AddTriangleWithAllNeighbours_r(new_group, &triangles->ptr()[i]);
		}
	}

	//
	// copy triangles to new charts
	//
	for(int i = 0; i < allgroups.numElem(); i++)
	{
		ThreadLock();
		lm_chart_t *new_chart = AllocChart();

		// set the surface
		new_chart->pUnlitSurface = chart->pUnlitSurface;

		ThreadUnlock();

		new_chart->normal = chart->normal;
		new_chart->indices.resize(allgroups[i]->numElem()*3);

		for(int j = 0; j < allgroups[i]->numElem(); j++)
		{
			new_chart->indices.append(allgroups[i]->ptr()[j].idxs[0]);
			new_chart->indices.append(allgroups[i]->ptr()[j].idxs[1]);
			new_chart->indices.append(allgroups[i]->ptr()[j].idxs[2]);
		}

		// this charts will be used
		new_chart->used = true;

		//
		// TODO: Final check: check all triangles for overlapping TC and generate new charts from intersecting
		//
		// GenChartVerts(new_chart);
		// SubdivideChartBySelfIntersecting(new_chart);

		ThreadLock();
		charts.append(new_chart);
		ThreadUnlock();

		// clenup
		delete allgroups[i];
	}
}

//------------------------------------------------------------------------------
// MakeCharts - generates charts for the surface, generates new verts and indxs
//------------------------------------------------------------------------------
void MakeCharts(cwsurface_t* pSurface, DkList<lm_chart_t*> &charts, Vector3D &normal, DkList<eqlevelvertexlm_t>& new_verts, DkList<int>& new_indices)
{
	DkList<lm_chart_t*> pre_charts;

	Vector3D	check_normal = normal;
	bool		has_new_normal = false;

	lm_chart_t*	cur_chart = NULL;
	Vector3D	cur_normal;

	DkList<int> triangles_used;
	triangles_used.resize(pSurface->numIndices / 3);

	//------------------------------------------------------------------
	// PHASE 1: Perform normal-based division on charts
	//------------------------------------------------------------------

	Vector3D v_final_normal;

get_back:

	ThreadLock();

	if(!cur_chart)
		cur_chart = AllocChart();

	// set the surface
	cur_chart->pUnlitSurface = pSurface;

	ThreadUnlock();

	cur_chart->indices.resize(pSurface->numIndices);

	// set chart normal
	cur_chart->normal = check_normal;

	cur_normal = check_normal;
	v_final_normal = vec3_zero;

	Rectangle_t rect;

	has_new_normal = false;

	int tri_id = 0;

	// skip invalid mesh groups
	bool bSkip = (triangles_used.numElem() == pSurface->numIndices/3);

	// add more triangles to one group as possible
	for(int i = 0; i < pSurface->numIndices && !bSkip; i+=3, tri_id++)
	{
		bool must_check = (triangles_used.findIndex(tri_id) == -1);

		if(!must_check)
			continue;

		Vector3D tri_normal;
		int triIdx[3] = {pSurface->pIndices[i], pSurface->pIndices[i+1], pSurface->pIndices[i+2]};
		ComputeTriangleNormal( pSurface->pVerts[triIdx[0]].position, pSurface->pVerts[triIdx[1]].position, pSurface->pVerts[triIdx[2]].position, tri_normal);

		float fPickAngle = dot(cur_normal, tri_normal);

		// add triangles to current chart if it is in angle
		if(fPickAngle > worldGlobals.fPickThreshold)
		{
			cur_chart->indices.append(triIdx[0]);
			cur_chart->indices.append(triIdx[1]);
			cur_chart->indices.append(triIdx[2]);

			// store our triangle to used list
			triangles_used.append(tri_id);

			v_final_normal += tri_normal;
		}
		else
		{
			if(!has_new_normal)
			{
				// if this triangle is not corresponds the threshold, generate new group
				has_new_normal = true;

				check_normal = tri_normal;
			}
		}
	}

	// normalize final normal and set
	cur_chart->normal = normalize(v_final_normal);

	pre_charts.append(cur_chart);

	cur_chart = NULL;

	// go back and fill new chart
	if(has_new_normal)
		goto get_back;

	//------------------------------------------------------------------
	// PHASE 2: Subdivide on subcharts by checking neighbour triangles
	//------------------------------------------------------------------

	// divide triangle parts on new charts
	for(int i = 0; i < pre_charts.numElem(); i++)
	{
		ChartNeighbourDivision(pre_charts[i], charts, pSurface->pVerts); // new_verts);

		ThreadLock();
		FreeChart(pre_charts[i]);
		ThreadUnlock();
	}
}

//------------------------------------------------------------------------------
// CopySurfaceDataToLit - converts surface to lit
//------------------------------------------------------------------------------
void CopySurfaceDataToLit(cwsurface_t* pSrc, cwlitsurface_t* pDst)
{
	pDst->flags			= pSrc->flags;
	pDst->numIndices	= pSrc->numIndices;
	pDst->numVertices	= pSrc->numVertices;

	pDst->pMaterial		= pSrc->pMaterial;

	pDst->pVerts = new eqlevelvertexlm_t[pDst->numVertices];
	pDst->pIndices = new int[pDst->numIndices];

	pDst->bbox.Reset();

	for(int i = 0; i < pDst->numVertices; i++)
	{
		pDst->pVerts[i] = pSrc->pVerts[i];
		pDst->bbox.AddVertex(pDst->pVerts[i].position);
	}

	memcpy(pDst->pIndices, pSrc->pIndices, sizeof(int)*pDst->numIndices);
}

//------------------------------------------------------------------------------
// ImagePutPixelsInThreshRadius - radial paints on image using placement thresh
//------------------------------------------------------------------------------
void ImagePutPixelsInThreshRadius(uint px, uint py, uint iwide, uint itall, ubyte* image)
{
	for(uint y = 0; y < worldGlobals.nPackThreshold*2; y++)
	{
		for(uint x = 0; x < worldGlobals.nPackThreshold*2; x++)
		{
			uint paint_x = px+(x-worldGlobals.nPackThreshold);
			uint paint_y = py+(y-worldGlobals.nPackThreshold);

			//if(length(Vector2D(x-g_nPackThreshold, y-g_nPackThreshold)) > g_nPackThreshold)
			//	continue;

			if((paint_x >= 0) && (paint_x < iwide) && (paint_y >= 0) && (paint_y < itall))
				image[paint_y * iwide + paint_x] = 1;
		}
	}
}

//------------------------------------------------------------------------------
// ChartMakeImageStamp - creates a temporary image that relates 
// to the chart lightmap coordiantes
//------------------------------------------------------------------------------
void ChartMakeImageStamp(lm_chart_t *chart, const DkList<eqlevelvertexlm_t>& verts, const DkList<int>& indices)
{
	Vector2D rect_size = chart->rect.GetSize() + Vector2D(float(worldGlobals.nPackThreshold)*4.0f);

	chart->iwide = ceil(rect_size.x);
	chart->itall = ceil(rect_size.y);

	chart->image = (ubyte*)malloc(chart->iwide*chart->itall);

	// rasterize our stamp on new image
	CRasterizer<ubyte> raster;

	raster.SetImage(chart->image, chart->iwide, chart->itall);
	raster.Clear(0);

	Vector2D offset = -chart->rect.GetLeftTop() + Vector2D(worldGlobals.nPackThreshold*2.0f);

	int nDrawnPixels = 0;

	// draw out triangles to make stamp
	for(int i = 0; i < indices.numElem(); i+=3)
	{
		int src_idx0 = indices[i];
		int src_idx1 = indices[i+1];
		int src_idx2 = indices[i+2];

		// check image to vertex bounds
		Vector2D v0 = verts[src_idx0].texcoord.zw();// + offset;
		Vector2D v1 = verts[src_idx1].texcoord.zw();// + offset;
		Vector2D v2 = verts[src_idx2].texcoord.zw();// + offset;

		if(!chart->rect.IsInRectangle(v0) || !chart->rect.IsInRectangle(v1) || !chart->rect.IsInRectangle(v2))
		{
			MsgWarning("WARNING: Triangle vertex is outside rectangle:\n[%g %g] [%g %g] [%g %g]\n", v0.x,v0.y,v1.x,v1.y,v2.x,v2.y);
			ASSERT(!"stop");
		}

		v0 += offset;
		v1 += offset;
		v2 += offset;

		
		if((v0.x > chart->iwide || v0.y > chart->itall ||
			v1.x > chart->iwide || v1.y > chart->itall ||
			v2.x > chart->iwide || v2.y > chart->itall ||
			v0.x < 0 || v0.y < 0 ||
			v1.x < 0 || v1.y < 0 ||
			v2.x < 0 || v2.y < 0))
		{
			MsgWarning("WARNING: Triangle vertex is outside chart image:\n[%g %g] [%g %g] [%g %g]\n", v0.x,v0.y,v1.x,v1.y,v2.x,v2.y);
			MsgWarning("Chart image: %d %d\n", chart->iwide, chart->itall);
		}
		

		raster.ResetOcclusionTest();

		raster.DrawTriangle( 1, v0, 1, v1, 1, v2 );

		nDrawnPixels += raster.GetDrawnPixels();

		raster.ResetOcclusionTest();

		raster.DrawLine(1,v0,1,v1);
		raster.DrawLine(1,v1,1,v2);
		raster.DrawLine(1,v2,1,v0);
	}

	Vector2D check_size = chart->rect.GetSize();

	if(check_size.x > worldGlobals.lightmapSize || check_size.y > worldGlobals.lightmapSize)
		MsgWarning("Warning! Chart is still bigger than lightmap!\nSize: %fx%f\n", check_size.x, check_size.y);

	// if our object isn't small than 16x16 pixels
	// generate low resolution image for testing
	if(chart->iwide > 4 && chart->itall > 4)
	{
		// quarer size!
		chart->iwide_lo = (chart->iwide / 4);
		chart->itall_lo = (chart->itall / 4);

		chart->image_lo = (ubyte*)malloc( chart->iwide_lo * chart->itall_lo );
		memset(chart->image_lo, 0, chart->iwide_lo * chart->itall_lo);

		for(uint y = 0; y < chart->itall_lo; y++)
		{
			for(uint x = 0; x < chart->iwide_lo; x++)
			{
				int x_pos = x*4;
				int y_pos = y*4;

				chart->image_lo[y*chart->iwide_lo+x] = chart->image[y_pos*chart->iwide+x_pos];
			}
		}
	}

	ubyte* pNewImg = (ubyte*)malloc(chart->iwide*chart->itall);
	memcpy(pNewImg, chart->image, chart->iwide*chart->itall);
	
	// add extra padding distance to leave lighting errors
	// and compute painted area to sort
	for(uint y = 0; y < chart->itall; y++)
	{
		for(uint x = 0; x < chart->iwide; x++)
		{
			if(chart->image[y*chart->iwide+x] > 0)
			{
				chart->area++;
				ImagePutPixelsInThreshRadius(x,y,chart->iwide,chart->itall,pNewImg);
			}
		}
	}

	// compute paint area
	for(uint y = 0; y < chart->itall; y++)
	{
		for(uint x = 0; x < chart->iwide; x++)
		{
			if(pNewImg[y*chart->iwide+x] > 0)
				chart->paintarea++;
		}
	}

	free(chart->image);
	chart->image = pNewImg;
}

// rotates chart for minimal bounding box
float GetChartIdealAngle(lm_chart_t* pChart, Rectangle_t rect, Vector3D chart_center, cwsurface_t* pSurface)
{
#define NUM_STEPS	32
#define STEP_ANGLE	(360.0f/(float)NUM_STEPS)

	float best_angle = 0.0f;
	Rectangle_t best_rect = rect;

	bool found = false;

	for(int i = 0; i < NUM_STEPS; i++)
	{
		float fAngle = STEP_ANGLE * (float)i;

		Matrix2x2 rot = rotate2(DEG2RAD(fAngle));

		Rectangle_t tempRect;

		for(int j = 0; j < pChart->indices.numElem(); j++)
		{
			int src_index = pChart->indices[j];

			eqlevelvertexlm_t vertex( pSurface->pVerts[src_index] );
			vertex.texcoord.z = dot(LUXEL(vertex.position)-chart_center, normalize(pChart->tangent) /*+ dot(vertex.normal, charts[i]->tangent)*/);
			vertex.texcoord.w = dot(LUXEL(vertex.position)-chart_center, normalize(pChart->binormal) /*+ dot(vertex.normal, charts[i]->binormal)*/);

			// add verts to chart rectangle
			tempRect.AddVertex(rot*vertex.texcoord.zw());
		}

		Vector2D size = tempRect.GetSize();
		Vector2D bestsize = best_rect.GetSize();

		float fSize = size.x * size.y;
		float fBestSize = bestsize.x * bestsize.y;

		//Msg("(%g degrees) %g < %g ???\n", fAngle, fSizeA, fBestSize);

		// if new size is bigger than old, keep original
		if(fSize < fBestSize)
		{
			best_rect = tempRect;
			best_angle = fAngle;

			found = true;
		}
		else if(found)
			return best_angle;
	}

	return best_angle;
}
/*
// searches trinanle, returns index
bool CheckTriangleOverlapping(DkList<itriangle>* tris, const itriangle& tofind, eqlevelvertex_t* verts)
{
	for(int i = 0; i < tris->numElem(); i++)
	{
		Vector2D v0,v1,v2;

		v0 = verts[tris->ptr()[i].idxs[0]].position;
		v1 = verts[tris->ptr()[i].idxs[1]].position;
		v2 = verts[tris->ptr()[i].idxs[2]].position;

		if(IsTriangleIntersectsTriangle2D( tris->ptr()[i], tofind ))
			return true;
	}

	return false;
}*/

//------------------------------------------------------------------------------
// ChartMakeVertsAndCoords - generates new verts with coordinates for surface
//------------------------------------------------------------------------------
void ChartMakeVertsAndCoords(cwsurface_t* pSurface, DkList<lm_chart_t*>& charts, DkList<eqlevelvertexlm_t>& verts, DkList<int> &indices)
{
	// make new vertices, compute texture coordinates
	// and remap lm_chart_t triangles to the verts and indices array

	for(int i = 0; i < charts.numElem(); i++)
	{
		// discard empty
		if(charts[i]->indices.numElem() < 3)
		{
			charts[i]->used = false;
			continue;
		}

		//
		// THIS NEXT PHASE WILL BE MOVED TO MakeCharts, NAME IT GenChartVerts()
		//

		// get normal angle and compute two other vectors
		Vector3D n_angle = VectorAngles(charts[i]->normal);
		AngleVectors(n_angle, &charts[i]->normal, &charts[i]->tangent, &charts[i]->binormal);

		BoundingBox chartBBox;

		// build chart bounding box
		for(int j = 0; j < charts[i]->indices.numElem(); j++)
		{
			int src_index = charts[i]->indices[j];

			// bbox is lightmap-scaled
			chartBBox.AddVertex(LUXEL(pSurface->pVerts[src_index].position));
		}

		Vector3D chart_center = chartBBox.GetCenter();

		Rectangle_t rect;

		// compute 2D rectangle
		for(int j = 0; j < charts[i]->indices.numElem(); j++)
		{
			int src_index = charts[i]->indices[j];

			eqlevelvertexlm_t vertex( pSurface->pVerts[src_index] );
			vertex.texcoord.z = dot(LUXEL(vertex.position-chart_center), normalize(charts[i]->tangent)/* + dot(vertex.normal, charts[i]->tangent)*/);
			vertex.texcoord.w = dot(LUXEL(vertex.position-chart_center), normalize(charts[i]->binormal)/* + dot(vertex.normal, charts[i]->binormal)*/);

			// add verts to chart rectangle
			rect.AddVertex(vertex.texcoord.zw());

			// TODO: generate vertices and then check em all, make new charts from intersected, and recompute coords again
		}

		//
		// END OF MakeCharts MOVEMENT, PLEASE DO THIS IMPORTANT WORK
		//

		// compute ideal angle for this chart
		float chart_angle = GetChartIdealAngle(charts[i], rect, chart_center, pSurface);
		Matrix2x2 rot_matrix = rotate2(DEG2RAD(chart_angle));

		float fChartScaling = 1.0f;
		Vector2D rect_size = sign(rect.vrightBottom)*rect.vrightBottom + sign(rect.vleftTop)*rect.vleftTop;

		// Scale up this chart if it's size less than pixel
		// FIXME: this is a very weird code because it produces unlit luxels on smallest areas
		float fChartBoxPixels = rect_size.x * rect_size.y;

		if(fChartBoxPixels < 1.0f && fChartBoxPixels > 0) // less than one pixel
			fChartScaling *= (1.0f / fChartBoxPixels);

		// recompute size
		rect_size = Vector2D(fabs(rect_size.x)+worldGlobals.nPackThreshold*2,fabs(rect_size.y)+worldGlobals.nPackThreshold*2);

		// if chart size exceeds the lightmap size, decrease it's area
		// FIXME: weird code, may be minimal area testing?
		//
		if(rect_size.x > worldGlobals.lightmapSize || rect_size.y > worldGlobals.lightmapSize)
		{
			float maximal_size = 0;

			if(rect_size.x > rect_size.y)
				maximal_size = rect_size.x;
			else
				maximal_size = rect_size.y;

			fChartScaling *= (worldGlobals.lightmapSize / maximal_size)*0.85f;
		}

		Vector2D chart_center2d = rect.GetCenter();

		charts[i]->rect.Reset();

		DkList<eqlevelvertexlm_t> new_verts;
		new_verts.resize(charts[i]->indices.numElem());

		DkList<int> new_indices;
		new_indices.resize( charts[i]->indices.numElem() );

		// compute final lightmap coordinates
		for(int j = 0; j < charts[i]->indices.numElem(); j+=3)
		{
			int src_index0 = charts[i]->indices[j];
			int src_index1 = charts[i]->indices[j+1];
			int src_index2 = charts[i]->indices[j+2];

			ASSERT(!(src_index0 < 0 || src_index1 < 0 || src_index2 < 0));

#define ADD_LMUV_VERTEX(idx)												\
			eqlevelvertexlm_t vtx##idx##( pSurface->pVerts[src_index##idx##] );			\
			Vector2D lm_tc##idx##;												\
			lm_tc##idx##.x = dot( LUXEL( vtx##idx##.position - chart_center ), normalize( charts[i]->tangent )/* + dot(vertex.normal, charts[i]->tangent)*/) * fChartScaling;	\
			lm_tc##idx##.y = dot( LUXEL( vtx##idx##.position - chart_center ), normalize( charts[i]->binormal )/* + dot(vertex.normal, charts[i]->binormal)*/) * fChartScaling;	\
			lm_tc##idx## = rot_matrix*lm_tc##idx##;										\
			vtx##idx##.texcoord.z = lm_tc##idx##.x;										\
			vtx##idx##.texcoord.w = lm_tc##idx##.y;										\
			charts[i]->rect.AddVertex( lm_tc##idx## );									\

			ADD_LMUV_VERTEX(0);
			ADD_LMUV_VERTEX(1);
			ADD_LMUV_VERTEX(2);

			int idx[3] = {-1,-1,-1};
				
			// find any vertex
			for(int k = 0; k < new_verts.numElem(); k++)
			{
				if(idx[0] == -1 && (new_verts[k] == vtx0))
					idx[0] = k;
				else if(idx[1] == -1 && (new_verts[k] == vtx1))
					idx[1] = k;
				else if(idx[2] == -1 && (new_verts[k] == vtx2))
					idx[2] = k;

				if(idx[0] != -1 && idx[1] != -1 && idx[2] != -1)
					break;
			}
	
			if(idx[0] == -1)
				idx[0] = new_verts.append(vtx0);

			if(idx[1] == -1)
				idx[1] = new_verts.append(vtx1);

			if(idx[2] == -1)
				idx[2] = new_verts.append(vtx2);

			new_indices.append(idx[0]);
			new_indices.append(idx[1]);
			new_indices.append(idx[2]);
		}

		/*
		// now try subdivide chart if it has any overlapping verts;
		for(int j = 0; j < charts[i]->indices.numElem(); j+=3)
		{
			int idx0 = charts[i]->indices[j];
			int idx1 = charts[i]->indices[j+1];
			int idx2 = charts[i]->indices[j+2];


		}*/

		if(new_indices.numElem() < 3 || new_verts.numElem() == 0)
			ASSERT(!"No data generated.");

		// make image stamp, for packing
		ChartMakeImageStamp( charts[i], new_verts, new_indices );

		// finally, add
		charts[i]->indices.clear();
		int start_vertex = verts.numElem();

		for(int j = 0; j < new_indices.numElem(); j++)
			charts[i]->indices.append( new_indices[j] + start_vertex );

		verts.append(new_verts);
		indices.append( new_indices );
	}
}

//------------------------------------------------------------------------------
// ParametrizeSurfaceToLightmap - creates new temporary surface, and new verts
//------------------------------------------------------------------------------
void ParametrizeSurfaceToLightmap(int id)//cwsurface_t* pSurface, DkList<lm_chart_t*> &all_charts)
{
	if((g_surfaces[id]->flags & CS_FLAG_NODRAW) || (g_surfaces[id]->flags & CS_FLAG_OCCLUDER))
		return;

	g_surfaces[id]->surf_index = id;

	cwsurface_t* pSurface = g_surfaces[id];

	cwlitsurface_t* pDestSurf = new cwlitsurface_t;

	// this will be a final faces
	DkList<eqlevelvertexlm_t>	verts;
	DkList<int>					indices;

	DkList<lm_chart_t*>			charts;

	Vector3D cur_normal;

	int numTriangles = pSurface->numIndices / 3;

	// pick first triangle randomly
	int nTri = RandomInt( 0, numTriangles-1 )*3;
	int triIdx[3] = {pSurface->pIndices[nTri], pSurface->pIndices[nTri+1], pSurface->pIndices[nTri+2]};

	ComputeTriangleNormal( pSurface->pVerts[triIdx[0]].position, pSurface->pVerts[triIdx[1]].position, pSurface->pVerts[triIdx[2]].position, cur_normal);

	if(pSurface->flags & CS_FLAG_NOLIGHTMAP)
	{
		// just copy surface
		CopySurfaceDataToLit(pSurface, pDestSurf);

		ThreadLock();
		g_litsurfaces.append(pDestSurf);
		ThreadUnlock();

		return;
	}
	else
	{
		verts.resize( pSurface->numVertices );
		indices.resize( pSurface->numIndices );

		// make a first division (it also does next inside)
		MakeCharts(pSurface, charts, cur_normal, verts, indices);

		// also the charts needs to be sorted by size, but we need their verts
		ChartMakeVertsAndCoords(pSurface, charts, verts, indices);

		// pack the lightmap surfaces here, and assign the lightmap indices
	
		for(int i = 0; i < charts.numElem(); i++)
			charts[i]->pSurface = pDestSurf;
		
		pDestSurf->flags			= pSurface->flags;
		pDestSurf->bbox				= pSurface->bbox;
		pDestSurf->numIndices		= indices.numElem();
		pDestSurf->numVertices		= verts.numElem();

		pDestSurf->pMaterial		= pSurface->pMaterial;

		pDestSurf->pVerts = new eqlevelvertexlm_t[ pDestSurf->numVertices ];
		pDestSurf->pIndices = new int[ pDestSurf->numIndices ];

		memcpy( pDestSurf->pVerts, verts.ptr(), verts.numElem() * sizeof(eqlevelvertexlm_t) );
		memcpy( pDestSurf->pIndices, indices.ptr(), indices.numElem() * sizeof(int) );

		ThreadLock();
		g_litsurfaces.append( pDestSurf );
		ThreadUnlock();
	}

	ThreadLock();
	g_usedcharts.append(charts);
	ThreadUnlock();
}

ubyte*	g_pCurrentLightmap = NULL;
ubyte*	g_pSrcImage = NULL;
int		g_wide = 0;
int		g_tall = 0;
int		g_x_pos = 0;
int		g_y_pos = 0;

void Threaded_PutImageLine(int y)
{
	for(uint x = 0; x < g_wide; x++)
	{
		if(g_pSrcImage[y * g_wide + x] == 0)
			continue;

		uint16 color = (g_pCurrentLightmap[(y+g_y_pos) * worldGlobals.lightmapSize + (x+g_x_pos)] + g_pSrcImage[y * g_wide + x]);

		if(color > 255)
			color = 255;

		g_pCurrentLightmap[(y+g_y_pos) * worldGlobals.lightmapSize + (x+g_x_pos)] = (ubyte)color;
	}
}

//------------------------------------------------------------------------------
// PutImage - additively puts image on current lightmap surface
//------------------------------------------------------------------------------
void PutImage(ubyte* pCurrentLightmap, uint& x_pos, uint& y_pos, ubyte* pSrc, uint wide, uint tall, uint map_size)
{
	for(uint y = 0; y < tall; y++)
	{
		for(uint x = 0; x < wide; x++)
		{
			if(pSrc[y * wide + x] == 0)
				continue;

			uint16 color = (pCurrentLightmap[(y+y_pos) * map_size + (x+x_pos)] + pSrc[y * wide + x]);

			if(color > 255)
				color = 255;

			pCurrentLightmap[(y+y_pos) * map_size + (x+x_pos)] = (ubyte)color;
		}
	}
}

int	g_nMovementX = 0;

void Threaded_TestPutImageLine(int x)
{
	bool bMove = false;

	for(uint y = 0; y < g_tall; y++)
	{
		ubyte color = (g_pCurrentLightmap[(y+g_y_pos) * worldGlobals.lightmapSize + (x+g_x_pos)] + g_pSrcImage[y * g_wide + x]);

		if(color > 1)
		{
			bMove = true;
			break;
		}
	}

	if(bMove)
		g_nMovementX++;
}


//------------------------------------------------------------------------------
// TestImagePut - tests image placement and computes movement factor
//------------------------------------------------------------------------------

bool IsChartIntersectsWithOthers(lm_chart_t* pChart, uint& x_pos, uint& y_pos)
{
	// additional check
	for(int i = 0; i < g_usedcharts.numElem(); i++)
	{
		if(g_usedcharts[i] == pChart)
			continue;

		if(g_usedcharts[i]->lm_index == -1)
			continue;

		if( IsChartIntersectsChart(pChart, g_usedcharts[i], Vector2D(x_pos,y_pos), Vector2D(g_usedcharts[i]->x,g_usedcharts[i]->y)))
			return true;
	}

	return false;
}

bool TestImagePut(ubyte* pCurrentLightmap, uint& x_pos, uint& y_pos, lm_chart_t* pChart, uint lmSize)
{
	// TODO: use quadtree of current lightmap

	uint movement_x = 0;
	
	// horizontal-packing scheme
	for(uint x = 0; x < pChart->iwide; x+=worldGlobals.nPackThreshold)
	{
		bool bMove = false;
		for(uint y = 0; y < pChart->itall; y+=worldGlobals.nPackThreshold)
		{
			ubyte color = (pCurrentLightmap[(y+y_pos) * lmSize + (x+x_pos)] + pChart->image[y * pChart->iwide + x]);

			if( color > 1 )
			{
				bMove = true;
				break;
			}
		}

		if(bMove)
			movement_x += worldGlobals.nPackThreshold;
	}

	if(movement_x > 0)
	{	
		x_pos += movement_x;
		return false;
	}
	
	// test success, space is free
	return true;//!IsChartIntersectsWithOthers(pChart, x_pos, y_pos);
}

//------------------------------------------------------------------------------
// TestChartPut - test chart for intersection with other charts
//------------------------------------------------------------------------------

bool TestChartPut(lm_chart_t* pChart, uint& x_pos, uint& y_pos)
{
	uint movement_x = 0;

	for(int i = 0; i < g_usedcharts.numElem(); i++)
	{
		if(g_usedcharts[i] == pChart)
			continue;

		if(g_usedcharts[i]->lm_index == -1)
			continue;

		if( IsChartIntersectsChart(pChart, g_usedcharts[i], Vector2D(x_pos,y_pos), Vector2D(g_usedcharts[i]->x,g_usedcharts[i]->y)))
			movement_x += worldGlobals.nPackThreshold;
		else
			break;
	}

	if(movement_x > 0)
	{	
		x_pos += movement_x;
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// TestAndPackChart - Tests chart on image in all possible positions and packs
// FIXME: DAMN TOO SLOW!!! Do some threaded processing?
//------------------------------------------------------------------------------
void TestAndPackChart(int id)
{
	lm_chart_t *chart = g_usedcharts[id];

	ubyte*	pCurrentLightmap = NULL;	// current lightmap
	ubyte*	pCurrentLoLightmap = NULL;
	cwlightmap_t* pLightmap = NULL;

	// compute image movement count using the bounds
	uint move_count_x = worldGlobals.lightmapSize - chart->iwide - 1;
	uint move_count_y = worldGlobals.lightmapSize - chart->itall - 1;

	// set chart lightmap index
	chart->lm_index = 0;

	int lmQuarterSize = worldGlobals.lightmapSize / 4;

re_search_place:

	// select or create lightmap
	if(!pCurrentLightmap)
	{
		if( chart->lm_index < g_pLightmaps.numElem() )
		{
			pLightmap = g_pLightmaps[chart->lm_index];
			pCurrentLightmap = g_pLightmaps[chart->lm_index]->lightmap_pack_image;
			pCurrentLoLightmap = g_pLightmaps[chart->lm_index]->lightmap_pack_image_lo;

			// copy lightmap image to lores
			for(uint qy = 0; qy < lmQuarterSize; qy++)
			{
				for(uint qx = 0; qx < lmQuarterSize; qx++)
				{
					int x_pos = qx*4;
					int y_pos = qy*4;

					pCurrentLoLightmap[qy*lmQuarterSize+qx] += pCurrentLightmap[y_pos*worldGlobals.lightmapSize+x_pos];
				}
			}
		}
		else
		{
			pLightmap = AllocLightmap();
			pCurrentLightmap = pLightmap->lightmap_pack_image;
			pCurrentLoLightmap = pLightmap->lightmap_pack_image_lo;
		}
	}

	//
	// switch to next lightmap if we doesn't have enough free space
	//
	if(pLightmap->used_area + chart->paintarea > worldGlobals.lightmapSize * worldGlobals.lightmapSize)
	{
		chart->lm_index++;
		pCurrentLightmap = NULL;
		goto re_search_place;
	}

	uint startPosX = 0;
	uint startPosY = 0;

	//ASSERT(!"ATTACH");

	bool bDetected = false;

	/*
	// detect ideal position for putting image
	if(chart->image_lo)
	{
		// compute image movement count using the bounds
		uint lmove_count_x = lmQuarterSize - chart->iwide_lo - 1;
		uint lmove_count_y = lmQuarterSize - chart->itall_lo - 1;

		for(uint y = 0; y < lmove_count_y; y++)
		{
			for(uint x = 0; x < lmove_count_x; x++)
			{
				if( TestImagePut(pCurrentLoLightmap, x, y, chart, lmQuarterSize) )
				{
					bDetected = true;
					startPosX = x*4;
					startPosY = y*4;
					break;
				}
			}

			if(bDetected)
				break;
		}
	}
	*/

	/*
	if(startPosX+chart->iwide > worldGlobals.lightmapSize)
		startPosX = worldGlobals.lightmapSize - chart->iwide - 2;
	
	if(startPosY+chart->itall > worldGlobals.lightmapSize)
		startPosY = worldGlobals.lightmapSize - chart->itall - 2;
		*/

	// 
	// FIXME: This is bruteforce, that exactly means word "SLOW", use octree or other algorhitms to find intersection/overlapping
	//
	for(uint y = startPosY; y < move_count_y; y++)
	{
		//uint x = startPosX;
		for(uint x = startPosX; x < move_count_x; x++)
		{
			// test image and movement
			if( TestImagePut(pCurrentLightmap, x, y, chart, worldGlobals.lightmapSize) )
			{
				// put image
				PutImage(pCurrentLightmap, x, y, chart->image, chart->iwide, chart->itall, worldGlobals.lightmapSize);

				pLightmap->used_area += chart->paintarea;

				pLightmap->last_x = x;
				pLightmap->last_y = y;

				// assign coords
				chart->x = x;
				chart->y = y;

				uint qx = x/4;
				uint qy = y/4;

				return; // bail out
			}
		}
	}

	// no place found for chart?
	// try put into next lightmap

	chart->lm_index++;
	pCurrentLightmap = NULL;

	goto re_search_place;
}

int chart_size_sort_fn( lm_chart_t* const &a, lm_chart_t* const &b )
{
	// TODO: per-surface optimization

	return (b->area - a->area);
}

struct surface_charts_t
{
	cwlitsurface_t*		surface;
	DkList<lm_chart_t*>	surf_charts;
	int					lm_index;
};

//------------------------------------------------------------------------------
// AddSurfChartsList - generates assignment of charts to surfaces with lm_index
//------------------------------------------------------------------------------
void AddSurfChartsList(lm_chart_t* pChart, DkList<surface_charts_t> &surfcharts)
{
	surface_charts_t* pSurfChart = NULL;

	for(int i = 0; i < surfcharts.numElem(); i++)
	{
		if(surfcharts[i].surface == pChart->pSurface && surfcharts[i].lm_index == pChart->lm_index)
		{
			pSurfChart = &surfcharts[i];
			break;
		}
	}

	if(!pSurfChart)
	{
		surface_charts_t newsurfcharts;
		newsurfcharts.surface = pChart->pSurface;
		newsurfcharts.lm_index = pChart->lm_index;

		int id = surfcharts.append(newsurfcharts);
		pSurfChart = &surfcharts[id];
	}

	pSurfChart->surf_charts.append(pChart);
}

//------------------------------------------------------------------------------
// GenerateSurfFromSurfChartList - generates new surfaces with lightmap indices
//------------------------------------------------------------------------------
void GenerateSurfFromSurfChartList(surface_charts_t &surfcharts)
{
	DkList<eqlevelvertexlm_t>	verts;
	DkList<int>					indices;

	verts.resize(surfcharts.surface->numVertices);
	indices.resize(surfcharts.surface->numVertices);

	// create new lit surface
	cwlitsurface_t* pSurf = new cwlitsurface_t;
	*pSurf = *surfcharts.surface;

	pSurf->lightmap_id = surfcharts.lm_index;

	float inv_lightmap_size = 1.0f / (float)worldGlobals.lightmapSize;
	
	for(int i = 0; i < surfcharts.surf_charts.numElem(); i++)
	{
		lm_chart_t* pChart = surfcharts.surf_charts[i];

		Vector2D offset = (-pChart->rect.vleftTop) + Vector2D( worldGlobals.nPackThreshold );
		
		for(int j = 0; j < pChart->indices.numElem(); j++)
		{
			int src_idx = pChart->indices[j];

			eqlevelvertexlm_t vert = pSurf->pVerts[src_idx];

			vert.texcoord.z += pChart->x + offset.x;
			vert.texcoord.w += pChart->y + offset.y;

			vert.texcoord.z *= inv_lightmap_size;
			vert.texcoord.w *= inv_lightmap_size;

			int new_idx = verts.addUnique(vert);
			indices.append(new_idx);
		}
	}

	pSurf->numVertices = verts.numElem();
	pSurf->numIndices = indices.numElem();

	pSurf->pVerts = new eqlevelvertexlm_t[pSurf->numVertices];
	pSurf->pIndices = new int[pSurf->numIndices];

	memcpy(pSurf->pVerts, verts.ptr(), sizeof(eqlevelvertexlm_t)*verts.numElem());
	memcpy(pSurf->pIndices, indices.ptr(), sizeof(int)*indices.numElem());

	ComputeSurfaceBBOX(pSurf);

	g_litsurfaces.append(pSurf);
}

//------------------------------------------------------------------------------
// ParametrizeLightmapUVs - main function to parametrization and UV pack
//------------------------------------------------------------------------------
void ParametrizeLightmapUVs()
{
	EqString procfile_path(varargs("worlds/%s/eqwc_litgeometry.proc", worldGlobals.worldName.GetData()));

	// if procfile with lightmaps is exist, load it
	if( g_fileSystem->FileExist(procfile_path.GetData(), SP_MOD) )
	{
		IFile* pLoadStream = g_fileSystem->Open( procfile_path.GetData(), "rb" );

		int nCount = 0;

		// write surface count
		pLoadStream->Read( &nCount, 1, sizeof(int) );

		Msg("PROC FILE: Surface count is %d\n", nCount);

		for(int i = 0; i < nCount; i++)
		{
			cwlitsurface_t* pSurface = LoadLitSurface( pLoadStream );

			if(!pSurface)
			{
				break;
			}

			//realSurfs++;

			if(pSurface->lightmap_id+1 > worldGlobals.numLightmaps)
				worldGlobals.numLightmaps = pSurface->lightmap_id+1;

			ComputeSurfaceBBOX( pSurface );

			worldGlobals.worldBox.Merge(pSurface->bbox);

			g_litsurfaces.append( pSurface );
		}

		//Msg("PROC FILE: REAL Surface count is %d\n", realSurfs);

		Msg("Lightmaps retrieved: %d\n", worldGlobals.numLightmaps);

		g_fileSystem->Close(pLoadStream);

		return;
	}

	if(worldGlobals.bNoLightmap)
	{
		IFile* pSaveStream = g_fileSystem->Open( procfile_path.GetData(), "wb");

		int nCount = g_surfaces.numElem();

		// write surface count
		pSaveStream->Write(&nCount, 1, sizeof(int));

		// just convert surfaces to lit
		for(int i = 0; i < g_surfaces.numElem(); i++)
		{
			cwlitsurface_t* pDestSurf = new cwlitsurface_t;

			CopySurfaceDataToLit( g_surfaces[i], pDestSurf );
			
			g_litsurfaces.append( pDestSurf );

			// save our backup file
			SaveLitSurface( pDestSurf, pSaveStream );
		}

		g_fileSystem->Close(pSaveStream);

		return;
	}
	
	StartPacifier("ParametrizeLightmapUVs: ");
	RunThreadsOnIndividual( g_surfaces.numElem(), true, ParametrizeSurfaceToLightmap );
	EndPacifier();

	// sort charts by size
	g_usedcharts.sort( chart_size_sort_fn );

	int start = Platform_GetCurrentTime();

	/*
	OpenCLInit();

	if(g_cl_initialized)
	{
		StartPacifier("LightmapUVAtlasPack_CL: ");
		// then pack all charts TODO: some threaded processing?
		for(int i = 0; i < g_usedcharts.numElem(); i++)
		{
			UpdatePacifier((float)i / (float)g_usedcharts.numElem());
			//TestAndPackChartCL(i);
		}
		EndPacifier();

		OpenCLRelease();
	}
	else
	{*/
		StartPacifier("LightmapUVAtlasPack: ");
		// then pack all charts
		for(int i = 0; i < g_usedcharts.numElem(); i++)
		{
			UpdatePacifier((float)i / (float)g_usedcharts.numElem());
			TestAndPackChart(i);
		}
		EndPacifier();
	//}
	
	int end = Platform_GetCurrentTime();
	Msg(" (%i)\n", end - start);
	

	DkList<surface_charts_t> surfcharts;

	StartPacifier("LightmapIndexAssign: ");
	for(int i = 0; i < g_usedcharts.numElem(); i++)
	{
		UpdatePacifier((float)i*0.5f / (float)g_usedcharts.numElem());
		AddSurfChartsList( g_usedcharts[i], surfcharts );
	}

	DkList<cwlitsurface_t*> lit_surfs_orig;
	lit_surfs_orig.append(g_litsurfaces);
	g_litsurfaces.clear();

	for(int i = 0; i < surfcharts.numElem(); i++)
	{
		UpdatePacifier(((float)i*0.5f / (float)surfcharts.numElem()) + 0.5f);
		GenerateSurfFromSurfChartList( surfcharts[i] );
	}

	for(int i = 0; i < lit_surfs_orig.numElem(); i++)
	{
		if(lit_surfs_orig[i]->flags & CS_FLAG_NOLIGHTMAP)
			g_litsurfaces.append(lit_surfs_orig[i]);
		else
			FreeLitSurface(lit_surfs_orig[i]);
	}

	EndPacifier();

	worldGlobals.numLightmaps = g_pLightmaps.numElem();

	// save surfaces
	IFile* pSaveStream = g_fileSystem->Open( procfile_path.GetData(), "wb");
		
	int nCount = g_litsurfaces.numElem();

	// write surface count
	pSaveStream->Write(&nCount, 1, sizeof(int));

	for(int i = 0; i < g_litsurfaces.numElem(); i++)
		SaveLitSurface( g_litsurfaces[i], pSaveStream );

	g_fileSystem->Close(pSaveStream);

	// free unused data
	for(int i = 0; i < g_pLightmaps.numElem(); i++)
	{
		// DEBUG CODE
		/*
		g_fileSystem->MakeDir("EQWC_LightmapPack", SP_ROOT);
		
		int qSize = worldGlobals.lightmapSize*4;	// mega size

		ubyte* pBigImage = (ubyte*)malloc(qSize*qSize);
		memset(pBigImage, 0, sizeof(qSize*qSize));
		
		// rasterize our stamp on new image
		CRasterizer<ubyte> raster;
		raster.SetImage(pBigImage, qSize, qSize);
		raster.SetAdditiveColor( true );
		raster.Clear( 0 );

		for(int j = 0; j < g_litsurfaces.numElem(); j++)
		{
			if(g_litsurfaces[j]->lightmap_id != i)
				continue;

			for(int k = 0; k < g_litsurfaces[j]->numIndices; k+=3)
			{
				int src_idx0 = g_litsurfaces[j]->pIndices[k];
				int src_idx1 = g_litsurfaces[j]->pIndices[k+1];
				int src_idx2 = g_litsurfaces[j]->pIndices[k+2];

				Vector2D v0 = Vector2D(g_litsurfaces[j]->pVerts[src_idx0].texcoord.z,g_litsurfaces[j]->pVerts[src_idx0].texcoord.w)*qSize;
				Vector2D v1 = Vector2D(g_litsurfaces[j]->pVerts[src_idx1].texcoord.z,g_litsurfaces[j]->pVerts[src_idx1].texcoord.w)*qSize;
				Vector2D v2 = Vector2D(g_litsurfaces[j]->pVerts[src_idx2].texcoord.z,g_litsurfaces[j]->pVerts[src_idx2].texcoord.w)*qSize;

				raster.DrawTriangle( 5, v0, 5, v1, 5, v2 );
			}
		}

		// currently painted
		IFile* pFile = g_fileSystem->Open(varargs("EQWC_LightmapPack/lm_%d_check%d.raw", i, qSize), "wb", SP_ROOT);
		pFile->Write( pBigImage, 1, qSize*qSize);
		g_fileSystem->Close(pFile);

		free(pBigImage);

		// original
		pFile = g_fileSystem->Open(varargs("EQWC_LightmapPack/lm_%d_orig%d.raw", i, worldGlobals.lightmapSize), "wb", SP_ROOT);
		pFile->Write(g_pLightmaps[i]->lightmap_pack_image, 1, worldGlobals.lightmapSize*worldGlobals.lightmapSize);
		g_fileSystem->Close(pFile);
		*/
		// END OF DEBUG CODE

		Msg("lightmap %d used area: %d (%.2f percent)\n", i, g_pLightmaps[i]->used_area, ((float)g_pLightmaps[i]->used_area / (float)(worldGlobals.lightmapSize* worldGlobals.lightmapSize))*100.0f);

		FreeLightmap(g_pLightmaps[i]);
	}
	
	g_pLightmaps.clear();

	// free all charts
	FreeAllCharts();
}