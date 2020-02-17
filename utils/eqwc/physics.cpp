//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics model processing
//////////////////////////////////////////////////////////////////////////////////

#include "eqwc.h"
#include "threadedprocess.h"

DkList<eqlevelphyssurf_t>	g_physicssurfs;
DkList<eqphysicsvertex_t>	g_physicsverts;
DkList<int>					g_physicsindices;

int GetMostFactor(Vector4D &factors)
{
	int nMostFactor = 0;

	for(int i = 0; i < 4; i++)
	{
		if(factors[i] > 0.25f)
			nMostFactor = i;
	}

	return nMostFactor;
}

// adds surface to physics geometry
void AddSurfaceToPhysics(cwsurface_t* pSurf)
{
	int first_vert = g_physicsverts.numElem();
	int first_index = g_physicsindices.numElem();

	// process multi-material (vertex transition surfaces)
	if(pSurf->flags & CS_FLAG_MULTIMATERIAL)
	{
		ThreadLock();
		g_physicsindices.resize(first_index + pSurf->numIndices*4);
		ThreadUnlock();

		// make four surface groups
		for(int i = 0; i < 4; i++)
		{
			DkList<int> grouptriangleindices;

			for(int j = 0; j < pSurf->numIndices; j+=3)
			{
				int i0 = pSurf->pIndices[j];
				int i1 = pSurf->pIndices[j+1];
				int i2 = pSurf->pIndices[j+2];

				// compute replacement factor
				Vector4D factors = (pSurf->pVerts[i0].vertexPaint+pSurf->pVerts[i1].vertexPaint+pSurf->pVerts[i2].vertexPaint) / 3.0f;

				if(GetMostFactor(factors) == i)
				{
					grouptriangleindices.append(i0);
					grouptriangleindices.append(i1);
					grouptriangleindices.append(i2);
				}
			}

			if(grouptriangleindices.numElem() == 0)
				continue;

			first_vert = g_physicsverts.numElem();
			first_index = g_physicsindices.numElem();

			eqlevelphyssurf_t surf;
			surf.firstindex = first_index;
			surf.firstvertex = first_vert;

			surf.numvertices = g_physicsverts.numElem();
			surf.numindices = 0;

			IMatVar* pSurfPropsVar = pSurf->pMaterial->FindMaterialVar(varargs("surfaceprops%d", i+1));

			if(!pSurfPropsVar)
				pSurfPropsVar = pSurf->pMaterial->FindMaterialVar("surfaceprops");

			

			// copy surface properties string
			if(pSurfPropsVar)
				strcpy(surf.surfaceprops, pSurfPropsVar->GetString());
			else
				strcpy(surf.surfaceprops, "default");

			ThreadLock();
			surf.material_id = g_materials.addUnique(pSurf->pMaterial);

			g_physicsindices.resize(first_index + pSurf->numIndices);
			g_physicsverts.resize(first_vert + pSurf->numVertices);

			/*
			for(int j = 0; j < grouptriangleindices.numElem(); j++)
			{
				int index = g_physicsverts.addUnique(pSurf->pVerts[grouptriangleindices[j]]);
				g_physicsindices.append(index);
			}
			*/
			DkList<eqphysicsvertex_t>	sverts;
			DkList<int>					sindices;

			sindices.resize(pSurf->numIndices);
			sverts.resize(pSurf->numVertices);

			for(int j = 0; j < grouptriangleindices.numElem(); j++)
			{
				int index = sverts.addUnique( pSurf->pVerts[grouptriangleindices[j]] );
				sindices.append(first_vert+index);
			}

			for(int j = 0; j < sindices.numElem(); j++)
				g_physicsindices.append(sindices[j]);

			for(int j = 0; j < sverts.numElem(); j++)
				g_physicsverts.append(sverts[j]);

			surf.numvertices = g_physicsverts.numElem() - surf.numvertices;
			surf.numindices = grouptriangleindices.numElem();

			g_physicssurfs.append(surf);
			ThreadUnlock();
		}
	}
	else
	{
		eqlevelphyssurf_t surf;
		surf.firstindex = first_index;
		surf.firstvertex = first_vert;

		surf.numvertices = g_physicsverts.numElem();
		surf.numindices = 0;

		IMatVar* pSurfPropsVar = pSurf->pMaterial->FindMaterialVar("surfaceprops");

		// copy surface properties string
		if(pSurfPropsVar)
			strcpy(surf.surfaceprops, pSurfPropsVar->GetString());
		else
			strcpy(surf.surfaceprops, "default");

		ThreadLock();
		surf.material_id = g_materials.addUnique( pSurf->pMaterial );

		g_physicsindices.resize(first_index + pSurf->numIndices);
		g_physicsverts.resize(first_vert + pSurf->numVertices);

		DkList<eqphysicsvertex_t>	sverts;
		DkList<int>					sindices;

		sindices.resize(pSurf->numIndices);
		sverts.resize(pSurf->numVertices);

		for(int i = 0; i < pSurf->numIndices; i++)
		{
			int index = sverts.addUnique( pSurf->pVerts[pSurf->pIndices[i]] );
			sindices.append(first_vert+index);
		}

		for(int i = 0; i < sindices.numElem(); i++)
			g_physicsindices.append(sindices[i]);

		for(int i = 0; i < sverts.numElem(); i++)
			g_physicsverts.append(sverts[i]);

		surf.numvertices = g_physicsverts.numElem() - surf.numvertices;
		surf.numindices = pSurf->numIndices;

		g_physicssurfs.append(surf);
		ThreadUnlock();
	}
}

DkList<cwsurface_t*> g_new_groups;

void AddSurfaceToPhysicsThreaded(int n)
{
	AddSurfaceToPhysics( g_new_groups[n] );
	FreeSurface( g_new_groups[n] );
}

// builds physics geometry
void BuildPhysicsGeometry()
{
	StartPacifier("PreparePhysicsSurfaces: ");
	for(int i = 0; i < g_surfaces.numElem(); i++)
	{
		UpdatePacifier((float)i / (float)g_surfaces.numElem());

		if(!(g_surfaces[i]->flags & CS_FLAG_NOCOLLIDE))
			MergeSurfaceIntoList( g_surfaces[i], g_new_groups, false );
	}
	EndPacifier();
	/*
	StartPacifier("AddSurfacesToPhysics: ");
	RunThreadsOnIndividual(g_new_groups.numElem(), true, AddSurfaceToPhysicsThreaded);
	EndPacifier();
	*/
	
	StartPacifier("AddSurfacesToPhysics: ");
	for(int i = 0; i < g_new_groups.numElem(); i++)
	{
		UpdatePacifier((float)i / (float)g_new_groups.numElem());

		AddSurfaceToPhysics( g_new_groups[i] );
		FreeSurface(g_new_groups[i]);
	}
	EndPacifier();
	
}