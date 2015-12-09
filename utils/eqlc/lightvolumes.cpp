//**************** Copyright (C) Parallel Prevision, L.L.C 2011 ******************
//
// Description: Lightmap light volumes
//
//********************************************************************************

#include "eqlc.h"

#define DEFAULT_LVOL_RADIUS 64

float g_fLightVolumeRadius = DEFAULT_LVOL_RADIUS;
BoundingBox						g_worldBox;
DkList<eqlevellightvolume_t>	g_lightvolumes;

int sizeX;
int sizeY;
int sizeZ;


void GenerateLightVolumes()
{
	// restore bbox information
	for(int i = 0; i < g_pLevel->m_numSectors; i++)
	{
		g_worldBox.AddVertex(g_pLevel->m_pSectors[i].sector_info.mins);
		g_worldBox.AddVertex(g_pLevel->m_pSectors[i].sector_info.maxs);
	}

	// compute level bounding volumes from world size
	float cx = ceilf((g_worldBox.maxPoint.x - g_worldBox.minPoint.x) / g_fLightVolumeRadius);
	float cy = ceilf((g_worldBox.maxPoint.y - g_worldBox.minPoint.y) / g_fLightVolumeRadius);
	float cz = ceilf((g_worldBox.maxPoint.z - g_worldBox.minPoint.z) / g_fLightVolumeRadius);

	Vector3D mid = 0.5f * (g_worldBox.minPoint + g_worldBox.maxPoint);

	sizeX = (int) cx;
	sizeY = (int) cy;
	sizeZ = (int) cz;

	int numVolumes = sizeX*sizeY*sizeZ;

	MsgWarning("Light volumes: %d\n", numVolumes);

	g_lights_volume.setNum(g_lights.numElem());

	StartPacifier("GenLightVolumes ");

	g_lightvolumes.setNum(sizeX*sizeY*sizeZ);

	for(int i = 0; i < g_lights_volume.numElem(); i++)
	{
		g_lights_volume[i].assigned_volumes.resize(sizeX*sizeZ);
	}

	eqlevellightvolume_t* volumePtr = g_lightvolumes.ptr();

	// create an sectors and assign surfaces to the sectors to do subdivisions faster
	for (int z = 0; z < sizeZ; z++)
	{
		UpdatePacifier((float)z / (float)sizeZ);

		float fz = g_worldBox.minPoint.z + g_fLightVolumeRadius * (z + 0.5f);

		for (int y = 0; y < sizeY; y++)
		{
			float fy = g_worldBox.minPoint.y + g_fLightVolumeRadius * (y + 0.5f);

			for (int x = 0; x < sizeX; x++)
			{
				float fx = g_worldBox.minPoint.x + g_fLightVolumeRadius * (x + 0.5f);

				volumePtr->position_and_radius.x = fx;
				volumePtr->position_and_radius.y = fy;
				volumePtr->position_and_radius.z = fz;

				volumePtr->sector_id = -1;
				volumePtr->color = vec4_zero;

				/*
				for(int i = 0; i < g_lights.numElem(); i++)
				{
					if(length(g_lights[i]->position - volumePtr->position_and_radius.xyz()) < g_lights[i]->radius.x)
						g_lights_volume[i].assigned_volumes.append(volumePtr);
				}
				*/
				

				volumePtr++;
			}
		}
	}

	EndPacifier();

	/*
	g_lightvolumes.resize(lVolumes.numElem());

	StartPacifier("AssignLightVolumes ");
	for(int i = 0; i < g_pLevel->m_numSectors; i++)
	{
		BoundingBox sector_box(g_pLevel->m_pSectors[i].sector_info.mins, g_pLevel->m_pSectors[i].sector_info.maxs);

		UpdatePacifier((float)i / (float)g_pLevel->m_numSectors);
		for(int j = 0; j < lVolumes.numElem(); j++)
		{
			if(sector_box.Contains(lVolumes[j].position_and_radius.xyz()))
			{
				lVolumes[j].sector_id = i;
				g_lightvolumes.append(lVolumes[j]);
			}
		}
	}
	EndPacifier();
	*/

	MsgWarning("Light volumes: %d\n", g_lightvolumes.numElem());
}