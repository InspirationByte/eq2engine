//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Decal generator of EQWC
//////////////////////////////////////////////////////////////////////////////////

#include "eqwc.h"
#include "threadedprocess.h"

DkList<cwdecal_t*> g_decals;

void GenerateDecals()
{
	//Sleep(6000);
	/*
	StartPacifier("GenerateDecalGeom: ");
	RunThreadsOnIndividual(g_decals.numElem(), true, GenerateDecalGeometry);
	EndPacifier();
	*/

	// make entity decals
	for(int i = 0; i < g_decals.numElem(); i++)
	{
		cwentity_t* pEnt = new cwentity_t;
		pEnt->layer_index = 0; // g_decals[i]->layer_idx;

		ekeyvalue_t pair;

		strcpy(pair.name, "classname");
		strcpy(pair.value, "info_worlddecal");
		pEnt->keys.append(pair);

		strcpy(pair.name, "material");
		strcpy(pair.value, g_decals[i]->pMaterial->GetName());
		pEnt->keys.append(pair);

		strcpy(pair.name, "origin");
		strcpy(pair.value, varargs("%g %g %g", g_decals[i]->origin.x, g_decals[i]->origin.y, g_decals[i]->origin.z));
		pEnt->keys.append(pair);

		strcpy(pair.name, "size");
		strcpy(pair.value, varargs("%g %g %g", g_decals[i]->size.x, g_decals[i]->size.y, g_decals[i]->size.z));
		pEnt->keys.append(pair);

		strcpy(pair.name, "normal");
		strcpy(pair.value, varargs("%g %g %g", g_decals[i]->projaxis.x, g_decals[i]->projaxis.y, g_decals[i]->projaxis.z));
		pEnt->keys.append(pair);

		strcpy(pair.name, "projected");
		strcpy(pair.value, varargs("%d", g_decals[i]->projectcoord));
		pEnt->keys.append(pair);
		
		strcpy(pair.name, "texscale");
		strcpy(pair.value, varargs("%g %g", g_decals[i]->texscale.x, g_decals[i]->texscale.y));
		pEnt->keys.append(pair);

		strcpy(pair.name, "texrotate");
		strcpy(pair.value, varargs("%g", g_decals[i]->rotate));
		pEnt->keys.append(pair);

		g_entities.append( pEnt );
	}

	Msg("Added %d decals as info_worlddecal\n", g_decals.numElem());
}