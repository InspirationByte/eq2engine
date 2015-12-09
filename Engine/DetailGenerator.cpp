//***********Copyright (C) Illusion Way Interactive Software 2008-2010************
//
// Description: Grass batch generator
//
//********************************************************************************

#include "model.h"
#include "IDKPhysics.h"
#include "DetailGenerator.h"
#include "ViewRenderer.h"

#define DETAILS_BOUNDARY 512 // boundary that separates all models to groups

// Vertex descriptor
typedef struct dvertexdesc_s
{
	Vector3D			point;
	Vector2D			texCoord;
	Vector3D			normal;
	Vector3D			color;
}dvertexdesc_t;

// group descriptor
struct DetailGroup_t
{
	int			numVerts;
	int			numTriangles;

	int			vertsOfs;
	int			trisOfs;

	uint8		materialIndex;

	Vector3D	boxMin;
	Vector3D	boxMax;

	inline dvertexdesc_t *pVertex( int i ) const 
	{
		return (dvertexdesc_t *)(((ubyte *)this) + vertsOfs) + i; 
	};

	inline uint32 *pIndex( int i ) const 
	{
		return (uint32 *)(((ubyte *)this) + trisOfs) + i; 
	};
};

struct DetailMaterialDesc_t
{
	char	mat_path[256];
};

struct DetailHeader_t
{
	int			numGroups;
	int			groupsOfs;

	inline DetailGroup_t *pGroup( int i ) const 
	{
		return (DetailGroup_t *)(((ubyte *)this) + groupsOfs) + i; 
	};

	int			numMaterials;
	int			materialsOfs;

	inline DetailMaterialDesc_t *pMaterial( int i ) const 
	{
		return (DetailMaterialDesc_t *)(((ubyte *)this) + numMaterials) + i; 
	};
};

struct detailmodel_t
{
	char	model[256];

	int		mod_index;

	float	scale;
	float	percentage;
};

struct detailtype_t
{
	char			name[128];

	float			fillRatio;

	detailmodel_t*	models;
	int				numModels;
};

DkList<detailtype_t*>	g_DetailTypes;

struct detail_t
{
	IEngineModel*	model;

	Vector3D		position;
	Vector3D		angles;
	float			scale;
};

DkList<detail_t*>	g_DetailList;

detailtype_t* GetDetailTypeByStr(const char* type)
{
	for(int i = 0; i < g_DetailTypes.numElem(); i++)
	{
		if(!stricmp(g_DetailTypes[i]->name, type))
			return g_DetailTypes[i];
	}

	return NULL;
}

void TraceDetailsOnSurface(bsphwsurface_t* surf)
{
	detailtype_t* dType = NULL;

	// get material of this surface
	IMaterial* pMaterial = surf->pSurfaceMaterial;
	if(!pMaterial)
		return;

	// find detail type
	IMatVar* detTypeVar = pMaterial->FindMaterialVar("detailtype");
	if(!detTypeVar)
		return;

	// resolve detail type
	dType = GetDetailTypeByStr(detTypeVar->GetString());
	if(!dType)
	{
		MsgInfo("Type %s not found.\n", detTypeVar->GetString());
		return;
	}

	// compute how many details will be put on surface using bounds
	float fBoxSize = length(surf->max - surf->min);
	int numDetails = fBoxSize*dType->fillRatio;

	// if all these stuff is found, do tracing in surf->min and surf->max bounds
	for(int i = 0; i < numDetails; i++)
	{
		// randomize vertor
		Vector3D		randomized(RandomFloat(surf->min.x,surf->max.x), surf->max.y+10,RandomFloat(surf->min.z,surf->max.z));
		// get best model
		detailmodel_t	detailModel = dType->models[RandomInt(0, dType->numModels-1)];

		// trace down, world-only
		internaltrace_t trace;
		physics->InternalTraceLine(randomized, randomized - Vector3D(0,1024,0), COLLISION_GROUP_WORLDGEOMETRY, &trace);

		// if we collide
		if(trace.fraction < 1)
		{
			// check hit normal
			if(dot(trace.normal, Vector3D(0,1,0)) < 0.2f)
				continue;

			extern BSP* s_pBSP;

			if(s_pBSP->GetLeafFromPosition(trace.traceEnd) == -1)
				continue;


			// check material
			if(pMaterial != trace.m_pHitMaterial)
			{
				// retrace
				continue;
			}

			// create new model
			detail_t* m_detail = new detail_t;

			m_detail->angles = Vector3D(0,RandomFloat(-360,360),0);
			m_detail->position = trace.traceEnd;
			m_detail->scale = detailModel.scale;

			detailModel.mod_index = g_pModelCache->PrecacheModel(detailModel.model);

			m_detail->model = g_pModelCache->GetModel(detailModel.mod_index);

			// add model to list
			g_DetailList.append(m_detail);
		}
	}
}

void TraceLeafForDetails(bsphwleaf_t* leaf)
{
	for(int i = 0; i < leaf->numFaces; i++)
	{
		TraceDetailsOnSurface(leaf->firstsurface[i]);
	}
}

void WalkNodeForDetails(int nnode, enginebspnode_t *nodes, bsphwleaf_t* leaves)
{
	enginebspnode_t*	node;
	bsphwleaf_t*		leaf;

	if(nnode < 0) 
	{
		leaf = &leaves[~nnode];

		//we are in a leaf, check it's surfaces, and trace for details
		TraceLeafForDetails(leaf);
		return;
	}

	node = &nodes[nnode];

	WalkNodeForDetails(node->children[0], nodes, leaves);
	WalkNodeForDetails(node->children[1], nodes, leaves);
}

bool det_loaded = false;

bool LoadDetailInfo()
{
	if(det_loaded)
		return true;

	KeyValues kv;
	if(kv.LoadFromFile("DetailTypes.txt"))
	{
		kvsection_t *sec = kv.FindRootSection("DetailTypes");

		if(!sec)
			return false;

		for(int i = 0; i < sec->sections.numElem(); i++)
		{
			kvsection_t* det_sec = sec->sections[i];

			detailtype_t* dNewType = new detailtype_t;
			g_DetailTypes.append(dNewType);

			strcpy(dNewType->name, det_sec->sectionname);

			kvkeyvaluepair_t* key = NULL;
			
			key = KeyValues_FindKey(det_sec, "fillRatio");
			if(key)
				dNewType->fillRatio = atof(key->value);
			else
				dNewType->fillRatio = 100;

			dNewType->numModels = det_sec->sections.numElem();
			dNewType->models = new detailmodel_t[dNewType->numModels];

			for(int j = 0; j < det_sec->sections.numElem(); j++)
			{
				kvsection_t* type_sec = det_sec->sections[j];

				key = KeyValues_FindKey(type_sec, "model");
				if(key)
					strcpy(dNewType->models[j].model, key->value);
				else
					strcpy(dNewType->models[j].model, "models/error.egf");

				dNewType->models[j].mod_index = g_pModelCache->PrecacheModel(dNewType->models[j].model);

				key = KeyValues_FindKey(type_sec, "scale");
				if(key)
					dNewType->models[i].scale = atof(key->value);
				else
					dNewType->models[i].scale = 1.0f;

				key = KeyValues_FindKey(type_sec, "percentage");
				if(key)
					dNewType->models[i].percentage = atof(key->value);
				else
					dNewType->models[i].percentage = 0.5f;
			}
		}
	}
	else
	{
		return false;
	}

	det_loaded = true;

	return true;
}

// uses bsp for detail generation
void GenerateDetails(const char* mapName, enginebspnode_t *nodes, bsphwleaf_t* leaves)
{
	if(!LoadDetailInfo())
	{
		MsgError("Can't load DetailTypes.txt! Generation aborted.");
		return;
	}

	MsgInfo("Generating details...\n");

	// walk on the all nodes
	WalkNodeForDetails(0, nodes, leaves);
}

void DestroyDetails()
{
	g_DetailList.clear();
}

void DrawDetails()
{
	for(int i = 0; i < g_DetailList.numElem(); i++)
	{
		if(length(g_DetailList[i]->position - scenerenderer->GetView()->GetOrigin()) > 950)
			continue;

		if(!scenerenderer->GetViewFrustum()->IsBoxInside(g_DetailList[i]->position.x-40,g_DetailList[i]->position.x+40,
															g_DetailList[i]->position.y-40,g_DetailList[i]->position.y+40,
															g_DetailList[i]->position.z-40,g_DetailList[i]->position.z+40)
															)
		continue;

		scenerenderer->DrawModel(g_DetailList[i]->model, g_DetailList[i]->position, g_DetailList[i]->angles, Vector3D(g_DetailList[i]->scale), 0);
	}
}