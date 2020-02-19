//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Water volume generator
//////////////////////////////////////////////////////////////////////////////////

#include "threadedprocess.h"
#include "eqwc.h"

DkList<cwwaterinfo_t*>		g_waterInfos;

void BrushBounds(cwbrush_t* pBrush, BoundingBox &box);

bool IsPointInsideWaterVolume(cwwatervolume_t* volume, Vector3D &point, float eps = 0.1f)
{
	for(int i = 0; i < volume->volumePlanes.numElem(); i++)
	{
		ClassifyPlane_e nPlaneClass = volume->volumePlanes[i].ClassifyPoint(point, eps);

		if(nPlaneClass == CP_FRONT)
			return false;
	}

	return true;
}

bool IsPointInsideBrush(cwbrush_t* volume, Vector3D &point, float eps = 0.1f);

bool IsWaterVolumeIntersectsOtherWaterVolume(cwwatervolume_t* volume1, cwwatervolume_t* volume2)
{
	for(int i = 0; i < volume2->volumePoints.numElem(); i++)
	{
		if(IsPointInsideWaterVolume(volume1, volume2->volumePoints[i], 0.5f))
			return true;
	}

	for(int i = 0; i < volume1->volumePoints.numElem(); i++)
	{
		if(IsPointInsideWaterVolume(volume2, volume1->volumePoints[i], 0.5f))
			return true;
	}

	return false;
}

void BuildVolumeListForWaterInfo_r(cwwaterinfo_t* pWater, cwwatervolume_t* checkVolume, DkList<cwwatervolume_t*> &volumeList)
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
			if( IsWaterVolumeIntersectsOtherWaterVolume(checkVolume, volumeList[i]) )
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
				pWater->volumes.append(volumeList[i]);

				pWater->bounds.Merge(volumeList[i]->bounds);

				// recurse them
				BuildVolumeListForWaterInfo_r(pWater, volumeList[i], volumeList);
			}
		}
	}

}

void MakePointListForWaterVolume(cwwatervolume_t* vol)
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
					if(IsPointInsideWaterVolume( vol, point ))
						vol->volumePoints.append(point);
				}
			}
		}
	}

	//Msg("Added points: %d\n", vol->volumePoints.numElem());
}

void MakeWaterInfosFromVolumes()
{
	DkList<cwwatervolume_t*> volumeList;

	for(cwbrush_t* list = g_pBrushList; list; list = list->next)
	{
		if(list->flags & BRUSH_FLAG_WATER)
		{
			cwwatervolume_t* pVolume = new cwwatervolume_t;
			volumeList.append(pVolume);

			pVolume->brush = list;

			pVolume->checked = false;
			pVolume->waterinfo = NULL;

			BrushBounds(pVolume->brush, pVolume->bounds);

			for(int j = 0; j < list->numfaces; j++)
				pVolume->volumePlanes.append(list->faces[j].Plane);

			// make point list for volume
			MakePointListForWaterVolume( pVolume );
		}
	}

	cwwaterinfo_t* pWaterInfo = NULL;

	StartPacifier("MakeWaterInfo ");

	// find intersecting volumes and join 'em all
	for(int i = 0; i < volumeList.numElem(); i++)
	{
		UpdatePacifier((float)i / (float)volumeList.numElem());

		// if ve have checked volume, continue...
		if(volumeList[i]->checked)
			continue;

		// start new water
		pWaterInfo = new cwwaterinfo_t;
		pWaterInfo->waterHeight = 0;

		volumeList[i]->checked = true;
		pWaterInfo->volumes.append(volumeList[i]);

		pWaterInfo->bounds.Reset();

		pWaterInfo->bounds.Merge(volumeList[i]->bounds);
		volumeList[i]->waterinfo = pWaterInfo;

		pWaterInfo->water_idx = g_waterInfos.numElem();
		pWaterInfo->pMaterial = NULL;

		g_waterInfos.append(pWaterInfo);

		BuildVolumeListForWaterInfo_r(pWaterInfo, volumeList[i], volumeList);
	}

	for(int i = 0; i < g_waterInfos.numElem(); i++)
	{
		for(int j = 0; j < pWaterInfo->volumes.numElem(); j++)
		{
			for(int k = 0; k < pWaterInfo->volumes[j]->volumePlanes.numElem(); k++)
			{
				float fDot = dot(pWaterInfo->volumes[j]->volumePlanes[k].normal, Vector3D(0,1,0));

				// the expensive water should have valid waterheight
				if( fDot > 0.9 )
				{
					pWaterInfo->waterHeight = -pWaterInfo->volumes[j]->volumePlanes[k].Distance( vec3_zero );

					cwbrushface_t* pFace = &pWaterInfo->volumes[j]->brush->faces[k];

					pWaterInfo->pMaterial = pFace->pMaterial;
				}
			}
		}
	}

	EndPacifier();

	Msg("%d water infos\n", g_waterInfos.numElem());
}

void MakeWaterVolumes()
{
	MsgWarning("Creating water volumes\n");

	// recompute bbox again
	for(int i = 0; i < g_litsurfaces.numElem(); i++)
		ComputeSurfaceBBOX( g_litsurfaces[i] );

	int nWaterVolumes = 0;

	// count water brushes
	for(cwbrush_t* list = g_pBrushList; list; list = list->next)
	{
		if(list->flags & BRUSH_FLAG_WATER)
			nWaterVolumes++;
	}

	if(nWaterVolumes > 0)
	{
		Msg("%d water volumes found\n", nWaterVolumes);

		// TODO: for each layer

		// make water volumes
		MakeWaterInfosFromVolumes();
	}
}