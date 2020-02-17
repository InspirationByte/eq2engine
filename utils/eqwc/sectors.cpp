//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Sectors and subdivision
//////////////////////////////////////////////////////////////////////////////////

// Sector geometry keeps here
#include "threadedprocess.h"
#include "eqwc.h"

void CopyLitSurface(cwlitsurface_t* pSrc, cwlitsurface_t* pDst)
{
	pDst->flags			= pSrc->flags;
	pDst->bbox			= pSrc->bbox;
	pDst->numIndices	= pSrc->numIndices;
	pDst->numVertices	= pSrc->numVertices;

	pDst->pMaterial		= pSrc->pMaterial;

	pDst->lightmap_id	= pSrc->lightmap_id;

	if(pDst->pVerts)
		delete [] pDst->pVerts;

	if(pDst->pIndices)
		delete [] pDst->pIndices;

	pDst->pVerts = new eqlevelvertexlm_t[pDst->numVertices];
	pDst->pIndices = new int[pDst->numIndices];

	memcpy(pDst->pVerts, pSrc->pVerts, sizeof(eqlevelvertexlm_t)*pDst->numVertices);
	memcpy(pDst->pIndices, pSrc->pIndices, sizeof(int)*pDst->numIndices);
}

void SplitSurfaceByPlane(cwlitsurface_t* surf, Plane &plane, cwlitsurface_t* front, cwlitsurface_t* back)
{
	int* front_vert_remap = new int[surf->numVertices];
	int* back_vert_remap = new int[surf->numVertices];

	int nNegative = 0;

	for(int i = 0; i < surf->numVertices; i++)
	{
		front_vert_remap[i] = -1;
		back_vert_remap[i] = -1;

		if(plane.ClassifyPointEpsilon( surf->pVerts[i].position, 0.0f) == CP_BACK)
			nNegative++;
	}

	if(nNegative == surf->numVertices)
	{
		if(back)
			CopyLitSurface(surf, back);

		delete [] front_vert_remap;
		delete [] back_vert_remap;

		return;
	}

	if(nNegative == 0)
	{
		if(front)
			CopyLitSurface(surf, front);

		delete [] front_vert_remap;
		delete [] back_vert_remap;

		return;
	}

	DkList<eqlevelvertexlm_t>	front_vertices;
	DkList<int>					front_indices;

	DkList<eqlevelvertexlm_t>	back_vertices;
	DkList<int>					back_indices;

	front_vertices.resize( surf->numVertices + 16 );
	front_indices.resize( surf->numIndices + 16 );

	back_vertices.resize( surf->numVertices + 16 );
	back_indices.resize( surf->numIndices + 16 );

#define ADD_REMAPPED_VERTEX(idx, vertex, name)						\
{																\
	if(##name##_vert_remap[ idx ] != -1)						\
	{															\
		##name##_indices.append( ##name##_vert_remap[ idx ] );	\
	}															\
	else														\
	{															\
		int currVerts = ##name##_vertices.numElem();			\
		##name##_vert_remap[idx] = currVerts;					\
		##name##_indices.append( currVerts );					\
		##name##_vertices.append( vertex );						\
	}															\
}

#define ADD_VERTEX(vertex, name)								\
{																\
	int currVerts = ##name##_vertices.numElem();				\
	##name##_indices.append( currVerts );						\
	##name##_vertices.append( vertex );							\
}

	for(int i = 0; i < surf->numIndices; i += 3)
	{
		eqlevelvertexlm_t v[3];
		float d[3];
		int id[3];

		for (int j = 0; j < 3; j++)
		{
			id[j] = surf->pIndices[i + j];
			v[j] = surf->pVerts[id[j]];
			d[j] = plane.Distance(v[j].position);
		}

		// fill non-clipped triangles with fastest remap method
		if (d[0] >= 0 && d[1] >= 0 && d[2] >= 0)
		{
			if (front)
			{
				for (int j = 0; j < 3; j++)
				{
					ADD_REMAPPED_VERTEX(id[j], v[j], front)
				}
			}
		}
		else if (d[0] < 0 && d[1] < 0 && d[2] < 0)
		{
			if (back)
			{
				for (int j = 0; j < 3; j++)
				{
					ADD_REMAPPED_VERTEX(id[j], v[j], back)
				}
			}
		}
		else // clip vertices and add
		{
			int front0 = 0;
			int prev = 2;
			for (int k = 0; k < 3; k++)
			{
				if (d[prev] < 0 && d[k] >= 0)
				{
					front0 = k;
					break;
				}
				prev = k;
			}

			int front1 = (front0 + 1) % 3;
			int front2 = (front0 + 2) % 3;

			if (d[front1] >= 0)
			{
				float x0 = d[front1] / dot(plane.normal, v[front1].position - v[front2].position);
				float x1 = d[front0] / dot(plane.normal, v[front0].position - v[front2].position);

				if(front)
				{
					
					ADD_REMAPPED_VERTEX(surf->pIndices[i+front0], surf->pVerts[surf->pIndices[i+front0]], front)
					ADD_REMAPPED_VERTEX(surf->pIndices[i+front1], surf->pVerts[surf->pIndices[i+front1]], front)

					// middle vertex is shared
					int idx = front_vertices.append(InterpolateLevelVertex(surf->pVerts[surf->pIndices[i+front1]], surf->pVerts[surf->pIndices[i+front2]], x0));
					front_indices.append(idx);
					
					ADD_REMAPPED_VERTEX(surf->pIndices[i+front0], surf->pVerts[surf->pIndices[i+front0]], front)
					front_indices.append(idx);
					ADD_VERTEX(InterpolateLevelVertex(surf->pVerts[surf->pIndices[i+front0]], surf->pVerts[surf->pIndices[i+front2]], x1), front)

				}

				if(back)
				{
					// non-interpolated verts added as well
					ADD_REMAPPED_VERTEX(surf->pIndices[i+front2], surf->pVerts[surf->pIndices[i+front2]], back)
					ADD_VERTEX( InterpolateLevelVertex(surf->pVerts[surf->pIndices[i+front0]], surf->pVerts[surf->pIndices[i+front2]], x1), back)
					ADD_VERTEX( InterpolateLevelVertex(surf->pVerts[surf->pIndices[i+front1]], surf->pVerts[surf->pIndices[i+front2]], x0), back)
				}
			} 
			else 
			{
				float x0 = d[front0] / dot(plane.normal, v[front0].position - v[front1].position);
				float x1 = d[front0] / dot(plane.normal, v[front0].position - v[front2].position);

				if(front)
				{
					// non-interpolated verts added as well
					ADD_REMAPPED_VERTEX(surf->pIndices[i+front0], surf->pVerts[surf->pIndices[i+front0]], back)
					ADD_VERTEX( InterpolateLevelVertex(surf->pVerts[surf->pIndices[i+front0]], surf->pVerts[surf->pIndices[i+front1]], x0), back)
					ADD_VERTEX( InterpolateLevelVertex(surf->pVerts[surf->pIndices[i+front0]], surf->pVerts[surf->pIndices[i+front2]], x1), back)
				}

				if(back)
				{
					
					ADD_REMAPPED_VERTEX(surf->pIndices[i+front1], surf->pVerts[surf->pIndices[i+front1]], back)
					ADD_REMAPPED_VERTEX(surf->pIndices[i+front2], surf->pVerts[surf->pIndices[i+front2]], back)

					// middle vertex is shared
					int idx = back_vertices.append(InterpolateLevelVertex(surf->pVerts[surf->pIndices[i+front0]], surf->pVerts[surf->pIndices[i+front2]], x1));
					back_indices.append(idx);

					ADD_REMAPPED_VERTEX(surf->pIndices[i+front1], surf->pVerts[surf->pIndices[i+front1]], back)
					back_indices.append(idx);
					ADD_VERTEX(InterpolateLevelVertex(surf->pVerts[surf->pIndices[i+front0]], surf->pVerts[surf->pIndices[i+front1]], x0), back)
					
				}
			}
		}
	}

	// remappers done now
	delete [] front_vert_remap;
	delete [] back_vert_remap;

	if(front_indices.numElem() > 0 && front)
	{
		if(front->pVerts)
			delete [] front->pVerts;

		if(front->pIndices)
			delete [] front->pIndices;

		front->numVertices = front_vertices.numElem();
		front->numIndices = front_indices.numElem();
		front->pVerts = new eqlevelvertexlm_t[front->numVertices];
		front->pIndices = new int[front->numIndices];
		front->lightmap_id = surf->lightmap_id;

		memcpy(front->pVerts, front_vertices.ptr(), sizeof(eqlevelvertexlm_t) * front->numVertices);
		memcpy(front->pIndices, front_indices.ptr(), sizeof(int) * front->numIndices);

		front->pMaterial = surf->pMaterial;
		front->flags = surf->flags;
	}

	if(back_indices.numElem() > 0 && back)
	{
		if(back->pVerts)
			delete [] back->pVerts;

		if(back->pIndices)
			delete [] back->pIndices;

		back->numVertices = back_vertices.numElem();
		back->numIndices = back_indices.numElem();
		back->pVerts = new eqlevelvertexlm_t[back->numVertices];
		back->pIndices = new int[back->numIndices];
		back->lightmap_id = surf->lightmap_id;

		memcpy(back->pVerts, back_vertices.ptr(), sizeof(eqlevelvertexlm_t) * back->numVertices);
		memcpy(back->pIndices, back_indices.ptr(), sizeof(int) * back->numIndices);

		back->pMaterial = surf->pMaterial;
		back->flags = surf->flags;
	}
}

void AssignSurfacesToSector(cwsector_t* pSector)
{
	for(int i = 0; i < g_litsurfaces.numElem(); i++)
	{
		if((g_litsurfaces[i]->flags & CS_FLAG_USEDINSECTOR) || (g_litsurfaces[i]->flags & CS_FLAG_SUBDIVIDE) ||
			(g_litsurfaces[i]->flags & CS_FLAG_NODRAW) || (g_litsurfaces[i]->flags & CS_FLAG_OCCLUDER))
			continue;

		// check surface boundaries
		// if surface is bigger than the sector size, then keep it for future subdivisions.
		// small geometry will not being subdivided

		if(pSector->bbox.Intersects(g_litsurfaces[i]->bbox))
		{
			bool bMustDivide = !(g_litsurfaces[i]->flags & CS_FLAG_DONTSUBDIVIDE);

			// check sizes
			if(length(g_litsurfaces[i]->bbox.GetSize()) < length(pSector->bbox.GetSize()) || !bMustDivide)
			{
				// add sector usage flag only
				g_litsurfaces[i]->flags |= CS_FLAG_USEDINSECTOR;

				// add as node to sector
				cwsectornode_t node;
				node.surface = g_litsurfaces[i];
				node.bbox = g_litsurfaces[i]->bbox;

				pSector->nodes.append(node);
			}
			else
			{
				// let it be for subdivision
				g_litsurfaces[i]->flags |= CS_FLAG_SUBDIVIDE;
			}
		}
	}
}

static int sizeX = 0;
static int sizeY = 0;
static int sizeZ = 0;

// subdivides surface by cubes
#pragma todo("add room's sub-volumes (create tree) instead of surfaces for more fastest culling")
bool SectoralSurfaceSubdivision(cwlitsurface_t *surf, DkList<cwlitsurface_t*>& outSurface)
{
	//if(!(surf->flags & CS_FLAG_SUBDIVIDE))
	//	return false;

	cwlitsurface_t* pSurf = surf;

	// create an sectors and assign surfaces to the sectors to do subdivisions faster
	for (int z = 0; z < sizeZ; z++)
	{
		float fz0 = worldGlobals.worldBox.minPoint.z + worldGlobals.fSectorSize * z;
		float fz1 = worldGlobals.worldBox.minPoint.z + worldGlobals.fSectorSize * (z + 1);

		cwlitsurface_t* frontZ = new cwlitsurface_t;
		cwlitsurface_t* backZ = new cwlitsurface_t;

		Plane plZ(Vector3D(0,0,-1), fz1);

		SplitSurfaceByPlane(pSurf, plZ, frontZ, backZ);

		for (int y = 0; y < sizeY; y++)
		{
			float fy0 = worldGlobals.worldBox.minPoint.y + worldGlobals.fSectorSize * y;
			float fy1 = worldGlobals.worldBox.minPoint.y + worldGlobals.fSectorSize * (y + 1);

			cwlitsurface_t* frontY = new cwlitsurface_t;
			cwlitsurface_t* backY = new cwlitsurface_t;

			Plane plY(Vector3D(0,-1,0), fy1);

			SplitSurfaceByPlane(frontZ, plY, frontY, backY);

			for (int x = 0; x < sizeX; x++)
			{
				float fx0 = worldGlobals.worldBox.minPoint.x + worldGlobals.fSectorSize * x;
				float fx1 = worldGlobals.worldBox.minPoint.x + worldGlobals.fSectorSize * (x + 1);

				cwlitsurface_t* frontX = new cwlitsurface_t;
				cwlitsurface_t* backX = new cwlitsurface_t;

				Plane plX(Vector3D(-1,0,0), fx1);

				SplitSurfaceByPlane(frontY, plX, frontX, backX);
				FreeLitSurface(frontY);

				if(frontX->numVertices)
				{
					ComputeSurfaceBBOX( frontX );

					// Add frontX to room volume
					// TODO: perfectly to add new room volume, needs whole rework
					outSurface.append(frontX);
				}
				else
					FreeLitSurface(frontX);

				frontY = backX;
			}

			FreeLitSurface(frontY);
			frontZ = backY;
		}

		FreeLitSurface(frontZ);
		pSurf = backZ;
	}

	return true;
}

void ProcessSectorSubdivision()
{
	// compute level bounding volumes from world size
	float cx = ceilf(( worldGlobals.worldBox.maxPoint.x - worldGlobals.worldBox.minPoint.x) / worldGlobals.fSectorSize);
	float cy = ceilf(( worldGlobals.worldBox.maxPoint.y - worldGlobals.worldBox.minPoint.y) / worldGlobals.fSectorSize);
	float cz = ceilf(( worldGlobals.worldBox.maxPoint.z - worldGlobals.worldBox.minPoint.z) / worldGlobals.fSectorSize);

	Vector3D mid = 0.5f * ( worldGlobals.worldBox.minPoint + worldGlobals.worldBox.maxPoint );

	sizeX = (int) cx;
	sizeY = (int) cy;
	sizeZ = (int) cz;

	int nCubes = sizeX * sizeY * sizeZ;

	Msg("Subdivision box size: %g\n", worldGlobals.fSectorSize);
}

DkList<cwareaportal_t*> g_portalList;
DkList<cwroom_t*>		g_roomList;

void SubdivideRoomVolume(cwroom_t* pRoom, cwroomvolume_t* volume)
{
	DkList<cwlitsurface_t*> litSurfacesSubdiv;

	for(int i = 0; i < volume->surfaces.numElem(); i++)
	{
		if(SectoralSurfaceSubdivision(volume->surfaces[i], litSurfacesSubdiv))
			FreeLitSurface(volume->surfaces[i]);
		else
			litSurfacesSubdiv.append(volume->surfaces[i]);
	}

	volume->surfaces.clear();
	volume->surfaces.append(litSurfacesSubdiv);
}

void BrushBounds(cwbrush_t* pBrush, BoundingBox &box)
{
	box.Reset();

	for(int i = 0; i < pBrush->numfaces; i++)
	{
		for(int j = 0; j < pBrush->faces[i].winding->numVerts; j++)
		{
			box.AddVertex(pBrush->faces[i].winding->vertices[j].position);
		}
		/*
		box.AddVertices(pBrush->faces[i].winding->vertices, 
						pBrush->faces[i].winding->numVerts,
						offsetOf(eqlevelvertex_t, position),
						sizeof(eqlevelvertex_t));
						*/
	}
}

bool IsPointInsideVolume(cwroomvolume_t* volume, Vector3D &point, float eps = 0.1f)
{
	for(int i = 0; i < volume->volumePlanes.numElem(); i++)
	{
		ClassifyPlane_e nPlaneClass = volume->volumePlanes[i].ClassifyPointEpsilon(point, eps);

		if(nPlaneClass == CP_FRONT)
			return false;
	}

	return true;
}

bool IsPointInsideBrush(cwbrush_t* volume, Vector3D &point, float eps = 0.1f)
{
	for(int i = 0; i < volume->numfaces; i++)
	{
		ClassifyPlane_e nPlaneClass = volume->faces[i].Plane.ClassifyPointEpsilon(point, eps);

		if(nPlaneClass == CP_FRONT)
			return false;
	}

	return true;
}

bool IsVolumeIntersectsOtherVolume(cwroomvolume_t* volume1, cwroomvolume_t* volume2)
{
	for(int i = 0; i < volume2->volumePoints.numElem(); i++)
	{
		if(IsPointInsideVolume(volume1, volume2->volumePoints[i], 0.5f))
			return true;
	}

	for(int i = 0; i < volume1->volumePoints.numElem(); i++)
	{
		if(IsPointInsideVolume(volume2, volume1->volumePoints[i], 0.5f))
			return true;
	}

	return false;
}

bool IsAreaportalIntersectsVolume(cwareaportal_t* volume1, cwroomvolume_t* volume2)
{
	for(int i = 0; i < volume2->volumePoints.numElem(); i++)
	{
		if(IsPointInsideBrush(volume1->brush, volume2->volumePoints[i], 0.5f))
			return true;
	}

	for(int i = 0; i < volume1->volumePoints.numElem(); i++)
	{
		if(IsPointInsideVolume(volume2, volume1->volumePoints[i], 0.5f))
			return true;
	}

	return false;
}

brushwinding_t* GetInsideWinding(cwbrush_t* volume1, cwroomvolume_t* volume2)
{
	for(int i = 0; i < volume1->numfaces; i++)
	{
		brushwinding_t* pWinding = volume1->faces[i].winding;
		if(!pWinding)
		{
			continue;
		}

		if(volume1->faces[i].flags & CS_FLAG_NODRAW)
		{
			continue;
		}

		int numInside = 0;
		for(int j = 0; j < pWinding->numVerts; j++)
		{
			if(IsPointInsideVolume(volume2, pWinding->vertices[j].position))
				numInside++;
		}

		if(numInside > 0)
			return pWinding;
	}

	return NULL;
}

int FindPlane(Plane& pl, cwroomvolume_t* volume)
{
	for(int i = 0; i < volume->volumePlanes.numElem(); i++)
	{
		// be more strict with normals
		if(volume->volumePlanes[i].CompareEpsilon(pl, 32.0, 0.15f))
			return i;
	}

	return -1;
}

void MakePointListForAreaPortal(cwareaportal_t* vol)
{
	for(int i = 0; i < vol->brush->numfaces; i++)
	{
		Plane iPlane = vol->brush->faces[i].Plane;
		for(int j = i+1; j < vol->brush->numfaces; j++)
		{
			Plane jPlane = vol->brush->faces[j].Plane;
			for(int k = j+1; k < vol->brush->numfaces; k++)
			{
				Plane kPlane = vol->brush->faces[k].Plane;

				Vector3D point;
				if( iPlane.GetIntersectionWithPlanes(jPlane, kPlane, point) )
				{
					if(IsPointInsideBrush( vol->brush, point ))
						vol->volumePoints.append(point);
				}
			}
		}
	}

	//Msg("Added points: %d\n", vol->volumePoints.numElem());
}

void MakePortalList()
{
	int nBrush = 0;
	for(cwbrush_t* list = g_pBrushList; list; list = list->next)
	{
		if((list->flags & BRUSH_FLAG_ROOMFILLER) && (list->flags & BRUSH_FLAG_AREAPORTAL))
			continue;

		if(!(list->flags & BRUSH_FLAG_AREAPORTAL))
			continue;

		if(list->numfaces != 6)
		{
			MsgWarning("Warning! AreaPortal Brush %d have more or less than 6 faces (%d)\n", nBrush, list->numfaces);
			continue;
		}

		cwareaportal_t* pPortal = new cwareaportal_t;
		pPortal->brush = list;
		pPortal->numRooms = 0;

		BrushBounds(pPortal->brush, pPortal->bounds);

		memset(pPortal->rooms, 0, sizeof(pPortal->rooms));
		memset(pPortal->windings, 0, sizeof(pPortal->windings));

		MakePointListForAreaPortal( pPortal );

		g_portalList.append(pPortal);
	}

	// find intersecting rooms for these portals
	for(int i = 0; i < g_portalList.numElem(); i++)
	{
		for(int j = 0; j < g_roomList.numElem(); j++)
		{
			if(g_roomList[j]->bounds.Intersects(g_portalList[i]->bounds))
			{
				for(int k = 0; k < g_roomList[j]->volumes.numElem(); k++)
				{
					cwroomvolume_t* pVolume = g_roomList[j]->volumes[k];

					if(  pVolume->bounds.Intersects(g_portalList[i]->bounds) && IsAreaportalIntersectsVolume( g_portalList[i], pVolume ) )
					{
						if(g_portalList[i]->numRooms == 2)
						{
							MsgError("ERROR! Portal can only connect two rooms!\n");
							continue;
						} 

						// portal center
						Vector3D portal_center = g_portalList[i]->bounds.GetCenter();

						brushwinding_t* winding = GetInsideWinding( g_portalList[i]->brush, pVolume );
						g_portalList[i]->windings[g_portalList[i]->numRooms] = winding;

						g_portalList[i]->rooms[g_portalList[i]->numRooms] = g_roomList[j];

						// compute distance between selected face and center
						float fMoveDistance = fabs(winding->face->Plane.Distance(portal_center));

						Plane pl = winding->face->Plane;

						pl.normal *= -1;
						pl.offset *= -1;

						// get the facing plane to areaportal plane and move it to center of portal
						int planeIdx = FindPlane( pl, pVolume );

						// 
						if(planeIdx != -1)
						{
							if(g_portalList[i]->numRooms == 0)
							{
								Plane plane = pVolume->volumePlanes[planeIdx];

								plane.offset = -dot(plane.normal, portal_center);

								// set planes for two rooms
								pVolume->volumePlanes[planeIdx] = plane;

								g_portalList[i]->firstroom_volume_idx = k;
								g_portalList[i]->firstroom_volplane_idx = planeIdx;
							}
							else if(g_portalList[i]->numRooms == 1)
							{
								// set to be identifical with previous plane to avoid errors
								int volIdx = g_portalList[i]->firstroom_volume_idx;
								int nplaneIdx = g_portalList[i]->firstroom_volplane_idx;

								Plane neoPlane = g_portalList[i]->rooms[0]->volumes[volIdx]->volumePlanes[nplaneIdx];
								neoPlane.normal *= -1;
								neoPlane.offset *= -1;

								pVolume->volumePlanes[planeIdx] = neoPlane;
							}

							pVolume->bounds.AddVertex(portal_center);
							g_roomList[j]->bounds.AddVertex(portal_center);
						}
						else
							MsgWarning("No facing plane to portal (designer's fail or bug?).");

						g_portalList[i]->numRooms++;

						g_roomList[j]->portals[g_roomList[j]->numPortals] = g_portalList[i];
						g_roomList[j]->numPortals++;

						// goto next room
						break;
					}
				}
			}
		}
	}

	Msg("%d area portals\n", g_portalList.numElem());
}

void BuildVolumeListForRoom_r(cwroom_t* pRoom, cwroomvolume_t* checkVolume, DkList<cwroomvolume_t*> &volumeList)
{
	for(int i = 0; i < volumeList.numElem(); i++)
	{
		if(checkVolume == volumeList[i])
			continue;

		if(volumeList[i]->checked)
			continue;

		Volume vol;
		vol.LoadBoundingBox(checkVolume->bounds.minPoint, checkVolume->bounds.maxPoint);

		Vector3D mins = volumeList[i]->bounds.minPoint;
		Vector3D maxs = volumeList[i]->bounds.maxPoint;

		if(vol.IsBoxInside( mins.x, maxs.x, mins.y, maxs.y, mins.z, maxs.z, 1.0f))
		{
			if( IsVolumeIntersectsOtherVolume(checkVolume, volumeList[i]) )
			{
				// we need to get touching planes and equalize them
				for(int j = 0; j < volumeList[i]->volumePlanes.numElem(); j++)
				{
					for(int k = 0; k < checkVolume->volumePlanes.numElem(); k++)
					{
						Plane pl = checkVolume->volumePlanes[k];
						pl.normal *= -1;
						pl.offset *= -1;

						// max inside to 64 units
						if(volumeList[i]->volumePlanes[j].CompareEpsilon(pl, 64.0f, 0.25f ))
							volumeList[i]->volumePlanes[j] = pl;
					}
				}

				volumeList[i]->checked = true;
				pRoom->volumes.append(volumeList[i]);

				pRoom->bounds.Merge(volumeList[i]->bounds);

				// recurse them
				BuildVolumeListForRoom_r(pRoom, volumeList[i], volumeList);
			}
		}
	}

}

void MakePointListForVolume(cwroomvolume_t* vol)
{
	for(int i = 0; i < vol->volumePlanes.numElem(); i++)
	{
		Plane iPlane = vol->volumePlanes[i];
		for(int j = i+1; j < vol->volumePlanes.numElem(); j++)
		{
			Plane jPlane = vol->volumePlanes[j];
			for(int k = j+1; k < vol->volumePlanes.numElem(); k++)
			{
				Plane kPlane = vol->volumePlanes[k];

				Vector3D point;
				if( iPlane.GetIntersectionWithPlanes(jPlane, kPlane, point) )
				{
					if(IsPointInsideVolume( vol, point ))
						vol->volumePoints.append(point);
				}
			}
		}
	}

	//Msg("Added points: %d\n", vol->volumePoints.numElem());
}

void MakeRoomList()
{
	DkList<cwroomvolume_t*> volumeList;

	for(cwbrush_t* list = g_pBrushList; list; list = list->next)
	{
		if((list->flags & BRUSH_FLAG_ROOMFILLER) && (list->flags & BRUSH_FLAG_AREAPORTAL))
			continue;

		if(list->flags & BRUSH_FLAG_ROOMFILLER)
		{
			cwroomvolume_t* pVolume = new cwroomvolume_t;
			volumeList.append(pVolume);

			pVolume->brush = list;

			pVolume->checked = false;
			pVolume->room = NULL;

			BrushBounds(pVolume->brush, pVolume->bounds);

			for(int j = 0; j < list->numfaces; j++)
				pVolume->volumePlanes.append(list->faces[j].Plane);

			// make point list for volume
			MakePointListForVolume( pVolume );
		}
	}

	cwroom_t* pRoom = NULL;

	StartPacifier("AssignRoomVolumes ");

	// find intersecting volumes and join 'em all
	for(int i = 0; i < volumeList.numElem(); i++)
	{
		UpdatePacifier((float)i / (float)volumeList.numElem());

		// if ve have checked volume, continue...
		if(volumeList[i]->checked)
			continue;

		// start new room
		pRoom = new cwroom_t;
		pRoom->numPortals = 0;
		volumeList[i]->checked = true;
		pRoom->volumes.append(volumeList[i]);

		pRoom->bounds.Reset();

		pRoom->bounds.Merge(volumeList[i]->bounds);
		volumeList[i]->room = pRoom;

		pRoom->room_idx = g_roomList.numElem();

		g_roomList.append(pRoom);

		BuildVolumeListForRoom_r(pRoom, volumeList[i], volumeList);

		//Msg("%d volumes in room\n", pRoom->volumes.numElem());
	}

	EndPacifier();

	Msg("%d rooms\n", g_roomList.numElem());
}

int GetSurfaceInsideVolumeVertsCount(cwroomvolume_t* pVolume, cwlitsurface_t* pSurf)
{
	int count = 0;

	for(int i = 0; i < pSurf->numVertices; i++)
	{
		count += (IsPointInsideVolume(pVolume, pSurf->pVerts[i].position) ? 1 : 0);
	}

	return count;
}

cwlitsurface_t* ClipSurfaceToVolume(cwlitsurface_t* pSurface, cwroomvolume_t* pVolume);

void AssignSurfacesToRoomVolume(cwroomvolume_t* pVolume)
{
	for(int i = 0; i < g_litsurfaces.numElem(); i++)
	{
		/*
		if((g_litsurfaces[i]->flags & CS_FLAG_USEDINSECTOR) || (g_litsurfaces[i]->flags & CS_FLAG_SUBDIVIDE) ||
			(g_litsurfaces[i]->flags & CS_FLAG_NODRAW) || (g_litsurfaces[i]->flags & CS_FLAG_OCCLUDER))
			continue;
			*/

		if((g_litsurfaces[i]->flags & CS_FLAG_SUBDIVIDE) || (g_litsurfaces[i]->flags & CS_FLAG_NODRAW) || (g_litsurfaces[i]->flags & CS_FLAG_OCCLUDER))
			continue;

		// check surface boundaries
		// if surface is bigger than the sector size, then keep it for future subdivisions.
		// small geometry will not being subdivided

		if(pVolume->bounds.Intersects( g_litsurfaces[i]->bbox ))
		{
			int nVerts = GetSurfaceInsideVolumeVertsCount(pVolume, g_litsurfaces[i]);

			float fInsidePercent = ((float)nVerts / (float)g_litsurfaces[i]->numVertices);

			// if not all vertices inside, we need to subdivide
			bool bMustDivide = (nVerts != g_litsurfaces[i]->numVertices) && !(g_litsurfaces[i]->flags & CS_FLAG_DONTSUBDIVIDE);

			// check sizes
			if(/*length(g_litsurfaces[i]->bbox.GetSize()) < length(pVolume->bounds.GetSize()) &&*/ !bMustDivide)
			{
				if(g_litsurfaces[i]->numIndices < 3)
				{
					Msg("Removed\n");
					continue;
				}

				// check "insideness"
				//if(fInsidePercent < 0.25f)
				//	continue;

				// add sector usage flag only
				ThreadLock();
				g_litsurfaces[i]->flags |= CS_FLAG_USEDINSECTOR;
				ThreadUnlock();

				pVolume->surfaces.append( g_litsurfaces[i] );
			}
			else
			{
				// clip by this volume
				cwlitsurface_t* pSurface = ClipSurfaceToVolume(g_litsurfaces[i], pVolume);

				ThreadLock();
				g_litsurfaces[i]->flags |= CS_FLAG_USEDINSECTOR;
				ThreadUnlock();

				if(pSurface)
				{
					// add sector usage flag only
					pSurface->flags |= (CS_FLAG_SUBDIVIDE | CS_FLAG_USEDINSECTOR);

					ThreadLock();
					g_litsurfaces.append(pSurface);
					ThreadUnlock();

					ComputeSurfaceBBOX( pSurface );

					pVolume->surfaces.append(pSurface);
				}
			}
		}
	}
}

cwlitsurface_t* ClipSurfaceToVolume(cwlitsurface_t* pSurface, cwroomvolume_t* pVolume)
{
	/*
	if(!(pSurface->flags & CS_FLAG_SUBDIVIDE))
		return NULL;
		*/

	cwlitsurface_t* pOutFace = pSurface;

	for (int i = 0; i < pVolume->volumePlanes.numElem(); i++)
	{
		cwlitsurface_t* pTemp = new cwlitsurface_t;

		SplitSurfaceByPlane(pOutFace, pVolume->volumePlanes[i], NULL, pTemp);
		
		// if we totally clipped all
		if(pTemp->numIndices == 0)
		{
			// free old clipped ( but keep original)
			if(pOutFace != pSurface )
				FreeLitSurface(pOutFace);

			return NULL;
		}

		// free old clipped ( but keep original)
		if(pOutFace != pSurface )
			FreeLitSurface(pOutFace);

		// set it and do far
		pOutFace = pTemp;
	}


	
	// done
	return pOutFace;
}

/*
void ThreadedAssignSurfsToRoom(int i)
{
	for(int j = 0; j < g_roomList[i]->volumes.numElem(); j++)
	{
		AssignSurfacesToRoomVolume(g_roomList[i]->volumes[j]);
	}
}
*/
cwroom_t* g_pCurrentRoom = NULL;
void ThreadedAssignSurfacesToRoomVolume(int i)
{
	AssignSurfacesToRoomVolume(g_pCurrentRoom->volumes[i]);
}

void AssignSurfacesToVolumes()
{
	StartPacifier("AssignSurfacesToVolumes ");

	//RunThreadsOnIndividual(g_roomList.numElem(), true, ThreadedAssignSurfsToRoom);
	
	for(int i = 0; i < g_roomList.numElem(); i++)
	{
		UpdatePacifier((float)i / (float)g_roomList.numElem());

		g_pCurrentRoom = g_roomList[i];

		//RunThreadsOnIndividual(g_pCurrentRoom->volumes.numElem(), false, ThreadedAssignSurfacesToRoomVolume);

		for(int j = 0; j < g_pCurrentRoom->volumes.numElem(); j++)
		{
			AssignSurfacesToRoomVolume( g_pCurrentRoom->volumes[j] );

			// subdivide room volume on cube sectors if allowed
			if(worldGlobals.bSectorDivision)
				SubdivideRoomVolume(g_pCurrentRoom, g_pCurrentRoom->volumes[j]);
		}
		
	}

	EndPacifier();

	int nUnassignedCount = 0;

	for(int i = 0; i < g_litsurfaces.numElem(); i++)
	{
		if(!(g_litsurfaces[i]->flags & CS_FLAG_USEDINSECTOR))
			nUnassignedCount++;
	}

	if(nUnassignedCount)
		MsgError("Warning! Unassigned geometry found! They will not appear\nin the game/engine since you haven't\ncreated room/volume for them (%d exposed)!\n", nUnassignedCount);
}

void MakeSectors()
{
	MsgWarning("Computing new sector boxes...\n");

	// subdivide level bbox on the equal sectors with size g_fSectorSize
	// then subdivide whole geometry on this sectors
	// also the geometry marked with flag STFL_BACKGROUND 
	// will not be subdivided and will present in the main sector

	MsgInfo("World dimensions [%g %g %g] [%g %g %g]\n", 
		worldGlobals.worldBox.minPoint.x, worldGlobals.worldBox.minPoint.y, worldGlobals.worldBox.minPoint.z,
		worldGlobals.worldBox.maxPoint.x, worldGlobals.worldBox.maxPoint.y, worldGlobals.worldBox.maxPoint.z);

	// recompute bbox again
	for(int i = 0; i < g_litsurfaces.numElem(); i++)
		ComputeSurfaceBBOX( g_litsurfaces[i] );

	int nRooms = 0;
	int	nPortals = 0;
	int nBrushIndex = 0;

	// count rooms and portals
	for(cwbrush_t* list = g_pBrushList; list; list = list->next)
	{
		if((list->flags & BRUSH_FLAG_ROOMFILLER) && (list->flags & BRUSH_FLAG_AREAPORTAL))
		{
			Msg("Brush %d error: Cannot use room and areaportal property on one brush!\n", nBrushIndex);
			nBrushIndex++;
			continue;
		}

		if(list->flags & BRUSH_FLAG_ROOMFILLER)
			nRooms++;

		if(list->flags & BRUSH_FLAG_AREAPORTAL)
			nPortals++;

		nBrushIndex++;
	}

	// only if there is more that two rooms and portal between...
	if(nRooms > 0)
	{
		Msg("%d portals with %d room volumes\n", nPortals, nRooms);

		// TODO: for each layer

		// make room volumes
		MakeRoomList();

		// make portals
		MakePortalList();

		// make sector subdivision and turn them into normal surfaces again
		if(worldGlobals.bSectorDivision)
			ProcessSectorSubdivision();

		// assign surfaces to volumes before subdivision
		AssignSurfacesToVolumes();
	}

	/*
	// TODO: remove sectors
	if(worldGlobals.bSectorDivision)
	{
		ProcessSectorSubdivision();
	}
	else
	{
		// TODO: add all geometry to single sector
	}
	*/

	// remove unused sectors

	//g_sectors.insert(zeroSector, 0);
}