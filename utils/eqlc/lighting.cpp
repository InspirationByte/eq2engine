//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct lighting lightmap calculation
//////////////////////////////////////////////////////////////////////////////////

#include "eqlc.h"
#include "BaseRenderableObject.h"
#include "imaging/ImageLoader.h"

DkList<dlight_t*>				g_singlelights;
DkList<dlight_t*>				g_lights;
DkList<dlight_t*>				g_rad_lights;
DkList<eqlevellight_t>			g_levellights;

//DkList<ITexture*>				g_pLightmapTexturesWire;
DkList<ITexture*>				g_pLightmapTextures;
DkList<ITexture*>				g_pDirmapTextures;
ITexture*						g_pTempLightmap;
ITexture*						g_pTempLightmap2;

IMaterial*						g_lightmaterials[3][2][2];
IMaterial*						g_lightmapPixelCorrection;
IMaterial*						g_lightmapWireCombine;

renderAreaList_t*				g_pRenderArea = NULL;

#define POINTLIGHTCLASS			"light_point"
#define SPOTLIGHTCLASS			"light_spot"
#define SUNLIGHTCLASS			"light_sun"
#define STATICCLASS				"prop_static"

Vector2D UTIL_StringToVector2(const char *str)
{
	Vector2D vec(0,0);

	if(str)
		sscanf(str, "%f %f", &vec.x, &vec.y);

	return vec;
}

ColorRGB UTIL_StringToColor3(const char *str)
{
	ColorRGB col(0,0,0);

	if(str)
	{
		sscanf(str, "%f %f %f", &col.x, &col.y, &col.z);
	}

	return col;
}

ColorRGBA UTIL_StringToColor4(const char *str)
{
	ColorRGBA col(0,0,0,1);

	if(str)
		sscanf(str, "%f %f %f %f", &col.x, &col.y, &col.z, &col.w);

	return col;
}

typedef void (*LIGHTPARAMS)(CViewParams* pView, dlight_t* pLight, int &nDrawFlags);

void ComputePointLightParams(CViewParams* pView, dlight_t* pLight, int &nDrawFlags)
{
	nDrawFlags |= VR_FLAG_CUBEMAP;

	pView->SetOrigin(pLight->position);
	pView->SetAngles(vec3_zero);
	pView->SetFOV(90);
}

void ComputeSpotlightParams(CViewParams* pView, dlight_t* pLight, int &nDrawFlags)
{
	pView->SetOrigin(pLight->position);
	pView->SetAngles(pLight->angles);
	pView->SetFOV(pLight->fovWH.z);

	if(!(pLight->nFlags & LFLAG_MATRIXSET))
	{
		Matrix4x4 spot_proj = perspectiveMatrixY(DEG2RAD(pLight->fovWH.z), pLight->fovWH.x,pLight->fovWH.y, 1, pLight->radius.x);
		Matrix4x4 spot_view = !rotateXYZ4(DEG2RAD(pLight->angles.x),DEG2RAD(pLight->angles.y),DEG2RAD(pLight->angles.z));
		pLight->lightRotation = transpose(spot_view.getRotationComponent());
		spot_view.translate(-pLight->position);

		pLight->lightWVP = spot_proj*spot_view;
	}

	pLight->nFlags |= LFLAG_MATRIXSET;
}

void ComputeSunlightParams(CViewParams* pView, dlight_t* pLight, int &nDrawFlags)
{
	nDrawFlags |= VR_FLAG_ORTHOGONAL;
	pView->SetOrigin(pLight->position);
	pView->SetAngles(pLight->angles);
	pView->SetFOV(90);

	/*
	if(!(pLight->nFlags & LFLAG_MATRIXSET))
	{
		Matrix4x4 sun_proj = orthoMatrix(-pLight->fovWH.x,pLight->fovWH.x,-pLight->fovWH.y,pLight->fovWH.y, pLight->radius.x, pLight->radius.y);
		Matrix4x4 sun_view = rotateZXY(DEG2RAD(-pLight->angles.x),DEG2RAD(-pLight->angles.y),DEG2RAD(-pLight->angles.z));
		sun_view.translate(-pLight->position);
		pLight->lightWVP = sun_proj*sun_view;
	}
	*/

	
	pLight->nFlags |= LFLAG_MATRIXSET;
}

static LIGHTPARAMS pParamFuncs[DLT_COUNT] =
{
	ComputePointLightParams,
	ComputeSpotlightParams,
	ComputeSunlightParams
};

void ComputeLightViewsAndFlags(CViewParams* pView, dlight_t* pLight, int &nDrawFlags)
{
	pParamFuncs[pLight->nType](pView,pLight,nDrawFlags);
	pLight->lightBoundMin = pLight->position - Vector3D(pLight->radius.x);
	pLight->lightBoundMax = pLight->position + Vector3D(pLight->radius.x);
	nDrawFlags |= VR_FLAG_NO_MATERIALS;
}

bool ParseLightEntity_Point(kvkeybase_t* pSection)
{
	bool bDoLightmap = KV_GetValueBool(pSection->FindKeyBase("lightmap"), 0, false);
	bool bDoDynamic = KV_GetValueBool(pSection->FindKeyBase("dynamic"), 0, true);

	if(!bDoLightmap)
		return bDoDynamic;

	dlight_t* pLight = viewrenderer->AllocLight();
	pLight->nType = DLT_OMNIDIRECTIONAL;

	pLight->position = UTIL_StringToColor3(KV_GetValueString(pSection->FindKeyBase("origin")));
	pLight->color = UTIL_StringToColor3(KV_GetValueString(pSection->FindKeyBase("color"), 0, "1 1 1"));
	pLight->intensity = KV_GetValueFloat(pSection->FindKeyBase("intensity"), 0, 1.0f);
	pLight->radius.x = KV_GetValueFloat(pSection->FindKeyBase("radius"), 0, 300.0f);

	if(!KV_GetValueBool(pSection->FindKeyBase("shadows"),0, true))
		pLight->nFlags |= LFLAG_NOSHADOWS;

	kvkeybase_t *pair = pSection->FindKeyBase("mask");

	if(pair)
	{
		pLight->pMaskTexture = g_pShaderAPI->LoadTexture(pair->values[0],TEXFILTER_TRILINEAR_ANISO,TEXADDRESS_CLAMP,0);
		pLight->pMaskTexture->Ref_Grab();
	}

	eqlevellight_t level_light;
	level_light.type = DLT_OMNIDIRECTIONAL;
	level_light.origin = pLight->position;
	level_light.radius = pLight->radius.x;
	level_light.fov = -1;
	level_light.worldmatrix = identity4();
	level_light.flags = 0;

	// build light flags
	if(pLight->nFlags & LFLAG_NOSHADOWS)
		level_light.flags |= EQLEVEL_LIGHT_NOSHADOWS;

	if(bDoLightmap)
		level_light.flags |= EQLEVEL_LIGHT_LMLIGHT;

	if(bDoDynamic)
		level_light.flags |= EQLEVEL_LIGHT_DLIGHT;

	bool bNoTrace = KV_GetValueBool(pSection->FindKeyBase("notrace"), 0, true);

	if(bNoTrace)
		level_light.flags |= EQLEVEL_LIGHT_NOTRACE;

	level_light.color = pLight->color*pLight->intensity;

	g_levellights.append(level_light);

	int fuzzy = KV_GetValueInt(pSection->FindKeyBase("fuzzy"), 0, 0);

	float fuzzy_radius = KV_GetValueFloat(pSection->FindKeyBase("fuzzyeps"), 0, pLight->radius.x * 0.01f);

	if(fuzzy == 0)
	{
		fuzzy = 1;
		fuzzy_radius = 0.0f;
	}

	float fuzzy_intensity = 1.0f / fuzzy;

	g_singlelights.append( pLight );

	for(int i = 0; i < fuzzy; i++)
	{
		dlight_t* pFLight = viewrenderer->AllocLight();
		*pFLight = *pLight;
		pFLight->intensity*= fuzzy_intensity;
		pFLight->position = pLight->position + Vector3D(RandomFloat(-fuzzy_radius,fuzzy_radius),RandomFloat(-fuzzy_radius,fuzzy_radius),RandomFloat(-fuzzy_radius,fuzzy_radius));

		g_lights.append(pFLight);
	}

	//g_lights.append( pLight );

	return bDoDynamic;
}

bool ParseLightEntity_Spot(kvkeybase_t* pSection)
{
	bool bDoLightmap = KV_GetValueBool(pSection->FindKeyBase("lightmap"), 0, false);
	bool bDoDynamic = KV_GetValueBool(pSection->FindKeyBase("dynamic"), 0, true);

	if(!bDoLightmap)
		return bDoDynamic;

	dlight_t* pLight = viewrenderer->AllocLight();
	pLight->nType = DLT_SPOT;

	pLight->angles = UTIL_StringToColor3(KV_GetValueString(pSection->FindKeyBase("angles")));
	pLight->position = UTIL_StringToColor3(KV_GetValueString(pSection->FindKeyBase("origin")));
	pLight->color = UTIL_StringToColor3(KV_GetValueString(pSection->FindKeyBase("color"), 0, "1 1 1"));
	pLight->radius.x = KV_GetValueFloat(pSection->FindKeyBase("radius"), 0, 300.0f);
	pLight->fovWH = KV_GetValueFloat(pSection->FindKeyBase("fov"), 0, 90.0f);
	pLight->intensity = KV_GetValueFloat(pSection->FindKeyBase("intensity"), 0, 1.0f);

	if(!KV_GetValueBool(pSection->FindKeyBase("shadows"),0, true))
		pLight->nFlags |= LFLAG_NOSHADOWS;

	kvkeybase_t *pair = pSection->FindKeyBase("mask");

	if(pair)
	{
		pLight->pMaskTexture = g_pShaderAPI->LoadTexture(pair->values[0],TEXFILTER_TRILINEAR_ANISO,TEXADDRESS_CLAMP,0);
		pLight->pMaskTexture->Ref_Grab();
	}

	eqlevellight_t level_light;
	level_light.type = DLT_OMNIDIRECTIONAL;
	level_light.origin = pLight->position;
	level_light.radius = pLight->radius.x;
	level_light.fov = pLight->fovWH.z;

	// build world matrix
	Matrix4x4 spot_proj = perspectiveMatrixY(DEG2RAD(pLight->fovWH.z), pLight->fovWH.x,pLight->fovWH.y, 1, pLight->radius.x);
	Matrix4x4 spot_view = !rotateXYZ4(DEG2RAD(pLight->angles.x),DEG2RAD(pLight->angles.y),DEG2RAD(pLight->angles.z));
	pLight->lightRotation = transpose(spot_view.getRotationComponent());
	spot_view.translate(-pLight->position);

	level_light.worldmatrix  = spot_proj*spot_view;

	level_light.flags = 0;

	// build light flags
	if(pLight->nFlags & LFLAG_NOSHADOWS)
		level_light.flags |= EQLEVEL_LIGHT_NOSHADOWS;

	if(bDoLightmap)
		level_light.flags |= EQLEVEL_LIGHT_LMLIGHT;

	if(bDoDynamic)
		level_light.flags |= EQLEVEL_LIGHT_DLIGHT;

	level_light.color = pLight->color*pLight->intensity;

	g_levellights.append(level_light);

	int fuzzy = KV_GetValueInt(pSection->FindKeyBase("fuzzy"), 0, 0);

	float fuzzy_radius = KV_GetValueFloat(pSection->FindKeyBase("fuzzyeps"), 0, pLight->radius.x * 0.01f);

	if(fuzzy == 0)
	{
		fuzzy = 1;
		fuzzy_radius = 0.0f;
	}

	float fuzzy_intensity = 1.0f / fuzzy;

	g_singlelights.append( pLight );

	for(int i = 0; i < fuzzy; i++)
	{
		dlight_t* pFLight = viewrenderer->AllocLight();
		*pFLight = *pLight;
		pFLight->intensity*= fuzzy_intensity;
		pFLight->position = pLight->position + Vector3D(RandomFloat(-fuzzy_radius,fuzzy_radius),RandomFloat(-fuzzy_radius,fuzzy_radius),RandomFloat(-fuzzy_radius,fuzzy_radius));

		g_lights.append(pFLight);
	}

	return bDoDynamic;
}

bool ParseLightEntity_Sun(kvkeybase_t* pSection)
{
	bool bDoLightmap = KV_GetValueBool(pSection->FindKeyBase("lightmap"), false);
	bool bDoDynamic = KV_GetValueBool(pSection->FindKeyBase("dynamic"), true);

	if(!bDoLightmap)
		return bDoDynamic;

	dlight_t* pLight = viewrenderer->AllocLight();
	pLight->nType = DLT_SUN;

	g_lights.append(pLight);

	

	return bDoDynamic;
}

class CPropModel : public CBaseRenderableObject
{
public:
	CPropModel()
	{
		m_origin = vec3_zero;
		m_angles = vec3_zero;
		m_pModel = NULL;
	}

	void LoadModel(const char* pszFileName, Vector3D &origin, Vector3D &angles)
	{
		int cIndex = g_pModelCache->PrecacheModel(pszFileName);
		m_pModel = g_pModelCache->GetModel(cIndex);

		m_origin = origin;
		m_angles = angles;
	}

	// min bbox dimensions
	Vector3D GetBBoxMins()
	{
		return m_pModel->GetBBoxMins();
	}

	// max bbox dimensions
	Vector3D GetBBoxMaxs()
	{
		return m_pModel->GetBBoxMaxs();
	}

	// returns world transformation of this object
	Matrix4x4 GetRenderWorldTransform()
	{
		return ComputeWorldMatrix(m_origin, m_angles, Vector3D(1));
	}

	void Render(int nViewRenderFlags)
	{
		// render if in frustum
		Matrix4x4 wvp;
		Matrix4x4 world = GetRenderWorldTransform();

		materials->SetMatrix(MATRIXMODE_WORLD, GetRenderWorldTransform());

		if(materials->GetLight() && materials->GetLight()->nFlags & LFLAG_MATRIXSET)
			wvp = materials->GetLight()->lightWVP * world;
		else 
			materials->GetWorldViewProjection(wvp);

		Volume frustum;
		frustum.LoadAsFrustum(wvp);

		Vector3D mins = GetBBoxMins();
		Vector3D maxs = GetBBoxMaxs();

		if(!frustum.IsBoxInside(mins.x,maxs.x,mins.y,maxs.y,mins.z,maxs.z))
			return;

		//m_pModel->PrepareForSkinning(m_BoneMatrixList);

		studiohdr_t* pHdr = m_pModel->GetHWData()->studio;
		int nLod = 0;

		for(int i = 0; i < pHdr->numBodyGroups; i++)
		{
			int nLodableModelIndex = pHdr->pBodyGroups(i)->lodModelIndex;
			int nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];

			while(nLod > 0 && nModDescId != -1)
			{
				nLod--;
				nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];
			}

			if(nModDescId == -1)
				continue;

			for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numGroups; j++)
			{
				viewrenderer->DrawModelPart(m_pModel, nModDescId, j, nViewRenderFlags, NULL, NULL);
			}
		}
	}
private:

	IEqModel* m_pModel;
	Vector3D m_origin, m_angles;
};

CBasicRenderList g_modelrenderlist;

void LoadEntityList()
{
	for( int i = 0; i < g_pLevel->m_EntityKeyValues.GetRootSection()->keys.numElem(); i++ )
	{
		kvkeybase_t* pEntity = g_pLevel->m_EntityKeyValues.GetRootSection()->keys[i];
		if(!pEntity->keys.numElem())
			continue;

		kvkeybase_t* pPair = pEntity->FindKeyBase("classname");
		if(pPair)
		{
			bool bResult = true;

			if(!stricmp(pPair->values[0], POINTLIGHTCLASS))
				bResult = ParseLightEntity_Point(pEntity);
			else if(!stricmp(pPair->values[0], SPOTLIGHTCLASS))
				bResult = ParseLightEntity_Spot(pEntity);
			//else if(!stricmp(pPair->values[0], SUNLIGHTCLASS))
			//	bResult = ParseLightEntity_Point(pEntity);
			else if(!stricmp(pPair->values[0], STATICCLASS))
			{
				bResult = true;
				kvkeybase_t* pPairModel = pEntity->FindKeyBase("model");

				if(pPairModel)
				{
					CPropModel* pModel = new CPropModel();

					kvkeybase_t* pOriginKey = pEntity->FindKeyBase("origin");
					kvkeybase_t* pAnglesKey = pEntity->FindKeyBase("angles");

					Vector3D origin, angles;
					origin = UTIL_StringToColor3(KV_GetValueString(pOriginKey, 0, "0 0 0"));
					angles = UTIL_StringToColor3(KV_GetValueString(pAnglesKey, 0, "0 0 0"));
				
					pModel->LoadModel(pPairModel->values[0], origin, angles);
					g_modelrenderlist.AddRenderable(pModel);
				}
			}

			if(!bResult)
			{
				g_pLevel->m_EntityKeyValues.GetRootSection()->keys.removeIndex(i);
				i--;
				delete pEntity;
			}
		}
	}
}

void RenderLight(dlight_t* pLight, int lm_id, bool bDirectionMap = false)
{
	// set current light
	materials->SetLight(pLight);

	bool bDoShadows = !(pLight->nFlags & LFLAG_NOSHADOWS);

	CViewParams light_view(Vector3D(0,0,0),Vector3D(0),90);

	int nRenderFlags = VR_FLAG_SKIPVISUPDATE;
	ComputeLightViewsAndFlags(&light_view, pLight, nRenderFlags);

	viewrenderer->SetView( &light_view );

	// light our volumes

	viewrenderer->UpdateAreaVisibility( nRenderFlags );

	if(bDoShadows && !bDirectionMap)
	{
		viewrenderer->SetDrawMode(VDM_SHADOW);

		sceneinfo_t lightSceneInfo;
		lightSceneInfo.m_cAmbientColor = vec3_zero;
		lightSceneInfo.m_fZNear = 1.0f;
		lightSceneInfo.m_fZFar = pLight->radius.x;

		viewrenderer->SetSceneInfo(lightSceneInfo);

		// draw without wireframe
		materials->GetConfiguration().wireframeMode = false;

		g_pShaderAPI->ResetCounters();

		// draw world without translucent objects
		viewrenderer->DrawWorld( nRenderFlags | VR_FLAG_NO_TRANSLUCENT | VR_FLAG_NO_MATERIALS );


		g_pShaderAPI->ResetCounters();

		// draw static props
		viewrenderer->DrawRenderList(&g_modelrenderlist, nRenderFlags | VR_FLAG_NO_TRANSLUCENT);
	}

	viewrenderer->SetupRenderTargetToBackbuffer();

	viewrenderer->SetViewportSize(g_pLevel->m_nLightmapSize*4, g_pLevel->m_nLightmapSize*4);
	viewrenderer->SetLightmapTextureIndex(lm_id);

	// change mode to lighting
	materials->Setup2D(1,1);

	if(bDirectionMap)
		viewrenderer->SetupRenderTargets(1, &g_pDirmapTextures[lm_id], NULL);
	else
		viewrenderer->SetupRenderTargets(1, &g_pLightmapTextures[lm_id], NULL);

	g_pShaderAPI->Clear(false,true,false,ColorRGBA(0,0,0,0));

	// draw without wireframe
	materials->GetConfiguration().wireframeMode = false;

	// bind forward lighting material
	materials->BindMaterial(g_lightmaterials[pLight->nType][(int)bDoShadows][(int)bDirectionMap], false);

	viewrenderer->SetDrawMode(VDM_LIGHTING);

	viewrenderer->SetView(&light_view);

	viewrenderer->UpdateAreaVisibility( nRenderFlags );

	g_pShaderAPI->ResetCounters();

	// render scene with applying current light and store result in lightmap
	viewrenderer->DrawWorld(VR_FLAG_NO_MATERIALS);
}

void InitLightingShaders()
{
	viewrenderer->InitializeResources();

	g_pRenderArea = eqlevel->CreateRenderAreaList();
	viewrenderer->SetAreaList(g_pRenderArea);

	g_lightmaterials[DLT_OMNIDIRECTIONAL][0][0] = materials->GetMaterial("eqlc/pointlight");
	g_lightmaterials[DLT_OMNIDIRECTIONAL][1][0] = materials->GetMaterial("eqlc/pointlight_shadow");

	g_lightmaterials[DLT_SPOT][0][0] = materials->GetMaterial("eqlc/spotlight");
	g_lightmaterials[DLT_SPOT][1][0] = materials->GetMaterial("eqlc/spotlight_shadow");

	g_lightmaterials[DLT_SUN][0][0] = materials->GetMaterial("eqlc/sunlight");
	g_lightmaterials[DLT_SUN][1][0] = materials->GetMaterial("eqlc/sunlight_shadow");

	// make the direction shaders

	g_lightmaterials[DLT_OMNIDIRECTIONAL][0][1] = materials->GetMaterial("eqlc/pointlight_dir");
	g_lightmaterials[DLT_OMNIDIRECTIONAL][1][1] = materials->GetMaterial("eqlc/pointlight_shadow_dir");

	g_lightmaterials[DLT_SPOT][0][1] = materials->GetMaterial("eqlc/spotlight_dir");
	g_lightmaterials[DLT_SPOT][1][1] = materials->GetMaterial("eqlc/spotlight_shadow_dir");

	g_lightmaterials[DLT_SUN][0][1] = materials->GetMaterial("eqlc/sunlight_dir");
	g_lightmaterials[DLT_SUN][1][1] = materials->GetMaterial("eqlc/sunlight_shadow_dir");

	g_lightmapPixelCorrection = materials->GetMaterial("eqlc/lightmapfilter");
	g_lightmapWireCombine = materials->GetMaterial("eqlc/lightmapwirecombine");

	g_pTempLightmap = g_pShaderAPI->CreateRenderTarget(g_pLevel->m_nLightmapSize,g_pLevel->m_nLightmapSize,FORMAT_RGB8,TEXFILTER_TRILINEAR_ANISO);
	g_pTempLightmap->Ref_Grab();
	//g_pTempLightmap2 = g_pShaderAPI->CreateRenderTarget(g_pLevel->m_nLightmapSize,g_pLevel->m_nLightmapSize,FORMAT_RGBA8,TEXFILTER_NEAREST);
	//g_pTempLightmap2->Ref_Grab();
}

void AddLump(int nLump, ubyte *pData, int nDataSize, eqworldhdr_t* pHdr, IFile* pFile)
{
	eqworldlump_t lump;
	lump.data_type = nLump;
	lump.data_size = nDataSize;

	// write lump hdr
	g_fileSystem->Write(&lump, 1, sizeof(eqworldlump_t), pFile);

	// write lump data
	g_fileSystem->Write(pData, 1, nDataSize, pFile);


	pHdr->num_lumps++;
	Msg("lumps: %d\n", pHdr->num_lumps);
}


// writes key-values section.
void KV_WriteToFile_r(kvkeybase_t* pKeyBase, IFile* pFile, int nTabs = 0, bool bOldFormat = false);

void AddKVLump(int nLump, KeyValues &kv, eqworldhdr_t* pHdr, IFile* pFile)
{
	int begin_offset = g_fileSystem->Tell(pFile);

	eqworldlump_t lump;
	lump.data_type = nLump;
	lump.data_size = 0;

	// write lump hdr
	g_fileSystem->Write(&lump, 1, sizeof(eqworldlump_t), pFile);

	int data_pos = g_fileSystem->Tell(pFile);

	// write, without tabs
	KV_WriteToFile_r(kv.GetRootSection(), pFile, 0, false);

	int cur_pos = g_fileSystem->Tell(pFile);

	g_fileSystem->Seek(pFile, begin_offset, SEEK_SET);

	lump.data_size = cur_pos - data_pos;

	// write lump hdr agian
	g_fileSystem->Write(&lump, 1, sizeof(eqworldlump_t), pFile);

	// move forward
	g_fileSystem->Seek(pFile, cur_pos, SEEK_SET);

	pHdr->num_lumps++;
}

void SaveWorld()
{
	EqString world_geom_path(varargs("worlds/%s/world.build", g_worldName.GetData()));

	ubyte* pLevelData = (ubyte*)g_fileSystem->GetFileBuffer(world_geom_path.GetData(), 0, -1, true);

	if(!pLevelData)
		return;

	eqworldhdr_t* pHdr = (eqworldhdr_t*)pLevelData;
	if(pHdr->ident != EQWF_IDENT)
	{
		MsgError("ERROR: Invalid %s world file\n", g_worldName.GetData());
		PPFree(pLevelData);
		return;
	}

	if(pHdr->version < EQWF_VERSION)
	{
		MsgError("ERROR: Invalid %s world version, please rebuild fully\n", g_worldName.GetData());
		PPFree(pLevelData);
		return;
	}

	if(pHdr->version > EQWF_VERSION)
	{
		MsgError("ERROR: Invalid %s world version, please update SDK\n", g_worldName.GetData());
		PPFree(pLevelData);
		return;
	}

	ubyte* pLump = ((ubyte*)pHdr + sizeof(eqworldhdr_t));

	DkList<eqworldlump_t*> lumps;

	Msg("Reading original world lumps (%d)...\n", pHdr->num_lumps);
	// rescue all lumps excepting the entity lump
	for(int i = 0; i < pHdr->num_lumps; i++)
	{
		eqworldlump_t* lump_data = (eqworldlump_t*)pLump;

		int data_size = lump_data->data_size;

		if( (lump_data->data_type != EQWLUMP_ENTITIES) &&
			(lump_data->data_type != EQWLUMP_LIGHTS))
		{
			Msg("read %d lump\n", lump_data->data_type);
			lumps.append(lump_data);
		}

		// go to next lump
		pLump += sizeof(eqworldlump_t)+data_size;
	}

	// write base file
	eqworldhdr_t worldhdr;
	worldhdr.ident = EQWF_IDENT;
	worldhdr.version = EQWF_VERSION;

	worldhdr.num_lumps = 0;

	IFile* pFile = g_fileSystem->Open(world_geom_path.GetData(), "wb", SP_MOD);

	if(pFile)
	{
		MsgWarning("Writing %s... ", world_geom_path.GetData());

		g_fileSystem->Write(&worldhdr, 1, sizeof(worldhdr),pFile);

		for(int i = 0; i < lumps.numElem(); i++)
		{
			Msg("Adding %d lump\n", lumps[i]->data_type);
			AddLump( lumps[i]->data_type, ((ubyte*)lumps[i])+sizeof(eqworldlump_t),lumps[i]->data_size,&worldhdr, pFile);
		}

		AddLump( EQWLUMP_LIGHTS, (ubyte*)g_levellights.ptr(), g_levellights.numElem()*sizeof(eqlevellight_t),&worldhdr, pFile);

		// write new entities
		AddKVLump( EQWLUMP_ENTITIES, g_pLevel->m_EntityKeyValues, &worldhdr, pFile);

		// seek back
		g_fileSystem->Seek(pFile, 0, SEEK_SET);

		// rewrite hdr
		g_fileSystem->Write(&worldhdr, 1, sizeof(worldhdr),pFile);
		Msg("lumps: %d\n", worldhdr.num_lumps);

		g_fileSystem->Close(pFile);

		MsgWarning("Done\n");
	}
	else
		MsgError("ERROR! Cannot open file %s for writing\n", world_geom_path.GetData());

	PPFree(pLevelData);
}

void FixEdgesAndSave(int nLightmap, bool bDirMap)
{
	MsgInfo("Fixing lightmap on edges...\n");
	//CombineLightmap(lm_id);

	g_pShaderAPI->Reset();

	g_pShaderAPI->ChangeRenderTarget(g_pTempLightmap,0,NULL);

	g_pShaderAPI->Clear(true,true,false,ColorRGBA(0,0,0,1));

	materials->BindMaterial( g_lightmapPixelCorrection, false );

	if(bDirMap)
		g_pShaderAPI->SetTexture( g_pDirmapTextures[nLightmap],	0 );
	else
		g_pShaderAPI->SetTexture( g_pLightmapTextures[nLightmap],	0 );

	materials->SetRasterizerStates(CULL_NONE);

	Vertex2D_t verts[] = {MAKETEXQUAD(0,0,1,1,0)};
			
	materials->Setup2D(1,1);

	materials->Apply();

	// render quad
	IMeshBuilder* pMB = g_pShaderAPI->CreateMeshBuilder();

	if(pMB)
	{
		pMB->Begin(PRIM_TRIANGLE_STRIP);

		for(int i = 0; i < 4; i++)
		{
			pMB->Color4fv(color4_white);
			pMB->TexCoord2f(verts[i].texCoord.x, verts[i].texCoord.y);
			pMB->Position3f(verts[i].position.x, verts[i].position.y, 0);

			pMB->AdvanceVertex();
		}

		pMB->End();
	}

	g_pShaderAPI->DestroyMeshBuilder(pMB);

	EqString fileName;
	
	if(bDirMap)
		fileName = varargs("%s/worlds/%s/dm#%d.dds", g_fileSystem->GetCurrentGameDirectory(), g_pLevel->m_levelpath.GetData(), nLightmap);
	else
		fileName = varargs("%s/worlds/%s/lm#%d.dds", g_fileSystem->GetCurrentGameDirectory(), g_pLevel->m_levelpath.GetData(), nLightmap);

	MsgInfo("Saving %s...\n", fileName.GetData());
		
	texlockdata_t lockdata;
		
	// lock lightmap
	g_pTempLightmap->Lock( &lockdata, NULL, false, true, 0, 0 );

	// save to image
	CImage img;
	ubyte* pData = img.Create(FORMAT_RGBA8, g_pTempLightmap->GetWidth(), g_pTempLightmap->GetHeight(), 1, 1);

	memcpy(pData, lockdata.pData, g_pTempLightmap->GetWidth() * g_pTempLightmap->GetHeight() * 4);

	g_pTempLightmap->Unlock();

	// d3d fix
	img.SwapChannels(0, 2);

	img.SaveDDS( fileName.GetData() );

	//g_pShaderAPI->SaveRenderTarget( g_pTempLightmap, fileName.GetData() );
}

void RenderLightmap(int nLightmap, bool bDirMap)
{
	int w,h;
	g_pShaderAPI->GetViewportDimensions(w,h);

	viewrenderer->SetViewportSize(g_pLevel->m_nLightmapSize*4, g_pLevel->m_nLightmapSize*4);
	viewrenderer->SetLightmapTextureIndex(nLightmap);

	// Clear the rendertarget

	if(bDirMap)
		viewrenderer->SetupRenderTargets(1, &g_pDirmapTextures[nLightmap], NULL);
	else
		viewrenderer->SetupRenderTargets(1, &g_pLightmapTextures[nLightmap], NULL);

	g_pShaderAPI->Clear(true,true,false,ColorRGBA(0,0,0,0));

	// generate direct lighting
	// accumulate to rendertarget
	for(int i = 0; i < g_lights.numElem(); i++)
	{
		// do it
		materials->BeginFrame();

		viewrenderer->SetViewportSize(g_pLevel->m_nLightmapSize*4, g_pLevel->m_nLightmapSize*4);

		if(bDirMap)
			viewrenderer->SetupRenderTargets(1, &g_pDirmapTextures[nLightmap], NULL);
		else
			viewrenderer->SetupRenderTargets(1, &g_pLightmapTextures[nLightmap], NULL);

		float fUpdPacPercent = ((float)i/(float)g_lights.numElem()) / (float)g_pLevel->m_numLightmaps;
		UpdatePacifier(((float)nLightmap / (float)g_pLevel->m_numLightmaps) + fUpdPacPercent);

		// draw this light
		RenderLight( g_lights[i], nLightmap, bDirMap );

		g_pShaderAPI->ChangeRenderTargetToBackBuffer();
	
		viewrenderer->SetViewportSize(w, h);
		g_pShaderAPI->Reset();
		Vertex2D_t verts[] = {MAKETEXQUAD(0,0,1,1,0)};
			
		materials->Setup2D(1,1);

		if(bDirMap)
			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,verts,elementsOf(verts),g_pDirmapTextures[nLightmap]);
		else
			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,verts,elementsOf(verts),g_pLightmapTextures[nLightmap]);
		

		materials->EndFrame(NULL);
	}

	// reset
	viewrenderer->SetupRenderTargetToBackbuffer();

	/*
	g_pShaderAPI->ChangeRenderTargetToBackBuffer();
	
	viewrenderer->SetViewportSize(w, h);
	g_pShaderAPI->Reset();
	Vertex2D_t verts[] = {MAKETEXQUAD(0,0,1,1,0)};
			
	materials->Setup2D(1,1);
	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,verts,elementsOf(verts),g_pLightmapTextures[lm_id]);

	materials->EndFrame();
	*/
}

void GenerateLightmapTextures()
{
	g_pModelCache->PrecacheModel("models/error.egf");

	materials->SetDeviceBackbufferSize(g_pLevel->m_nLightmapSize, g_pLevel->m_nLightmapSize);

	// init light materials
	InitLightingShaders();

	// for first, we need light source list from entities
	LoadEntityList();

	Msg("Lightmap count: %d\n", g_pLevel->m_numLightmaps);
	Msg("Lightmap size: %d\n", g_pLevel->m_nLightmapSize);
	Msg("Lightmap lights: %d\n", g_lights.numElem());

	// create lightmap textures
	for(int lm_id = 0; lm_id < g_pLevel->m_numLightmaps; lm_id++)
	{
		ITexture* pLightmap = g_pShaderAPI->CreateRenderTarget(g_pLevel->m_nLightmapSize*4,g_pLevel->m_nLightmapSize*4,FORMAT_RGBA16F,TEXFILTER_NEAREST);
		pLightmap->Ref_Grab();
		g_pLightmapTextures.append(pLightmap);

		g_pDirmapTextures.append(pLightmap);

		/*
		ITexture* pDirMapTexture = g_pShaderAPI->CreateRenderTarget(g_pLevel->m_nLightmapSize*4,g_pLevel->m_nLightmapSize*4,FORMAT_RGBA16F,TEXFILTER_NEAREST);
		pDirMapTexture->Ref_Grab();
		g_pDirmapTextures.append(pDirMapTexture);
		*/
	}

	int w,h;
	g_pShaderAPI->GetViewportDimensions(w,h);

	g_pShaderAPI->Clear(true,true,false,ColorRGBA(1,0,0,1));

	StartPacifier("HWDirectLighting ");

	for(int lm_id = 0; lm_id < g_pLevel->m_numLightmaps; lm_id++)
	{
		RenderLightmap(lm_id, false);

		viewrenderer->SetViewportSize(g_pLevel->m_nLightmapSize, g_pLevel->m_nLightmapSize);
		
		materials->BeginFrame();
		FixEdgesAndSave(lm_id, false);
		materials->EndFrame(NULL);
		
	}

	EndPacifier();
	/*
	StartPacifier("HWDirectionMap ");

	for(int lm_id = 0; lm_id < g_pLevel->m_numLightmaps; lm_id++)
	{
		RenderLightmap(lm_id, true);

		viewrenderer->SetViewportSize(g_pLevel->m_nLightmapSize, g_pLevel->m_nLightmapSize);

		materials->BeginFrame();
		FixEdgesAndSave(lm_id, true);
		materials->EndFrame();
	}

	EndPacifier();
	*/

	// if -hwradiosity specified
	// divide level on boxes
	// and compute the GI lights position

	viewrenderer->ShutdownResources();

	g_pShaderAPI->Reset();

	// save world with new ents
	SaveWorld();
}