//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Brush processing with CSG
//////////////////////////////////////////////////////////////////////////////////

#include "eqwc.h"
#include "Utils/GeomTools.h"

DkList<brushwinding_t*> g_windings;
cwbrush_t*				g_pBrushList;
int						g_numBrushes;

void FreeBrush(cwbrush_t* brush)
{
	delete brush->faces;
	delete brush;
}

void FreeBrushList(cwbrush_t* brush)
{
	cwbrush_t* pBrush = brush;
	while(pBrush)
	{
		cwbrush_t* pTmp = pBrush;

		pBrush = pBrush->next;

		FreeBrush(pTmp);
	}
}

void CopyBrushFaceTo(cwbrushface_t* pDest, cwbrushface_t* pSrc)
{
	pDest->brush = pSrc->brush;
	pDest->flags = pSrc->flags;
	pDest->nSmoothingGroup = pSrc->nSmoothingGroup;
	pDest->Plane = pSrc->Plane;
	pDest->planenum = pSrc->planenum;
	pDest->pMaterial = pSrc->pMaterial;
	pDest->UAxis = pSrc->UAxis;
	pDest->VAxis = pSrc->VAxis;
	pDest->visibleHull = CopyWinding(pSrc->visibleHull);
	pDest->vScale = pSrc->vScale;
	pDest->winding = CopyWinding(pSrc->winding);
}

cwbrush_t* CopyBrush(cwbrush_t* pBrush)
{
	cwbrush_t* pCopy = new cwbrush_t();
	pCopy->flags = pBrush->flags;
	pCopy->numfaces = pBrush->numfaces;

	pCopy->faces = new cwbrushface_t[pBrush->numfaces];
	for(int i = 0; i < pBrush->numfaces; i++)
	{
		CopyBrushFaceTo(&pCopy->faces[i], &pBrush->faces[i]);

		pCopy->faces[i].brush = pCopy;
	}

	return pCopy;
}

cwbrush_t* CopyBrushList(cwbrush_t* pOthers)
{
	cwbrush_t* pBrush = pOthers;

	if(!pBrush)
		return NULL;

	cwbrush_t* pNewList = CopyBrush(pBrush);
	cwbrush_t** pNewListBrush = &pNewList->next;

	pBrush = pBrush->next;

	while(pBrush)
	{
		(*pNewListBrush) = CopyBrush(pBrush);
		pNewListBrush = &(*pNewListBrush)->next;

		pBrush = pBrush->next;
	}

	return pNewList;
}

brushwinding_t* AllocWinding()
{
	brushwinding_t* w = new brushwinding_t;
	w->numVerts = 0;

	g_windings.append(w);

	return w;
}

brushwinding_t* CopyWinding(const brushwinding_t* src)
{
	if(!src)
		return NULL;

	brushwinding_t* w = AllocWinding();
	w->face = src->face;
	w->numVerts = src->numVerts;

	memcpy(w->vertices, src->vertices, sizeof(eqlevelvertex_t) * w->numVerts);

	return w;
}

void FreeWinding(brushwinding_t* w)
{
	g_windings.remove(w);
	delete w;
}

bool WindingIsValid(brushwinding_t* pWinding)
{
	if(pWinding->numVerts < 3 || pWinding->numVerts > MAX_POINTS_ON_WINDING)
		return false;

	return true;
}

// clips vertices against plane
int ClipVertsAgainstPlane(Plane &plane, int vertexCount, eqlevelvertex_t *vertex, eqlevelvertex_t *newVertex)
{
	bool negative[MAX_POINTS_ON_WINDING*16];

	float t;

	// classify vertices
	int negativeCount = 0;
	for (int i = 0; i < vertexCount; i++)
	{
		negative[i] = (plane.ClassifyPoint(vertex[i].position) != CP_FRONT);
		negativeCount += negative[i];
	}
	
	if (negativeCount == vertexCount)
		return 0; // completely culled, we have no polygon

	if( negativeCount == 0 )
		return -1;

	int count = 0;

	for (int i = 0; i < vertexCount; i++)
	{
		// prev is the index of the previous vertex
		int	prev = (i != 0) ? i - 1 : vertexCount - 1;
		
		if (negative[i])
		{
			if (!negative[prev])
			{
				eqlevelvertex_t v1 = vertex[prev];
				eqlevelvertex_t v2 = vertex[i];

				// Current vertex is on negative side of plane,
				// but previous vertex is on positive side.
				Vector3D edge = v1.position - v2.position;

				// compute interpolation factor
				t = plane.Distance(v1.position) / dot(plane.normal, edge);

				newVertex[count] = InterpolateLevelVertex(v1,v2,t);
				count++;
			}
		}
		else
		{
			if (negative[prev])
			{
				eqlevelvertex_t v1 = vertex[i];
				eqlevelvertex_t v2 = vertex[prev];

				// Current vertex is on positive side of plane,
				// but previous vertex is on negative side.
				Vector3D edge = v1.position - v2.position;

				// compute interpolation factor
				t = plane.Distance(v1.position) / dot(plane.normal, edge);

				newVertex[count] = InterpolateLevelVertex(v1,v2,t);
				
				count++;
			}
			
			// include current vertex
			newVertex[count] = vertex[i];
			count++;
		}
	}
	
	// return new clipped vertex count
	return count;
}

// chops winding
void ChopWinding(brushwinding_t *w, Plane &plane)
{
	eqlevelvertex_t newVertices[MAX_POINTS_ON_WINDING];
	eqlevelvertex_t vertices[MAX_POINTS_ON_WINDING];

	int nVerts = w->numVerts;

	for(int i = 0; i < nVerts; i++)
		vertices[i] = w->vertices[i];

	int nNewVerts = ClipVertsAgainstPlane(plane, nVerts, vertices, newVertices);
	if(nNewVerts == -1)
		return;

	w->numVerts = 0;

	for(int i = 0; i < nNewVerts; i++)
		w->vertices[w->numVerts++] = newVertices[i];
}

// splits the face by this face, and results a
void Split(const brushwinding_t *w, Plane& plane, brushwinding_t **front, brushwinding_t **back )
{
	ClassifyPlane_e	*pCP = (ClassifyPlane_e*)malloc(w->numVerts*sizeof(ClassifyPlane_e));

	int	counts[3];
	counts[0] = counts[1] = counts[2] = 0;

	*front = *back = NULL;

	for ( int i = 0; i < w->numVerts; i++ )
	{
		ClassifyPlane_e plane_class = plane.ClassifyPointEpsilon ( w->vertices[i].position, 0.2f);

		pCP[i] = plane_class;

		counts[plane_class]++;
	}

	/*
	// if nothing at the front of the clipping plane
	if ( !counts[CP_FRONT] )
	{
		*back = CopyWinding(w);
		return;
	}

	// if nothing at the back of the clipping plane
	if ( !counts[CP_BACK] )
	{
		*front = CopyWinding(w);
		return;
	}
	*/

	brushwinding_t* pFront = AllocWinding();
	brushwinding_t* pBack = AllocWinding();

	pFront->face = w->face;
	pBack->face	= w->face;

	for ( int i = 0; i < w->numVerts; i++ )
	{
		//
		// Add point to appropriate list
		//
		switch (pCP[i])
		{
			case CP_FRONT:
			{
				pFront->vertices[pFront->numVerts] = w->vertices[i];
				pFront->numVerts++;
				break;
			}
			case CP_BACK:
			{
				pBack->vertices[pBack->numVerts] = w->vertices[i];
				pBack->numVerts++;
				break;
			}
			case CP_ONPLANE:
			{
				pFront->vertices[pFront->numVerts] = w->vertices[i];
				pBack->vertices[pBack->numVerts] = w->vertices[i];
				pFront->numVerts++;
				pBack->numVerts++;
			}
			break;
		}

		//
		// Check if edges should be split
		//
		int		iNext	= i + 1;
		bool	bIgnore	= false;

		if ( i == pBack->numVerts-1)
			iNext = 0;

		if((pCP[i] == CP_ONPLANE) && (pCP[iNext] != CP_ONPLANE))
			bIgnore = true;

		else if((pCP[iNext] == CP_ONPLANE) && ( pCP[i] != CP_ONPLANE))
			bIgnore = true;

		if (!bIgnore && (pCP[i] != pCP[iNext]))
		{
			eqlevelvertex_t	v;	// New vertex created by splitting
			float p;			// Percentage between the two points

			plane.GetIntersectionLineFraction(w->vertices[i].position, w->vertices[iNext].position, v.position, p );

			v.texcoord = lerp(w->vertices[i].texcoord, w->vertices[iNext].texcoord, p);

			pFront->vertices[pFront->numVerts] = v;
			pBack->vertices[pBack->numVerts] = v;
			pFront->numVerts++;
			pBack->numVerts++;
		}
	}

	if(pFront->numVerts > 2)
		*front = pFront;
	else
		FreeWinding(pFront);

	if(pBack->numVerts > 2)
		*back = pBack;
	else
		FreeWinding(pBack);
}

// makes infinite winding
void MakeInfiniteWinding(brushwinding_t *w, Plane &plane)
{
	// compute needed vectors
	Vector3D vRight, vUp;
	VectorVectors(plane.normal, vRight, vUp);

	// make vectors infinite
	vRight *= MAX_COORD_UNITS;
	vUp *= MAX_COORD_UNITS;

	Vector3D org = -plane.normal * plane.offset;

	// and finally fill really big winding
	w->vertices[0] = eqlevelvertex_t(org - vRight + vUp, vec2_zero, plane.normal);
	w->vertices[1] = eqlevelvertex_t(org + vRight + vUp, vec2_zero, plane.normal);
	w->vertices[2] = eqlevelvertex_t(org + vRight - vUp, vec2_zero, plane.normal);
	w->vertices[3] = eqlevelvertex_t(org - vRight - vUp, vec2_zero, plane.normal);

	w->numVerts = 4;
}

void MakeWindingRenderStuffFromFace(brushwinding_t *w, cwbrushface_t* face)
{
	// assign face
	w->face = face;

	w->face->pMaterial->GetShader()->InitParams();

	int texSizeW = 1;
	int texSizeH = 1;

	ITexture* pTex = w->face->pMaterial->GetShader()->GetBaseTexture();

	if(pTex)
	{
		texSizeW = pTex->GetWidth();
		texSizeH = pTex->GetHeight();
	}
	
	for(int i = 0; i < w->numVerts; i++ )
	{
		float U, V;
		
		U = dot(w->face->UAxis.normal, w->vertices[i].position);
		U /= ( ( float )texSizeW ) * w->face->vScale.x;
		U += ( w->face->UAxis.offset / ( float )texSizeW );

		V = dot(w->face->VAxis.normal, w->vertices[i].position);
		V /= ( ( float )texSizeH ) * w->face->vScale.y;
		V += ( w->face->VAxis.offset / ( float )texSizeH );

		w->vertices[i].texcoord.x = U;
		w->vertices[i].texcoord.y = V;

		w->vertices[i].tangent = vec3_zero;
		w->vertices[i].binormal = vec3_zero;
		w->vertices[i].normal = vec3_zero;
	}
	

	int num_triangles = ((w->numVerts < 4) ? 1 : (2 + w->numVerts - 4));

	int i = 0;

	// compute tangent space, texture coordinates
	for(i = 0; i < num_triangles; i++)
	{
		int idx0 = 0;
		int idx1 = i+1;
		int idx2 = i+2;

		Vector3D t,b,n;
		ComputeTriangleTBN(	w->vertices[idx0].position, w->vertices[idx1].position, w->vertices[idx2].position,
							w->vertices[idx0].texcoord, w->vertices[idx1].texcoord, w->vertices[idx2].texcoord, 
							n,t,b);

		w->vertices[idx0].tangent += t;
		w->vertices[idx0].binormal += b;
		w->vertices[idx0].normal += n;

		w->vertices[idx1].tangent += t;
		w->vertices[idx1].binormal += b;
		w->vertices[idx1].normal += n;

		w->vertices[idx2].tangent += t;
		w->vertices[idx2].binormal += b;
		w->vertices[idx2].normal += n;
	}

	for(i = 0; i < w->numVerts; i++)
	{
		w->vertices[i].tangent = normalize( w->vertices[i].tangent );
		w->vertices[i].binormal = normalize( w->vertices[i].binormal );
		w->vertices[i].normal = normalize( w->vertices[i].normal );
	}
}

void MakeSurfaceFromBrushFace(cwbrushface_t* face)
{
	cwsurface_t* pSurf = new cwsurface_t;

	pSurf->type = SURFTYPE_STATIC;

	pSurf->surfflags = 0; // there is no tesselation
	pSurf->flags = face->flags;	// assign compile flags
	pSurf->nSmoothingGroup = face->nSmoothingGroup;
	pSurf->nTesselation = 0;

	int num_triangles = ((face->winding->numVerts < 4) ? 1 : (2 + face->winding->numVerts - 4));

	pSurf->numVertices = face->winding->numVerts;
	pSurf->numIndices = num_triangles*3;

	pSurf->pMaterial = face->pMaterial;

	pSurf->pVerts = new eqlevelvertex_t[pSurf->numVertices];
	memcpy(pSurf->pVerts,face->winding->vertices,pSurf->numVertices*sizeof(eqlevelvertex_t));

	pSurf->pIndices = new int[pSurf->numIndices];

	int nIndex = 0;

	for(int i = 0; i < num_triangles; i++ )
	{
		int idx0 = 0;
		int idx1 = i+1;
		int idx2 = i+2;

		pSurf->pIndices[nIndex++] = idx0;
		pSurf->pIndices[nIndex++] = idx1;
		pSurf->pIndices[nIndex++] = idx2;
	}

	if(pSurf->flags & CS_FLAG_OCCLUDER)
		g_occlusionsurfaces.append(pSurf);
	else
	{
		MergeSurfaceIntoList(pSurf, g_surfaces, true);
		FreeSurface(pSurf);
		//g_surfaces.append(pSurf);
	}
}

void MakeSurfaceFromWinding(brushwinding_t* winding)
{
	cwsurface_t* pSurf = new cwsurface_t;

	pSurf->type = SURFTYPE_STATIC;

	pSurf->surfflags = 0; // there is no tesselation
	pSurf->flags = winding->face->flags;	// assign compile flags
	pSurf->nSmoothingGroup = winding->face->nSmoothingGroup;
	pSurf->nTesselation = 0;

	int num_triangles = ((winding->numVerts < 4) ? 1 : (2 + winding->numVerts - 4));

	pSurf->numVertices = winding->numVerts;
	pSurf->numIndices = num_triangles*3;

	pSurf->pMaterial = winding->face->pMaterial;

	pSurf->pVerts = new eqlevelvertex_t[pSurf->numVertices];
	memcpy(pSurf->pVerts, winding->vertices, pSurf->numVertices*sizeof(eqlevelvertex_t));

	pSurf->pIndices = new int[pSurf->numIndices];

	int nIndex = 0;

	for(int i = 0; i < num_triangles; i++ )
	{
		int idx0 = 0;
		int idx1 = i+1;
		int idx2 = i+2;

		pSurf->pIndices[nIndex++] = idx0;
		pSurf->pIndices[nIndex++] = idx1;
		pSurf->pIndices[nIndex++] = idx2;
	}

	if(pSurf->flags & CS_FLAG_OCCLUDER)
		g_occlusionsurfaces.append(pSurf);
	else
	{
		MergeSurfaceIntoList(pSurf, g_surfaces, true);

		FreeSurface(pSurf);
		//g_surfaces.append(pSurf);
	}
}

void WeldWindingVertices(brushwinding_t* a, brushwinding_t* b)
{
	// weld by distance and check normal angle for smoothing
	for(int i = 0; i < a->numVerts; i++)
	{
		for(int j = 0; j < b->numVerts; j++)
		{
			if(length(b->vertices[j].position - a->vertices[i].position) < WINDING_WELD_DIST)
			{
				Vector3D final_point = (a->vertices[i].position + b->vertices[j].position) * 0.5f;
				a->vertices[i].position = final_point;
				b->vertices[j].position = final_point;

				// also, make here smoothing group check
				if((a->face->pMaterial == b->face->pMaterial) && dot(a->vertices[i].normal, b->vertices[j].normal) > AUTOSMOOTHGROUP_THRESH)
				{
					Vector3D final_normal = normalize(a->vertices[i].normal + b->vertices[j].normal);
					a->vertices[i].normal = final_normal;
					b->vertices[j].normal = final_normal;
				}
			}
		}
	}
}

brushwinding_t* ClipPolygonToBrush(cwbrush_t* pClipper, int nFaceIndex, bool bClipOnPlane)
{
	return NULL;
}

void ClipToBrush(cwbrush_t* pDest, cwbrush_t* pClipper, bool bClipOnPlane)
{
	for(int i = 0; i < pDest->numfaces; i++)
	{
		pDest->faces[i].winding = ClipPolygonToBrush(pClipper, 0, bClipOnPlane);
	}
}

// brush processing and converting into simple surfaces
void ProcessBrushes()
{
	if(g_pBrushList == NULL)
	{
		MsgWarning("No brushes found, skipping\n");
		return;
	}

	MsgWarning("Processing brushes...\n");

	//cwbrush_t* pBrush;

	// make winding list
	StartPacifier("MakeWindings ");
	cwbrush_t* list;

	int i = 0;
	for ( list = g_pBrushList; list ; list = list->next, i++)
	{
		cwbrush_t* pBrush = list;
		UpdatePacifier((float)i / (float)g_numBrushes);

		for(int j = 0; j < pBrush->numfaces; j++)
		{
			brushwinding_t* winding = AllocWinding();
			
			MakeInfiniteWinding(winding, pBrush->faces[j].Plane);

			pBrush->faces[j].winding = winding;

			// chop me by another brush faces
			for(int k = 0; k < pBrush->numfaces; k++)
			{
				if(k == j)
					continue;

				Plane clipPlane = pBrush->faces[k].Plane;
				clipPlane.normal *= -1.0f;
				clipPlane.offset *= -1.0f;

				ChopWinding(winding, clipPlane);
			}

			MakeWindingRenderStuffFromFace(winding, &pBrush->faces[j]);
		}
	}
	EndPacifier();

	if(worldGlobals.bBrushCSG)
	{
		// make the linked list
		cwbrush_t* pClippedList = CopyBrushList(g_pBrushList);

		// CSG processing
		StartPacifier("BrushCSG: union ");

		/*
		cwbrush_t* list;
		int i = 0;

		for ( cwbrush_t* list = g_pBrushList; list ; list = list->next, i++)
		{
			cwbrush_t* list2;
			int j = 0;

			bool bClipOnPlane = false;

			for ( cwbrush_t* list2 = g_pBrushList; list2 ; list2 = list2->next, j++)
			{
				if(i != j)
				{
					ClipToBrush(list, list2, bClipOnPlane);
				}
				else
					bClipOnPlane = true;
			}
		}

		FreeBrushList( g_pBrushList );
		g_pBrushList = pClippedList;	// use new list
		*/
		EndPacifier();
	}

	StartPacifier("BrushWindingProcess ");

	// automatic smoothing groups assignment
	for(i = 0; i < g_windings.numElem(); i++)
	{
		if((g_windings[i]->face->brush->flags & BRUSH_FLAG_AREAPORTAL) || (g_windings[i]->face->brush->flags & BRUSH_FLAG_ROOMFILLER))
			continue;

		MakeWindingRenderStuffFromFace(g_windings[i], g_windings[i]->face);

		UpdatePacifier((float)i / (float)g_windings.numElem());
		for(int j = 0; j < g_windings.numElem(); j++)
		{
			if(i == j)
				continue;

			WeldWindingVertices(g_windings[i], g_windings[j]);
		}

		MakeSurfaceFromWinding(g_windings[i]);
	}

	EndPacifier();
}