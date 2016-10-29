//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers level data and renderer
//				You might consider it as some ugly code :(
//////////////////////////////////////////////////////////////////////////////////

#include "world.h"

#include "heightfield.h"

#include "levfile.h"
#include "level.h"
#include "physics.h"
#include "VirtualStream.h"

#include "utils/GeomTools.h"

#include "../shared_engine/physics/BulletConvert.h"

#include "eqGlobalMutex.h"

using namespace EqBulletUtils;
using namespace Threading;

ConVar r_enableLevelInstancing("r_enableLevelInstancing", "1", "Enable level models instancing if available", CV_ARCHIVE);
ConVar r_drawStaticLights("r_drawStaticLights", "1", "Draw static lights", CV_CHEAT);
ConVar w_freeze("w_freeze", "0", "Freeze region spooler", CV_CHEAT);

//-----------------------------------------------------------------------------------------

const int LEVEL_HEIGHTFIELD_SIZE		= 64;
const int LEVEL_HEIGHTFIELD_STEP_SIZE	= (HFIELD_POINT_SIZE*LEVEL_HEIGHTFIELD_SIZE);

CGameLevel::CGameLevel() :
	m_regions(NULL),
	m_wide(0),
	m_tall(0),
	m_cellsSize(64),
	m_regionOffsets(NULL),
	m_roadOffsets(NULL),
	m_occluderOffsets(NULL),
	m_numRegions(0),
	m_regionDataLumpOffset(0),
	m_roadDataLumpOffset(0),
	m_occluderDataLumpOffset(0),
	m_levelName("Unnamed"),
	m_instanceBuffer(NULL),
	m_mutex(GetGlobalMutex(MUTEXPURPOSE_LEVEL_LOADER))
{

}

void CGameLevel::Cleanup()
{
	if(!m_regions)
		return;

	Msg("Unloading level...\n");

	DevMsg(DEVMSG_CORE, "Stopping loader thread...\n");
	StopThread();

	// remove regions first
	int num = m_wide*m_tall;

	DevMsg(DEVMSG_CORE, "Unloading regions...\n");
	for(int i = 0; i < num; i++)
	{
		m_regions[i].Cleanup();
	}

	delete [] m_regions;
	m_regions = NULL;

	if(m_regionOffsets)
		delete [] m_regionOffsets;
	m_regionOffsets = NULL;

	if(m_roadOffsets)
		delete [] m_roadOffsets;
	m_roadOffsets = NULL;

	if(m_occluderOffsets)
		delete [] m_occluderOffsets;
	m_occluderOffsets = NULL;

	DevMsg(DEVMSG_CORE, "Unloading object defs...\n");
	for(int i = 0; i < m_objectDefs.numElem(); i++)
		delete m_objectDefs[i];

	m_objectDefs.clear();

	if(m_instanceBuffer)
		g_pShaderAPI->DestroyVertexBuffer(m_instanceBuffer);
	m_instanceBuffer = NULL;

	m_wide = 0;
	m_tall = 0;
	m_occluderDataLumpOffset = 0;
	m_numRegions = 0;
	m_regionDataLumpOffset = 0;
	m_roadDataLumpOffset = 0;
}

bool CGameLevel::Load(const char* levelname, kvkeybase_t* kvDefs)
{
	IFile* pFile = g_fileSystem->Open(varargs("levels/%s.lev", levelname), "rb", SP_MOD);

	if(!pFile)
	{
		MsgError("can't find level '%s'\n", levelname);
		return false;
	}

	Cleanup();

	levHdr_t hdr;
	pFile->Read(&hdr, sizeof(levHdr_t), 1);

	if(hdr.ident != LEVEL_IDENT)
	{
		MsgError("**ERROR** invalid level '%s' \n", levelname);
		g_fileSystem->Close(pFile);
		return false;
	}

	if(hdr.version != LEVEL_VERSION)
	{
		MsgError("**ERROR** '%s' - unsupported lev file version\n", levelname);
		g_fileSystem->Close(pFile);
		return false;
	}

	bool isOK = true;

	float startLoadTime = Platform_GetCurrentTime();

	levLump_t lump;

	// okay, here we go
	do
	{
		float startLumpTime = Platform_GetCurrentTime();

		if( pFile->Read(&lump, 1, sizeof(levLump_t)) == 0 )
		{
			isOK = false;
			break;
		}

		if(lump.type == LEVLUMP_ENDMARKER)
		{
			isOK = true;
			break;
		}
		else if(lump.type == LEVLUMP_REGIONMAPINFO)
		{
			DevMsg(DEVMSG_GAME,"LEVLUMP_REGIONINFO size = %d\n", lump.size);

			ReadRegionInfo(pFile);

			float loadTime = Platform_GetCurrentTime()-startLumpTime;
			DevMsg(DEVMSG_GAME, " took %g seconds\n", loadTime);
		}
		else if(lump.type == LEVLUMP_REGIONS)
		{
			DevMsg(DEVMSG_GAME, "LEVLUMP_REGIONS size = %d\n", lump.size);

			m_regionDataLumpOffset = pFile->Tell();

#ifdef EDITOR
			long lstart = pFile->Tell();

			// read heightfields
			for(int x = 0; x < m_wide; x++)
			{
				for(int y = 0; y < m_tall; y++)
				{
					int idx = y*m_wide+x;

					LoadRegionAt(idx, pFile);
				}
			}

			pFile->Seek(lstart + lump.size, VS_SEEK_SET);
			break;
#else
			// get out from this lump
			pFile->Seek(lump.size, VS_SEEK_CUR);
#endif
		}
		else if(lump.type == LEVLUMP_ROADS)
		{
			DevMsg(DEVMSG_GAME, "LEVLUMP_ROADS size = %d\n", lump.size);

			int lump_pos = pFile->Tell();

			// read road offsets
			m_roadOffsets = new int[m_wide*m_tall];
			pFile->Read(m_roadOffsets, m_wide*m_tall, sizeof(int));

			m_roadDataLumpOffset = pFile->Tell();

			pFile->Seek(lump_pos+lump.size, VS_SEEK_SET);
		}
		else if(lump.type == LEVLUMP_OCCLUDERS)
		{
			DevMsg(DEVMSG_GAME, "LEVLUMP_OCCLUDERS size = %d\n", lump.size);

			int lump_pos = pFile->Tell();

			// read road offsets
			m_occluderOffsets = new int[m_wide*m_tall];
			pFile->Read(m_occluderOffsets, m_wide*m_tall, sizeof(int));

			m_occluderDataLumpOffset = pFile->Tell();

			pFile->Seek(lump_pos+lump.size, VS_SEEK_SET);
		}
		else if(lump.type == LEVLUMP_OBJECTDEFS)
		{
			DevMsg(DEVMSG_GAME, "LEVLUMP_OBJECTDEFS size = %d\n", lump.size);

			ReadObjectDefsLump(pFile, kvDefs);
		}
		else if(lump.type == LEVLUMP_HEIGHTFIELDS)
		{
			DevMsg(DEVMSG_GAME, "LEVLUMP_HEIGHTFIELDS size = %d\n", lump.size);

			int lump_pos = pFile->Tell();

			ReadHeightfieldsLump(pFile);

			float loadTime = Platform_GetCurrentTime()-startLumpTime;
			DevMsg(DEVMSG_GAME, " took %g seconds\n", loadTime);

			pFile->Seek(lump_pos+lump.size, VS_SEEK_SET);
		}
		else if(lump.type == LEVLUMP_ZONES)
		{
			DevMsg(DEVMSG_GAME, "LEVLUMP_ZONES size = %d\n", lump.size);

			int lump_pos = pFile->Tell();

			// TODO: load LEVLUMP_ZONES

			pFile->Seek(lump_pos+lump.size, VS_SEEK_SET);
		}

		// TODO: other lumps

	}while(true);

#ifndef EDITOR
	StartWorkerThread( "LevelLoaderThread" );
	
#else
	// regenerate nav grid
	Nav_ClearCellStates( NAV_CLEAR_WORKSTATES );
#endif // EDITOR

	g_fileSystem->Close(pFile);

	float loadTime = Platform_GetCurrentTime()-startLoadTime;
	DevMsg(DEVMSG_GAME, "*** Level file read for %g seconds\n", loadTime);

	m_levelName = levelname;

	Msg("LEVEL %s\n", isOK ? "OK" : "LOAD FAILED");

	return isOK;
}

void CGameLevel::Init(int wide, int tall, int cells, bool clean)
{
	m_wide = wide;
	m_tall = tall;
	m_cellsSize = cells;

#ifdef EDITOR
	m_regions = new CEditorLevelRegion[m_wide*m_tall];
#else
	m_regions = new CLevelRegion[m_wide*m_tall];
#endif

	if(clean)
	{
		m_regionOffsets = new int[m_wide*m_tall];
		m_roadOffsets = new int[m_wide*m_tall];
		m_occluderOffsets = new int[m_wide*m_tall];
		m_regionDataLumpOffset = 0;
		m_roadDataLumpOffset = 0;
		m_occluderDataLumpOffset = 0;
		m_numRegions = m_wide*m_tall;
	}

	DevMsg(DEVMSG_CORE,"Creating map %d x %d regions (cell count = %d)\n", m_wide, m_tall, m_cellsSize);

	int nStepSize = HFIELD_POINT_SIZE*m_cellsSize;
	Vector3D center = Vector3D(m_wide*nStepSize, 0, m_tall*nStepSize)*0.5f - Vector3D(HFIELD_POINT_SIZE, 0, HFIELD_POINT_SIZE)*0.5f;

	// setup heightfield positions and we've done here
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			if(clean)
			{
				m_regionOffsets[idx] = -1;
				m_roadOffsets[idx] = -1;
				m_regions[idx].m_isLoaded = clean;
			}

			m_regions[idx].m_regionIndex = idx;
			m_regions[idx].m_level = this;
			m_regions[idx].Init();


			for(int i = 0; i < m_regions[idx].GetNumHFields(); i++)
			{
				if(!m_regions[idx].m_heightfield[i])
					continue;

				m_regions[idx].m_heightfield[i]->m_regionPos = IVector2D(x,y);
				m_regions[idx].m_heightfield[i]->m_position = Vector3D(x*nStepSize, 0, y*nStepSize) - center;

#ifdef EDITOR
				// init other things like road data
				m_regions[idx].m_heightfield[i]->Init(m_cellsSize, IVector2D(x, y));
			}

			m_regions[idx].InitRoads();
#else
			}
#endif // EDITOR

		}
	}

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	if(!m_instanceBuffer && caps.isInstancingSupported && r_enableLevelInstancing.GetBool())
	{
		m_instanceBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_MODEL_INSTANCES, sizeof(regObjectInstance_t));
		m_instanceBuffer->SetFlags( VERTBUFFER_FLAG_INSTANCEDATA );
	}
}

void CGameLevel::ReadRegionInfo(IVirtualStream* stream)
{
	levRegionMapInfo_t hdr;
	stream->Read(&hdr, 1, sizeof(levRegionMapInfo_t));

	m_numRegions = hdr.numRegions;

	DevMsg(DEVMSG_CORE,"[FILE] map %d x %d regions (cell count = %d) regs = %d\n", hdr.numRegionsWide, hdr.numRegionsTall, hdr.cellsSize, m_numRegions);

	int gridArraySize = hdr.numRegionsWide* hdr.numRegionsTall;

	// read region offsets
	m_regionOffsets = new int[gridArraySize];
	stream->Read(m_regionOffsets, gridArraySize, sizeof(int));

	// initialize
	Init(hdr.numRegionsWide, hdr.numRegionsTall, hdr.cellsSize, false);
}

void CGameLevel::LoadRegionAt(int regionIndex, IVirtualStream* stream)
{
	if(m_regionOffsets[regionIndex] == -1)
		return;

	CLevelRegion& reg = m_regions[regionIndex];

	int loffset = stream->Tell();

#ifndef EDITOR
	// Reload materials
	for(int i = 0; i < reg.GetNumHFields(); i++)
	{
		CHeightTileField* hfield = reg.GetHField(i);
		if(!hfield)
			continue;

		if(hfield->m_levOffset == 0)
			continue;

		stream->Seek(hfield->m_levOffset, VS_SEEK_SET);
		hfield->ReadOnlyMaterials(stream);
	}
#endif // EDITOR

	// Read region data
	int regOffset = m_regionDataLumpOffset + m_regionOffsets[regionIndex];
	stream->Seek(regOffset, VS_SEEK_SET);

	reg.ReadLoadRegion(stream, m_objectDefs);

	// read roads
	if(m_roadDataLumpOffset > 0 && m_roadOffsets[regionIndex] != -1)
	{
		int roadsOffset = m_roadDataLumpOffset + m_roadOffsets[regionIndex];
		stream->Seek(roadsOffset, VS_SEEK_SET);

		reg.ReadLoadRoads(stream);
	}

	// read occluders
	if(m_occluderDataLumpOffset > 0 && m_occluderOffsets[regionIndex] != -1)
	{
		int occlOffset = m_occluderDataLumpOffset + m_occluderOffsets[regionIndex];
		stream->Seek(occlOffset, VS_SEEK_SET);

		reg.ReadLoadOccluders(stream);
	}

	reg.m_scriptEventCallbackCalled = false;

	// return back
	stream->Seek(loffset, VS_SEEK_SET);
}

void CGameLevel::PreloadRegion(int x, int y)
{
	// open level file
	IFile* pFile = g_fileSystem->Open(varargs("levels/%s.lev", m_levelName.c_str()), "rb", SP_MOD);

	if(!pFile)
		return;

	int regIdx = y*m_wide+x;

	LoadRegionAt(regIdx, pFile);

	g_fileSystem->Close(pFile);
}

//-------------------------------------------------------------------------------------------------

void LoadDefLightData( wlightdata_t& out, kvkeybase_t* sec )
{
	for(int i = 0; i < sec->keys.numElem(); i++)
	{
		if(!stricmp(sec->keys[i]->name, "light"))
		{
			wlight_t light;
			light.position = KV_GetVector4D(sec->keys[i]);
			light.color = KV_GetVector4D(sec->keys[i], 4);

			// transform light position
			//Vector3D lightPos = light.position.xyz();
			//lightPos = (objDefMatrix*Vector4D(lightPos, 1.0f)).xyz();

			//light.position = Vector4D(lightPos, light.position.w);

			out.m_lights.append(light);
		}
		else if(!stricmp(sec->keys[i]->name, "glow"))
		{
			wglow_t light;
			light.position = KV_GetVector4D(sec->keys[i]);
			light.color = KV_GetVector4D(sec->keys[i], 4);
			light.type = KV_GetValueInt(sec->keys[i], 8);

			// transform light position
			//Vector3D lightPos = light.position.xyz();
			//lightPos = (objDefMatrix*Vector4D(lightPos, 1.0f)).xyz();

			//light.position = Vector4D(lightPos, light.position.w);

			out.m_glows.append(light);
		}
	}
}

//
// from car.cpp, pls move
//
extern void DrawLightEffect(const Vector3D& position, const ColorRGBA& color, float size, int type = 0);

bool DrawDefLightData( Matrix4x4& objDefMatrix, const wlightdata_t& data, float brightness )
{
	bool lightsAdded = false;

	if(g_pGameWorld->m_envConfig.lightsType & WLIGHTS_LAMPS)
	{
		for(int i = 0; i < data.m_lights.numElem(); i++)
		{
			wlight_t light = data.m_lights[i];
			light.color.w *= brightness * g_pGameWorld->m_envConfig.streetLightIntensity;

///-------------------------------------------
			// transform light position
			Vector3D lightPos = light.position.xyz();
			lightPos = (objDefMatrix*Vector4D(lightPos, 1.0f)).xyz();

			light.position = Vector4D(lightPos, light.position.w);
///-------------------------------------------

			bool added = r_drawStaticLights.GetBool() ? g_pGameWorld->AddLight( light ) : true;

			lightsAdded = lightsAdded || added;
		}

		if(!lightsAdded)
			return false;

		float extraBrightness = 0.0f;
		float extraSizeScale = 1.0f;

		if(g_pGameWorld->m_envConfig.weatherType != WEATHER_TYPE_CLEAR)
		{
			extraBrightness = 0.07f;	// add extra brightness to glows
			extraSizeScale = 1.25f;
		}

		for(int i = 0; i < data.m_glows.numElem(); i++)
		{
			// transform light position
			Vector3D lightPos = data.m_glows[i].position.xyz();
			lightPos = (objDefMatrix*Vector4D(lightPos, 1.0f)).xyz();

			if(g_pGameWorld->m_frustum.IsSphereInside(lightPos, data.m_glows[i].position.w))
			{
				ColorRGBA glowColor = data.m_glows[i].color;

				ColorRGBA extraGlowColor = lerp(glowColor*extraBrightness, Vector4D(extraBrightness), 0.25f);

				DrawLightEffect(lightPos,
								glowColor * glowColor.w*brightness + extraBrightness,
								data.m_glows[i].position.w * extraSizeScale,
								data.m_glows[i].type);
			}
		}
	}

	return true;
}

//-------------------------------------------------------------------------------------------------


void CGameLevel::ReadObjectDefsLump(IVirtualStream* stream, kvkeybase_t* kvDefs)
{
	//long fpos = stream->Tell();

	int numModels = 0;
	int modelNamesSize = 0;
	stream->Read(&numModels, 1, sizeof(int));
	stream->Read(&modelNamesSize, 1, sizeof(int));

	char* modelNamesData = new char[modelNamesSize];

	stream->Read(modelNamesData, 1, modelNamesSize);

	char* modelNamePtr = modelNamesData;

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	// load level models and associate objects from <levelname>_objects.txt
	for(int i = 0; i < numModels; i++)
	{
		CLevObjectDef* def = new CLevObjectDef();

		stream->Read(&def->m_info, 1, sizeof(levObjectDefInfo_t));

		def->m_name = modelNamePtr;

		if(def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			CLevelModel* model = new CLevelModel();
			model->Load( stream );
			model->PreloadTextures();

			model->Ref_Grab();

			bool isGroundModel = (def->m_info.modelflags & LMODEL_FLAG_ISGROUND);
			model->GeneratePhysicsData( isGroundModel );

			def->m_model = model;
			def->m_defModel = NULL;

			// init instancer
			if(caps.isInstancingSupported && r_enableLevelInstancing.GetBool())
				def->m_instData = new levObjInstanceData_t;
		}
		else if(def->m_info.type == LOBJ_TYPE_OBJECT_CFG)
		{
			def->m_model = NULL;

			kvkeybase_t* foundDef = NULL;

			for(int j = 0; j < kvDefs->keys.numElem(); j++)
			{
				if(!kvDefs->keys[j]->IsSection())
					continue;

				if(!stricmp(kvDefs->keys[j]->name, "billboardlist"))
					continue;

				if( KV_GetValueBool(kvDefs->keys[j]->FindKeyBase("IsExist")) )
					continue;

				if(!stricmp(KV_GetValueString(kvDefs->keys[j], 0, "error_no_def"), modelNamePtr))
				{
					foundDef = kvDefs->keys[j];
					foundDef->SetKey("IsExist", "1");
					break;
				}
			}

			if(foundDef)
			{
				def->m_defType = foundDef->name;

				def->m_defKeyvalues = new kvkeybase_t();
				def->m_defKeyvalues->MergeFrom( foundDef, true );
#ifdef EDITOR
				LoadDefLightData(def->m_lightData, foundDef);
#endif // EDITOR

				// try precache model
				int modelIdx = g_pModelCache->PrecacheModel( KV_GetValueString(foundDef->FindKeyBase("model"), 0, "models/error.egf"));
				def->m_defModel = g_pModelCache->GetModel(modelIdx);
			}
			else
			{
				MsgError("Cannot find object def '%s'\n", modelNamePtr);
				def->m_defType = "INVALID";

				// error model
				def->m_defModel = g_pModelCache->GetModel(0);
			}
		}

		m_objectDefs.append(def);

		// valid?
		modelNamePtr += strlen(modelNamePtr)+1;
	}

	//
	// load new objects from <levname>_objects.txt
	//
	for(int i = 0; i < kvDefs->keys.numElem(); i++)
	{
		kvkeybase_t* defSection = kvDefs->keys[i];

		if(!defSection->IsSection())
			continue;

		if(!stricmp(defSection->name, "billboardlist"))
			continue;

		bool isAlreadyAdded = KV_GetValueBool(defSection->FindKeyBase("IsExist"), 0, false);

		if(isAlreadyAdded)
			continue;

		kvkeybase_t* modelName = defSection->FindKeyBase("model");

		// precache model
		int modelIdx = g_pModelCache->PrecacheModel( KV_GetValueString(modelName, 0, "models/error.egf"));

		CLevObjectDef* def = new CLevObjectDef();
		def->m_name = KV_GetValueString(defSection, 0, "unnamed_def");

		MsgInfo("Adding object definition '%s'\n", def->m_name.c_str());

		def->m_defKeyvalues = new kvkeybase_t();
		def->m_defKeyvalues->MergeFrom( defSection, true );

		def->m_info.type = LOBJ_TYPE_OBJECT_CFG;

		def->m_model = NULL;
		def->m_defModel = g_pModelCache->GetModel(modelIdx);

		m_objectDefs.append(def);
	}

	delete [] modelNamesData;
}

void CGameLevel::ReadHeightfieldsLump( IVirtualStream* stream )
{
	int nStepSize = HFIELD_POINT_SIZE*m_cellsSize;
	Vector3D center = Vector3D(m_wide*nStepSize, 0, m_tall*nStepSize)*0.5f - Vector3D(HFIELD_POINT_SIZE, 0, HFIELD_POINT_SIZE)*0.5f;

	for(int i = 0; i < m_numRegions; i++)
	{
		int idx = 0;
		stream->Read(&idx, 1, sizeof(int));

		int numFields = 1;
		stream->Read(&numFields, 1, sizeof(int));

		IVector2D regionPos;
		regionPos.x = idx % m_wide;
		regionPos.y = (idx - regionPos.x) / m_wide;

		for(int j = 0; j < numFields; j++)
		{
			int hfieldIndex = j;
			stream->Read(&hfieldIndex, 1, sizeof(int));

			CHeightTileFieldRenderable* hfield = m_regions[idx].m_heightfield[hfieldIndex];

			if( !hfield )
			{
				hfield = new CHeightTileFieldRenderable();
				m_regions[idx].m_heightfield[hfieldIndex] = hfield;

				hfield->m_fieldIdx = hfieldIndex;

				hfield->m_regionPos = IVector2D(regionPos.x, regionPos.y);
				hfield->m_position = Vector3D(regionPos.x*nStepSize, 0, regionPos.y*nStepSize) - center;
			}

			hfield->Init(m_cellsSize, regionPos);
			hfield->m_levOffset = stream->Tell();

#ifdef EDITOR
			hfield->ReadFromStream(stream);
#else
			hfield->ReadOnlyPoints(stream);
#endif // EDITOR
		}
	}
}

CLevelRegion* CGameLevel::GetRegionAtPosition(const Vector3D& pos) const
{
	IVector2D regXY;

	if( GetPointAt(pos, regXY) )
		return &m_regions[regXY.y*m_wide + regXY.x];
	else
		return NULL;
}

CHeightTileFieldRenderable* CGameLevel::GetHeightFieldAt(const IVector2D& XYPos, int idx) const
{
	if(XYPos.x < 0 || XYPos.x >= m_wide)
		return NULL;

	if(XYPos.y < 0 || XYPos.y >= m_tall)
		return NULL;

	return m_regions[XYPos.y*m_wide + XYPos.x].GetHField(idx);
}

CLevelRegion* CGameLevel::GetRegionAt(const IVector2D& XYPos) const
{
	if(XYPos.x < 0 || XYPos.x >= m_wide)
		return NULL;

	if(XYPos.y < 0 || XYPos.y >= m_tall)
		return NULL;

	CLevelRegion* reg = &m_regions[XYPos.y*m_wide + XYPos.x];

	ASSERT(reg->m_regionIndex == XYPos.y*m_wide + XYPos.x);

	return reg;
}


bool CGameLevel::GetPointAt(const Vector3D& pos, IVector2D& outXYPos) const
{
	int cellSize = HFIELD_POINT_SIZE*m_cellsSize;

	Vector3D center(m_wide*cellSize*-0.5f, 0, m_tall*cellSize*-0.5f);

	Vector3D zeroed_pos = pos - center;

	float p_size = (1.0f / (float)cellSize);

	Vector2D xz_pos = (zeroed_pos.xz()) * p_size;

	outXYPos.x = xz_pos.x;
	outXYPos.y = xz_pos.y;

	if(outXYPos.x < 0 || outXYPos.x >= m_wide)
		return false;

	if(outXYPos.y < 0 || outXYPos.y >= m_tall)
		return false;

	return true;
}

IVector2D CGameLevel::PositionToGlobalTilePoint( const Vector3D& pos ) const
{
	IVector2D outXYPos;
	CLevelRegion* pRegion;

	if(GetRegionAndTile(pos, &pRegion, outXYPos))
	{
		IVector2D globalPoint;

		LocalToGlobalPoint(outXYPos, pRegion, globalPoint);

		return globalPoint;
	}

	return IVector2D(0,0);
}

Vector3D CGameLevel::GlobalTilePointToPosition( const IVector2D& point ) const
{
	IVector2D outXYPos;
	CLevelRegion* pRegion;

	GlobalToLocalPoint(point, outXYPos, &pRegion);

	if(pRegion)
	{
		return pRegion->CellToPosition(outXYPos.x,outXYPos.y);
	}

	return Vector3D(0,0,0);
}

bool CGameLevel::GetRegionAndTile(const Vector3D& pos, CLevelRegion** pReg, IVector2D& outXYPos) const
{
	CLevelRegion* pRegion = GetRegionAtPosition(pos);

	if(pRegion && pRegion->m_heightfield[0]->PointAtPos(pos, outXYPos.x, outXYPos.y))
	{
		(*pReg) = pRegion;
		return true;
	}
	else
		(*pReg) = NULL;

	return false;
}

bool CGameLevel::GetRegionAndTileAt(const IVector2D& point, CLevelRegion** pReg, IVector2D& outLocalXYPos) const
{
	CLevelRegion* pRegion = NULL;

	GlobalToLocalPoint(point, outLocalXYPos, &pRegion);

	if(pRegion) // && pRegion->GetHField(0)->GetTile(outLocalXYPos.x, outLocalXYPos.y))
	{
		if(pReg)
			(*pReg) = pRegion;

		return true;
	}
	else
	{
		if(pReg)
			(*pReg) = pRegion;
	}

	return false;
}

bool CGameLevel::GetTileGlobal(const Vector3D& pos, IVector2D& outGlobalXYPos) const
{
	CLevelRegion* pRegion = NULL;

	IVector2D localPos;

	if(!GetRegionAndTile(pos, &pRegion, localPos))
		return false;

	LocalToGlobalPoint(localPos, pRegion, outGlobalXYPos);

	return true;
}

void CGameLevel::GlobalToLocalPoint( const IVector2D& point, IVector2D& outLocalPoint, CLevelRegion** pRegion ) const
{
	IVector2D regPos;
	regPos.x = floor((float)point.x / (float)(m_cellsSize));
	regPos.y = floor((float)point.y / (float)(m_cellsSize));

	IVector2D globalStart;
	globalStart = regPos * m_cellsSize;

	outLocalPoint = point-globalStart;

	(*pRegion) = GetRegionAt(regPos);
}

void CGameLevel::LocalToGlobalPoint( const IVector2D& point, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const
{
	outGlobalPoint = pRegion->m_heightfield[0]->m_regionPos * m_cellsSize + point;
}

//------------------------------------------------------------------------------------------------------------------------------------


straight_t CGameLevel::GetStraightAtPoint( const IVector2D& point, int numIterations ) const
{
	CLevelRegion* pRegion = NULL;

	straight_t straight;
	IVector2D localPos;

	if( GetRegionAndTileAt(point, &pRegion, localPos ) && pRegion->m_roads )
	{
		int tileIdx = localPos.y * m_cellsSize + localPos.x;

		levroadcell_t& startCell = pRegion->m_roads[tileIdx];

		if(	startCell.type != ROADTYPE_STRAIGHT &&
			startCell.type != ROADTYPE_PARKINGLOT )
			return straight;

		int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
		int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

		straight.start = localPos;
		straight.dirChangeIter = INT_MAX;
		straight.breakIter = 0;

		LocalToGlobalPoint(localPos, pRegion, straight.start);

		int roadDir = startCell.direction;

		int checkRoadDir = roadDir;

		IVector2D lastPos = localPos;

		for(int i = 0; i < numIterations; i++,straight.breakIter++)
		{
			// go that direction
			IVector2D xyPos = lastPos + IVector2D(dx[checkRoadDir],dy[checkRoadDir]);//localPos + IVector2D(dx[roadDir],dy[roadDir])*i;

			lastPos = xyPos;

			CLevelRegion* pNextRegion;

			xyPos = pRegion->GetTileAndNeighbourRegion(xyPos.x, xyPos.y, &pNextRegion);
			int nextTileIdx = xyPos.y * m_cellsSize + xyPos.x;

			if(!pNextRegion)
				continue;

			if(!pNextRegion->m_roads)
				continue;

			levroadcell_t& roadCell = pNextRegion->m_roads[nextTileIdx];

			if( roadCell.type != ROADTYPE_STRAIGHT &&
				roadCell.type != ROADTYPE_PARKINGLOT )
				break;

			if( IsOppositeDirectionTo(roadCell.direction, checkRoadDir) )
				continue;

			if(roadCell.flags & ROAD_FLAG_TRAFFICLIGHT)
				straight.hasTrafficLight = true;

			if(	roadCell.direction != checkRoadDir &&
				i < straight.dirChangeIter)
			{
				straight.dirChangeIter = i;
			}

			checkRoadDir = roadCell.direction;

			// set the original direction
			straight.direction = roadDir;
			straight.end = xyPos;
			LocalToGlobalPoint(xyPos, pNextRegion, straight.end);
		}

		// don't let it be equal
		if(	straight.direction != -1 &&
			straight.start == straight.end)
		{
			// step back
			straight.start -= IVector2D(dx[straight.direction], dy[straight.direction]);
		}
	}

	return straight;
}

straight_t CGameLevel::GetStraightAtPos( const Vector3D& pos, int numIterations ) const
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return GetStraightAtPoint(globPos, numIterations);
}

roadJunction_t CGameLevel::GetJunctionAtPoint( const IVector2D& point, int numIterations ) const
{
	CLevelRegion* pRegion = NULL;

	roadJunction_t junction;
	junction.breakIter = 0;
	junction.startIter = -1;

	IVector2D localPos;

	if( GetRegionAndTileAt(point, &pRegion, localPos ) && pRegion->m_roads )
	{
		int tileIdx = localPos.y * m_cellsSize + localPos.x;

		levroadcell_t& startCell = pRegion->m_roads[tileIdx];

		if(	startCell.type == ROADTYPE_NOROAD )
		{
			return junction;
		}

		int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
		int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

		junction.start = localPos;
		junction.end = localPos;


		// if we already on junction
		if(startCell.type == ROADTYPE_JUNCTION)
		{
			return junction;
		}

		LocalToGlobalPoint(localPos, pRegion, junction.start);

		int roadDir = startCell.direction;

		int checkRoadDir = roadDir;

		IVector2D lastPos = localPos;

		for(int i = 0; i < numIterations; i++, junction.breakIter++)
		{
			// go that direction
			IVector2D xyPos = lastPos + IVector2D(dx[checkRoadDir],dy[checkRoadDir]);//localPos + IVector2D(dx[roadDir],dy[roadDir])*i;

			lastPos = xyPos;

			CLevelRegion* pNextRegion;

			xyPos = pRegion->GetTileAndNeighbourRegion(xyPos.x, xyPos.y, &pNextRegion);
			int nextTileIdx = xyPos.y * m_cellsSize + xyPos.x;

			if(!pNextRegion->m_roads)
				continue;

			levroadcell_t& roadCell = pNextRegion->m_roads[nextTileIdx];

			if( roadCell.type != ROADTYPE_JUNCTION && junction.startIter != -1 )
				break;

			if(	roadCell.type == ROADTYPE_NOROAD )
				break;

			if(roadCell.type == ROADTYPE_JUNCTION && junction.startIter == -1)
			{
				junction.startIter = i;
			}

			// set the original direction
			junction.end = xyPos;
			LocalToGlobalPoint(xyPos, pNextRegion, junction.end);
		}
	}

	return junction;
}

roadJunction_t CGameLevel::GetJunctionAtPos( const Vector3D& pos, int numIterations ) const
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return GetJunctionAtPoint(globPos, numIterations);
}

int	CGameLevel::GetLaneIndexAtPoint( const IVector2D& point, int numIterations)
{
	CLevelRegion* pRegion = NULL;

	IVector2D localPos;

	int nLane = 0;

	if( GetRegionAndTileAt(point, &pRegion, localPos ) && pRegion->m_roads )
	{
		int tileIdx = localPos.y * m_cellsSize + localPos.x;

		levroadcell_t& startCell = pRegion->m_roads[tileIdx];

		if(	startCell.type != ROADTYPE_STRAIGHT &&
			startCell.type != ROADTYPE_PARKINGLOT )
			return -1;

		int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
		int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

		int laneDir = startCell.direction;
		int laneRowDir = GetPerpendicularDir(startCell.direction);

		for(int i = 0; i < numIterations; i++)
		{
			IVector2D xyPos = localPos + IVector2D(dx[laneRowDir],dy[laneRowDir])*i;//localPos + IVector2D(dx[roadDir],dy[roadDir])*i;

			CLevelRegion* pNextRegion;

			xyPos = pRegion->GetTileAndNeighbourRegion(xyPos.x, xyPos.y, &pNextRegion);
			int nextTileIdx = xyPos.y * m_cellsSize + xyPos.x;

			if(!pNextRegion->m_roads)
				continue;

			levroadcell_t& roadCell = pNextRegion->m_roads[nextTileIdx];

			if(	roadCell.type != ROADTYPE_STRAIGHT &&
				roadCell.type != ROADTYPE_PARKINGLOT )
				break;

			if( roadCell.direction != laneDir)
				break;

			nLane++;
		}
	}

	return nLane;
}

int	CGameLevel::GetLaneIndexAtPos( const Vector3D& pos, int numIterations)
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return GetLaneIndexAtPoint(globPos, numIterations);
}

int	CGameLevel::GetRoadWidthInLanesAtPoint( const IVector2D& point, int numIterations )
{
	CLevelRegion* pRegion = NULL;

	IVector2D localPos;

	// trace to the right
	int nLanes = GetLaneIndexAtPoint(point, numIterations);

	if(nLanes == -1)
		return 0;

	// and left
	if( GetRegionAndTileAt(point, &pRegion, localPos ) && pRegion->m_roads )
	{
		int tileIdx = localPos.y * m_cellsSize + localPos.x;

		levroadcell_t& startCell = pRegion->m_roads[tileIdx];

		if(	startCell.type != ROADTYPE_STRAIGHT &&
			startCell.type != ROADTYPE_PARKINGLOT )
			return -1;

		int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
		int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

		int laneDir = startCell.direction;
		int laneRowDir = GetPerpendicularDir(startCell.direction);

		for(int i = 1; i < numIterations; i++)
		{
			IVector2D nextPos = localPos - IVector2D(dx[laneRowDir],dy[laneRowDir])*i;//localPos + IVector2D(dx[roadDir],dy[roadDir])*i;

			CLevelRegion* pNextRegion;

			nextPos = pRegion->GetTileAndNeighbourRegion(nextPos.x, nextPos.y, &pNextRegion);
			int nextTileIdx = nextPos.y * m_cellsSize + nextPos.x;

			if(!pNextRegion->m_roads)
				continue;

			levroadcell_t& roadCell = pNextRegion->m_roads[nextTileIdx];

			if(	roadCell.type != ROADTYPE_STRAIGHT &&
				roadCell.type != ROADTYPE_PARKINGLOT )
				break;

			// only parallels
			if( (roadCell.direction % 2) != (laneDir % 2))
				break;

			nLanes++;
		}
	}

	return nLanes;
}

int	CGameLevel::GetRoadWidthInLanesAtPos( const Vector3D& pos, int numIterations )
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return GetRoadWidthInLanesAtPoint(globPos, numIterations);
}

int	CGameLevel::GetNumLanesAtPoint( const IVector2D& point, int numIterations )
{
	CLevelRegion* pRegion = NULL;

	IVector2D localPos;

	int nLanes = GetLaneIndexAtPoint(point, numIterations);

	if(nLanes <= 0)
		return 0;

	if( GetRegionAndTileAt(point, &pRegion, localPos ) && pRegion->m_roads )
	{
		int tileIdx = localPos.y * m_cellsSize + localPos.x;

		levroadcell_t& startCell = pRegion->m_roads[tileIdx];

		if(	startCell.type != ROADTYPE_STRAIGHT &&
			startCell.type != ROADTYPE_PARKINGLOT )
			return 0;

		IVector2D lastPos = localPos;

		int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
		int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

		int laneDir = startCell.direction;
		int laneRowDir = GetPerpendicularDir(startCell.direction);

		for(int i = 1; i < numIterations; i++)
		{
			IVector2D xyPos = lastPos - IVector2D(dx[laneRowDir],dy[laneRowDir]);//localPos + IVector2D(dx[roadDir],dy[roadDir])*i;

			lastPos = xyPos;

			CLevelRegion* pNextRegion;

			xyPos = pRegion->GetTileAndNeighbourRegion(xyPos.x, xyPos.y, &pNextRegion);
			int nextTileIdx = xyPos.y * m_cellsSize + xyPos.x;

			if(!pNextRegion->m_roads)
				continue;

			levroadcell_t& roadCell = pNextRegion->m_roads[nextTileIdx];

			if(	roadCell.type != ROADTYPE_STRAIGHT &&
				roadCell.type != ROADTYPE_PARKINGLOT )
				break;

			if( roadCell.direction != laneDir)
				break;

			nLanes++;
		}
	}

	return nLanes;
}

int	CGameLevel::GetNumLanesAtPos( const Vector3D& pos, int numIterations )
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return GetNumLanesAtPoint(globPos, numIterations);
}

levroadcell_t* CGameLevel::GetGlobalRoadTile(const Vector3D& pos, CLevelRegion** pRegion) const
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return GetGlobalRoadTileAt(globPos, pRegion);
}

levroadcell_t* CGameLevel::GetGlobalRoadTileAt(const IVector2D& point, CLevelRegion** pRegion) const
{
	IVector2D outXYPos;

	CLevelRegion* reg = NULL;

	if( GetRegionAndTileAt(point, &reg, outXYPos) )
	{
		if(pRegion)
			*pRegion = reg;

		if(!reg->m_roads)
			return NULL;

		int ridx = outXYPos.y*m_cellsSize + outXYPos.x;

		return &reg->m_roads[ridx];
	}

	return NULL;
}

bool CGameLevel::FindBestRoadCellForTrafficLight( IVector2D& out, const Vector3D& origin, int trafficDir, int juncIterations )
{
	IVector2D cellPos = PositionToGlobalTilePoint(origin);
	int laneRowDir = GetPerpendicularDir(trafficDir);

	int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
	int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

	IVector2D forwardDir = IVector2D(dx[trafficDir],dy[trafficDir]);
	IVector2D rightDir = IVector2D(dx[laneRowDir],dy[laneRowDir]);

	levroadcell_t* roadTile = NULL;
	
	/*
	first method
		Find passing straight from left and back one step
	*/

	IVector2D roadTilePos = cellPos - rightDir - forwardDir;

	roadTile = GetGlobalRoadTileAt(roadTilePos);

	if(	roadTile && 
		(roadTile->type == ROADTYPE_STRAIGHT || roadTile->type == ROADTYPE_PARKINGLOT) && 
		roadTile->direction == trafficDir)
	{
		out = roadTilePos;
		return true;
	}


	/*
	second method
		Find passing straight from left, but iterate through junction
		Break on first occurence
	*/

	for(int i = 0; i < juncIterations; i++)
	{
		IVector2D checkTilePos = roadTilePos - forwardDir*i;

		roadTile = GetGlobalRoadTileAt( checkTilePos );

		if(	roadTile && 
			(roadTile->type == ROADTYPE_STRAIGHT || roadTile->type == ROADTYPE_PARKINGLOT) && 
			roadTile->direction == trafficDir)
		{
			out = checkTilePos;
			return true;
		}

	}

	/*
	third method
		Find passing straight using only backwards iteration through junctions
		If we have found straight in wrong direction (not perpendicular), just try searching to right
	*/

	IVector2D checkTilePos = cellPos;

	for(int i = 0; i < juncIterations; i++)
	{
		checkTilePos -= forwardDir;

		roadTile = GetGlobalRoadTileAt( checkTilePos );

		if(	roadTile && (roadTile->type == ROADTYPE_STRAIGHT || roadTile->type == ROADTYPE_PARKINGLOT))
		{
			// it's just beautiful we've found it
			if(roadTile->direction == trafficDir)
			{
				out = checkTilePos;
				return true;
			}

			// but what if we have wrong direction???
			if((roadTile->direction % 2) == (trafficDir % 2))
			{
				// search to the right
				checkTilePos += rightDir;
				continue;
			}

			// has to break here
			break;
		}
	}

	return false;
}

//------------------------------------------------------------------------------------------------------------------------------------

void CGameLevel::QueryNearestRegions( const Vector3D& pos, bool waitLoad )
{
	IVector2D posXY;
	if(GetPointAt(pos,posXY))
	{
		QueryNearestRegions(posXY, waitLoad);
	}
}

void CGameLevel::QueryNearestRegions( const IVector2D& point, bool waitLoad )
{
	CLevelRegion* region = GetRegionAt(point);

	if(!region)
		return;

	int numNeedToLoad = !region->m_isLoaded && (m_regionOffsets[point.y*m_wide+point.x] != -1);

	// query this region
	region->m_queryTimes.Increment();

	int dx[8] = NEIGHBOR_OFFS_XDX(point.x, 1);
	int dy[8] = NEIGHBOR_OFFS_YDY(point.y, 1);

	// query surrounding regions
	for(int i = 0; i < 8; i++)
	{
		CLevelRegion* nregion = GetRegionAt(IVector2D(dx[i], dy[i]));

		if(nregion)
		{
			if(!nregion->m_isLoaded)
				numNeedToLoad += (m_regionOffsets[dy[i]*m_wide+dx[i]] != -1);

			nregion->m_queryTimes.Increment();
		}
	}

	if( numNeedToLoad > 0 )
	{
		g_pShaderAPI->BeginAsyncOperation( GetThreadID() );

		// signal loader
		SignalWork();

		if( waitLoad )
			WaitForThread();
	}
}

void CGameLevel::CollectVisibleOccluders(occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition)
{
	frustumOccluders.frustum = g_pGameWorld->m_frustum;

	// don't render too far?
	IVector2D camPosReg;

	// mark renderable regions
	if( GetPointAt(cameraPosition, camPosReg) )
	{
		CLevelRegion* region = GetRegionAt(camPosReg);

		// query this region
		if(region)
		{
			bool visible = frustumOccluders.frustum.IsBoxInside(region->m_bbox.minPoint, region->m_bbox.maxPoint);

			if(visible)
				region->CollectVisibleOccluders(frustumOccluders, cameraPosition);
		}

		int dx[8] = NEIGHBOR_OFFS_XDX(camPosReg.x, 1);
		int dy[8] = NEIGHBOR_OFFS_YDY(camPosReg.y, 1);

		// surrounding regions frustum
		for(int i = 0; i < 8; i++)
		{
			CLevelRegion* nregion = GetRegionAt(IVector2D(dx[i], dy[i]));

			if(nregion)
			{
				bool visible = frustumOccluders.frustum.IsBoxInside(nregion->m_bbox.minPoint, nregion->m_bbox.maxPoint);

				if(visible)
					nregion->CollectVisibleOccluders(frustumOccluders, cameraPosition);
			}
		}
	}
}

extern ConVar r_regularLightTextureUpdates;

void CGameLevel::Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& occlFrustum, int nRenderFlags)
{
	bool renderTranslucency = (nRenderFlags & RFLAG_TRANSLUCENCY) > 0;

	// don't render too far?
	IVector2D camPosReg;

	// mark renderable regions
	if( GetPointAt(cameraPosition, camPosReg) )
	{
		CLevelRegion* region = GetRegionAt(camPosReg);

		// query this region
		if(region)
		{
			region->m_render = !(renderTranslucency && !region->m_hasTransparentSubsets); 

			if(region->m_render)
			{
				region->Render( cameraPosition, viewProj, occlFrustum, nRenderFlags );
				region->m_render = false;
			}
		}

		int dx[8] = NEIGHBOR_OFFS_XDX(camPosReg.x, 1);
		int dy[8] = NEIGHBOR_OFFS_YDY(camPosReg.y, 1);

		// surrounding regions frustum
		for(int i = 0; i < 8; i++)
		{
			CLevelRegion* nregion = GetRegionAt(IVector2D(dx[i], dy[i]));

			if(nregion && !(renderTranslucency && !nregion->m_hasTransparentSubsets))
			{
				nregion->m_render = occlFrustum.IsBoxVisible( nregion->m_bbox );

				if(nregion->m_render)
				{
					nregion->Render( cameraPosition, viewProj, occlFrustum, nRenderFlags );
					nregion->m_render = false;
				}
			}
		}
	}

	// FIXME: DISABLED
	if(renderTranslucency)
		return;

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	if(caps.isInstancingSupported && r_enableLevelInstancing.GetBool())
	{
		materials->SetInstancingEnabled( true ); // matsystem switch to use instancing in shaders
		materials->SetMatrix(MATRIXMODE_WORLD, identity4());

		// force disable vertex buffer
		g_pShaderAPI->SetVertexBuffer( NULL, 2 );

		for(int i = 0; i < m_objectDefs.numElem(); i++)
		{
			CLevObjectDef* def = m_objectDefs[i];

			if(def->m_info.type != LOBJ_TYPE_INTERNAL_STATIC)
				continue;

			if(!def->m_model || !def->m_instData)
				continue;

			if(def->m_instData->numInstances == 0)
				continue;

			CLevelModel* model = def->m_model;
			
			if(renderTranslucency && !model->m_hasTransparentSubsets)
				continue;

			// set vertex buffer
			g_pShaderAPI->SetVertexBuffer( m_instanceBuffer, 2 );

			// before lock we have to unbind our buffer
			g_pShaderAPI->ChangeVertexBuffer( NULL, 2 );

			int numInstances = def->m_instData->numInstances;

			m_instanceBuffer->Update(def->m_instData->instances, numInstances, 0, true);

			model->Render(nRenderFlags, model->m_bbox);

			// reset instance count
			def->m_instData->numInstances = 0;

			// disable this vertex buffer or our cars will be drawn many times
			g_pShaderAPI->SetVertexBuffer( NULL, 2 );
		}

		materials->SetInstancingEnabled( false );

		// force disable vertex buffer
		g_pShaderAPI->SetVertexBuffer( NULL, 2 );
		g_pShaderAPI->ChangeVertexBuffer(NULL, 2);
	}
}

int	CGameLevel::Run()
{
	UpdateRegionLoading();

	g_pShaderAPI->EndAsyncOperation();

	return 0;
}

int CGameLevel::UpdateRegionLoading()
{
#ifndef EDITOR

	int numLoadedRegions = 0;

	float startLoadTime = Platform_GetCurrentTime();

	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			// try preloading region
			if( !w_freeze.GetBool() &&
				!m_regions[idx].m_isLoaded &&
				(m_regions[idx].m_queryTimes.GetValue() > 0) &&
				m_regionOffsets[idx] != -1 )
			{
				PreloadRegion(x,y);

				numLoadedRegions++;
				DevMsg(DEVMSG_CORE, "Region %d loaded\n", idx);
			}
		}
	}

	float loadTime = Platform_GetCurrentTime()-startLoadTime;
	if (numLoadedRegions)
		DevMsg(DEVMSG_CORE, "*** %d regions loaded for %g seconds\n", numLoadedRegions, loadTime);

	// wait for matsystem
	if(numLoadedRegions)
		materials->Wait();

	return numLoadedRegions;
#else
	return 0;
#endif // EDITOR
}

int CGameLevel::UpdateRegions( RegionLoadUnloadCallbackFunc func )
{
	int numFreedRegions = 0;

	// unloading only available after loading
	if(!IsWorkDone())
		return 0;

	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

#ifndef EDITOR
			if(!w_freeze.GetBool() &&
				m_regions[idx].m_isLoaded &&
				(m_regions[idx].m_queryTimes.GetValue() <= 0) &&
				m_regionOffsets[idx] != -1 )
			{
				// unload region
				m_regions[idx].Cleanup();
				m_regions[idx].m_scriptEventCallbackCalled = false;

				numFreedRegions++;
				DevMsg(DEVMSG_CORE, "Region %d freed\n", idx);
			}
			else
			{

#pragma todo("SPAWN objects from UpdateRegions, not from loader thread")
			}
#endif // EDITOR

			//m_regions[idx].m_queryTimes.Decrement();

			//if(m_regions[idx].m_queryTimes.GetValue() < 0)
			m_regions[idx].m_queryTimes.SetValue(0);

			if( !m_regions[idx].m_scriptEventCallbackCalled )
			{
				if(func)
					(func)(&m_regions[idx], idx);

				m_regions[idx].m_scriptEventCallbackCalled = true;
			}
		}
	}

	return numFreedRegions;
}

void CGameLevel::RespawnAllObjects()
{
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;
			m_regions[idx].RespawnObjects();
		}
	}
}

#define OBSTACLE_STATIC_MAX_HEIGHT	(8.0f)
#define OBSTACLE_PROP_MAX_HEIGHT	(4.0f)

void CGameLevel::Nav_AddObstacle(CLevelRegion* reg, regionObject_t* ref)
{
	/*
		transformedModel = transformModelRef(ref)
		foreach triangle in transformedModel
			tileRange = getTileRange(triangle)
			foreach tile in tileRange
				volume = getTileVolume
				if( volume.checkIntersectionWithTriangle(triangle) )
					tile.bit = 1

	*/

	//int navCellGridSize = m_cellsSize*AI_NAVIGATION_GRID_SCALE;

	CLevObjectDef* def = ref->def;

	if(def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
		// static model processing

		if((def->m_info.modelflags & LMODEL_FLAG_ISGROUND) ||
			(def->m_info.modelflags & LMODEL_FLAG_NOCOLLIDE))
			return;

		CLevelModel* model = def->m_model;

		Matrix4x4 refTransform = ref->transform;

		for (int i = 0; i < model->m_numIndices; i += 3)
		{
			if (i + 1 > model->m_numIndices || i + 2 > model->m_numIndices)
				break;

			Vector3D p0, p1, p2;
			Vector3D v0, v1, v2;

			if (model->m_indices[i] >= model->m_numVerts ||
				model->m_indices[i + 1] >= model->m_numVerts ||
				model->m_indices[i + 2] >= model->m_numVerts)
				continue;

			// take physics geometry vertices
			if(model->m_physVerts != NULL)
			{
				p0 = model->m_physVerts[model->m_indices[i]];
				p1 = model->m_physVerts[model->m_indices[i + 1]];
				p2 = model->m_physVerts[model->m_indices[i + 2]];
			}
			else
			{
				// mostly used in editor...
				p0 = model->m_verts[model->m_indices[i]].position;
				p1 = model->m_verts[model->m_indices[i + 1]].position;
				p2 = model->m_verts[model->m_indices[i + 2]].position;
			}

			v0 = (refTransform*Vector4D(p0, 1.0f)).xyz();
			v1 = (refTransform*Vector4D(p1, 1.0f)).xyz();
			v2 = (refTransform*Vector4D(p2, 1.0f)).xyz();

			Vector3D normal;
			ComputeTriangleNormal(v0, v1, v2, normal);

			//if(fabs(dot(normal,vec3_up)) > 0.8f)
			//	continue;

			if(p0.y > OBSTACLE_STATIC_MAX_HEIGHT || p1.y > OBSTACLE_STATIC_MAX_HEIGHT || p2.y > OBSTACLE_STATIC_MAX_HEIGHT)
				continue;

			//debugoverlay->Polygon3D(v0, v1, v2, ColorRGBA(1, 0, 1, 0.25f), 100.0f);

			BoundingBox vertbox;
			vertbox.AddVertex(v0);
			vertbox.AddVertex(v1);
			vertbox.AddVertex(v2);

			// get a cell range
			IVector2D min, max;
			Nav_GetCellRangeFromAABB(vertbox.minPoint, vertbox.maxPoint, min, max);

			// in this range do...
			for (int y = min.y; y < max.y; y++)
			{
				for (int x = min.x; x < max.x; x++)
				{
					//Vector3D pointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(IVector2D(x, y));
					//debugoverlay->Box3D(pointPos - 0.5f, pointPos + 0.5f, ColorRGBA(1, 0, 1, 0.1f));

					ubyte& tile = Nav_GetTileAtGlobalPoint(IVector2D(x,y));
					tile = 0;
				}
			}
		}

	}
	else if(def->m_defModel != NULL)
	{
		if(	def->m_defType == "debris" ||
			def->m_defType == "sheets" ||
			def->m_defType == "misc")
			return;

		// studio model
		physmodeldata_t* physData = &def->m_defModel->GetHWData()->m_physmodel;

		for(int i = 0; i < physData->numIndices; i+=3)
		{
			Vector3D v0, v1, v2;
			Vector3D p0,p1,p2;

			if (physData->indices[i] >= physData->numVertices ||
				physData->indices[i + 1] >= physData->numVertices ||
				physData->indices[i + 2] >= physData->numVertices)
				continue;

			p0 = physData->vertices[physData->indices[i]];
			p1 = physData->vertices[physData->indices[i + 1]];
			p2 = physData->vertices[physData->indices[i + 2]];

			v0 = (ref->transform*Vector4D(p0, 1.0f)).xyz();
			v1 = (ref->transform*Vector4D(p1, 1.0f)).xyz();
			v2 = (ref->transform*Vector4D(p2, 1.0f)).xyz();

			Vector3D normal;
			ComputeTriangleNormal(v0, v1, v2, normal);

			//if (fabs(dot(normal, vec3_up)) > 0.8f)
			//	continue;

			if (p0.y > OBSTACLE_PROP_MAX_HEIGHT || p1.y > OBSTACLE_PROP_MAX_HEIGHT || p2.y > OBSTACLE_PROP_MAX_HEIGHT)
				continue;

			//debugoverlay->Polygon3D(v0,v1,v2, ColorRGBA(1,1,0,0.25f), 100.0f);

			BoundingBox vertbox;
			vertbox.AddVertex(v0);
			vertbox.AddVertex(v1);
			vertbox.AddVertex(v2);

			// get a cell range
			IVector2D min, max;
			Nav_GetCellRangeFromAABB(vertbox.minPoint, vertbox.maxPoint, min, max);

			// in this range do...
			for (int y = min.y; y < max.y; y++)
			{
				for (int x = min.x; x < max.x; x++)
				{
					//Vector3D pointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(IVector2D(x, y));
					//debugoverlay->Box3D(pointPos - 0.5f, pointPos + 0.5f, ColorRGBA(1, 0, 1, 0.1f));

					ubyte& tile = Nav_GetTileAtGlobalPoint(IVector2D(x, y));
					tile = 0;
				}
			}
		}
	}
}

void CGameLevel::Nav_GetCellRangeFromAABB(const Vector3D& mins, const Vector3D& maxs, IVector2D& xy1, IVector2D& xy2) const
{
	xy1 = Nav_PositionToGlobalNavPoint(mins)-2;
	xy2 = Nav_PositionToGlobalNavPoint(maxs)+2;
}

void CGameLevel::Nav_GlobalToLocalPoint(const IVector2D& point, IVector2D& outLocalPoint, CLevelRegion** pRegion) const
{
	int navGridSize = m_cellsSize*AI_NAVIGATION_GRID_SCALE;

	IVector2D regPos = point / navGridSize;

	outLocalPoint = point - (regPos * navGridSize);

	(*pRegion) = GetRegionAt(regPos);
}

void CGameLevel::Nav_LocalToGlobalPoint(const IVector2D& point, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const
{
	int navGridSize = m_cellsSize*AI_NAVIGATION_GRID_SCALE;
	outGlobalPoint = pRegion->m_heightfield[0]->m_regionPos * navGridSize + point;
}

#define NAV_POINT_SIZE (HFIELD_POINT_SIZE/AI_NAVIGATION_GRID_SCALE)

//
// conversions
//
IVector2D CGameLevel::Nav_PositionToGlobalNavPoint(const Vector3D& pos) const
{
	IVector2D gridWorldSize(m_wide, m_tall);
	gridWorldSize *= m_cellsSize*AI_NAVIGATION_GRID_SCALE;

	float p_size = (1.0f / NAV_POINT_SIZE);

	Vector2D xz_pos = pos.xz() * p_size + gridWorldSize/2;

	IVector2D out;

	out.x = xz_pos.x;// + 0.5f;
	out.y = xz_pos.y;// + 0.5f;

	return out;
}

Vector3D CGameLevel::Nav_GlobalPointToPosition(const IVector2D& point) const
{
	IVector2D outXYPos;
	CLevelRegion* pRegion;

	Nav_GlobalToLocalPoint(point, outXYPos, &pRegion);

	if (pRegion)
	{
		CHeightTileField& defField = *pRegion->m_heightfield[0];

		hfieldtile_t* tile = defField.GetTile(outXYPos.x / AI_NAVIGATION_GRID_SCALE, outXYPos.y / AI_NAVIGATION_GRID_SCALE);

		Vector3D tile_position;

		if (tile)
			tile_position = defField.m_position + Vector3D(outXYPos.x*NAV_POINT_SIZE, tile->height*HFIELD_HEIGHT_STEP, outXYPos.y*NAV_POINT_SIZE);
		else
			tile_position = defField.m_position + Vector3D(outXYPos.x*NAV_POINT_SIZE, 0, outXYPos.y*NAV_POINT_SIZE);

		tile_position.x -= NAV_POINT_SIZE*0.5f;
		tile_position.z -= NAV_POINT_SIZE*0.5f;

		return tile_position;
	}

	return Vector3D(0, 0, 0);
}

float estimateDist(const IVector2D& src, const IVector2D& dest)
{
	float d;

	Vector2D dir = Vector2D(dest) - Vector2D(src);

	// Euclidian Distance
	d = length(dir);

	// Manhattan distance
	//d = fabs(dir.x)+fabs(dir.y);

	// Chebyshev distance
	//d = max(abs(dir.x), abs(dir.y));

	return d;
}

navcell_t& CGameLevel::Nav_GetCellStateAtGlobalPoint(const IVector2D& point)
{
	int navSize = m_cellsSize * AI_NAVIGATION_GRID_SCALE;

	IVector2D localPoint;
	CLevelRegion* reg;

	Nav_GlobalToLocalPoint(point, localPoint, &reg);

	CScopedMutex m(m_mutex);

	if (reg && reg->m_isLoaded)
	{
		return reg->m_navGrid.cellStates[localPoint.y*navSize + localPoint.x];
	}

	static navcell_t emptyCell;
	emptyCell.cost = 15;
	emptyCell.flag = 1;
	emptyCell.pdir = 7;

	return emptyCell;
}

ubyte& CGameLevel::Nav_GetTileAtGlobalPoint(const IVector2D& point, bool obstacles)
{
	int navSize = m_cellsSize * AI_NAVIGATION_GRID_SCALE;

	IVector2D localPoint;
	CLevelRegion* reg;

	Nav_GlobalToLocalPoint(point, localPoint, &reg);

	CScopedMutex m(m_mutex);

	if (reg && reg->m_navGrid.staticObst)
	{
		if(obstacles)
			return reg->m_navGrid.dynamicObst[localPoint.y*navSize + localPoint.x];
		else
			return reg->m_navGrid.staticObst[localPoint.y*navSize + localPoint.x];
	}

	static ubyte emptyTile = 255;

	return emptyTile;
}

void CGameLevel::Nav_ClearCellStates(ECellClearStateMode mode)
{
	int navSize = (m_cellsSize*AI_NAVIGATION_GRID_SCALE);

	for (int x = 0; x < m_wide; x++)
	{
		for (int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide + x;

			m_mutex.Lock();

			CLevelRegion& reg = m_regions[idx];

			if(reg.m_isLoaded) // zero them
			{
				switch(mode)
				{
					case NAV_CLEAR_WORKSTATES:
						memset(reg.m_navGrid.cellStates, 0, navSize*navSize);
						break;
					case NAV_CLEAR_DYNAMIC_OBSTACLES:
						memset(reg.m_navGrid.dynamicObst, 0x4, navSize*navSize);
						break;
				}

				if( reg.m_navGrid.dirty )
				{
					for (int i = 0; i < reg.m_objects.numElem(); i++)
						Nav_AddObstacle(&reg, reg.m_objects[i]);

					reg.m_navGrid.dirty = false;
				}
			}

			m_mutex.Unlock();
		}
	}
}

bool CGameLevel::Nav_FindPath(const Vector3D& start, const Vector3D& end, pathFindResult_t& result, int iterationLimit, bool cellPriority)
{
	IVector2D startPoint = g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(start);
	IVector2D endPoint = g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(end);

	return Nav_FindPath2D(startPoint, endPoint, result, iterationLimit, cellPriority);
}

bool CGameLevel::Nav_FindPath2D(const IVector2D& start, const IVector2D& end, pathFindResult_t& result, int iterationLimit, bool cellPriority)
{
	if(start == end)
		return false;

	result.points.setNum(0,false);

	// directions
	int dx[] = NEIGHBOR_OFFS_XDX(0, (1));
	int dy[] = NEIGHBOR_OFFS_YDY(0, (1));

	// clear states before we proceed
	Nav_ClearCellStates( NAV_CLEAR_WORKSTATES );

	// check the start and dest points
	navcell_t& startCheck = Nav_GetCellStateAtGlobalPoint(start);
	navcell_t& endCheck = Nav_GetCellStateAtGlobalPoint(end);

	if(startCheck.flag == 0x1 || endCheck.flag == 0x1)
		return false;

	DkList<IVector2D> openSet(1024);	// we don't need closed set
	openSet.append(start);

	bool found = false;

	int totalIterations = 0;

	IVector2D dir = end-start;

	// go through all open pos
	while (openSet.numElem() > 0)
	{
		int bestOpenSetIdx = openSet.numElem() == 1 ? 0 : -1;

		if(bestOpenSetIdx == -1)
		{
			float minCost = 10000.0f;

			// search for best costless point
			for (int i = 0; i < openSet.numElem(); i++)
			{
				if(openSet[i] == end)
				{
					found = true;
					break;
				}

				navcell_t& cell = Nav_GetCellStateAtGlobalPoint(openSet[i]);
				ubyte val = Nav_GetTileAtGlobalPoint(openSet[i]);

				// calc cost and check
				float g = estimateDist(end, openSet[i]);
				float h = estimateDist(openSet[i] - IVector2D(dx[cell.pdir], dy[cell.pdir]), openSet[i]);
				float cost = g + h;	// regulate by cell priority

				if(cellPriority)
				{
					cost += val;
				}

				if (cost < minCost)
				{
					minCost = cost;
					bestOpenSetIdx = i;
				}
			}
		}

		if(found == true)
			break;

		if (bestOpenSetIdx == -1)
			break;

		IVector2D curPoint = openSet[bestOpenSetIdx];
		navcell_t& curCell = Nav_GetCellStateAtGlobalPoint(curPoint);

		// remove from open set
		openSet.fastRemoveIndex(bestOpenSetIdx);
		curCell.flag = 0x1;	// closed

		// STOP IF WE START FAILS
		if(totalIterations > iterationLimit)
			break;

		for (ubyte i = 0; i < 8; i++)
		{
			IVector2D nextPoint(curPoint + IVector2D(dx[i], dy[i]));

			navcell_t& nextCell = Nav_GetCellStateAtGlobalPoint(nextPoint);
			ubyte next = Nav_GetTileAtGlobalPoint(nextPoint);

			if(next == 0 || nextCell.flag == 0x1)	// wall or closed
				continue;

			ubyte trafficVal = Nav_GetTileAtGlobalPoint(nextPoint, true);

			if(trafficVal == 0)
				continue;

			totalIterations++;

			nextCell.flag = 0;
			nextCell.pdir = i;

			openSet.append( nextPoint );
		}
	}

	if(found)
	{
		IVector2D testPoint = end;
		navcell_t lastCell = Nav_GetCellStateAtGlobalPoint(testPoint);

		int lastDir = -1;

		do
		{
			// apply simplification
			if( lastDir != lastCell.pdir)
				result.points.append(testPoint);

			if(testPoint == start)
				break;

			IVector2D prevPoint = testPoint - IVector2D(dx[lastCell.pdir], dy[lastCell.pdir]);

			lastDir = lastCell.pdir;

			testPoint = prevPoint;
			lastCell = Nav_GetCellStateAtGlobalPoint(testPoint);
		}
		while(true);
	}

	return (result.points.numElem() > 0);
}

//------------------------------------------------------

void CGameLevel::GetDecalPolygons(decalprimitives_t& polys, const Volume& volume)
{
	polys.indices.setNum(0, false);
	polys.verts.setNum(0, false);

	for (int x = 0; x < m_wide; x++)
	{
		for (int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide + x;

			CScopedMutex m(m_mutex);

			CLevelRegion& reg = m_regions[idx];

			if(!reg.m_isLoaded)
				continue;

			if(!volume.IsBoxInside(reg.m_bbox.minPoint, reg.m_bbox.maxPoint))
				continue;

			reg.GetDecalPolygons(polys, volume);
		}
	}

}

//------------------------------------------------------------------------------------------------------------------------------------


bool IsStraightsOnSameLane( const straight_t& a, const straight_t& b )
{
	int dira = a.direction;
	int dirb = b.direction;

	if(dira > 2)
		dira -= 2;

	if(dirb > 2)
		dirb -= 2;

	return a.start[dira] == b.start[dirb];
}

IVector2D GetDirectionVecBy(const IVector2D& posA, const IVector2D& posB)
{
	return clamp(posB-posA, IVector2D(-1,-1),IVector2D(1,1));
}

IVector2D GetDirectionVec(int dirIdx)
{
	if(dirIdx > 3)
		dirIdx -= 4;

	int rX[4] = ROADNEIGHBOUR_OFFS_X(0);
	int rY[4] = ROADNEIGHBOUR_OFFS_Y(0);

	return IVector2D(rX[dirIdx], rY[dirIdx]);
}

bool IsPointOnStraight(const IVector2D& pos, const straight_t& straight)
{
	// make them floats
	float posOnLine = lineProjection((Vector2D)straight.start, (Vector2D)straight.end, (Vector2D)pos);

	IVector2D perpDirVec = GetDirectionVec( straight.direction+1 );

	IVector2D posComp = pos * perpDirVec;
	IVector2D strComp = straight.start * perpDirVec;

	if( posComp == strComp )
		return (posOnLine >= 0.0f && posOnLine <= 1.0f);

	return false;
}

int GetCellsBeforeStraight(const IVector2D& pos, const straight_t& straight)
{
	if(straight.direction == -1)
		return 1000;

	int dirV = 1 - (straight.direction % 2);

	int cmpA = min(pos[dirV], straight.start[dirV]);
	int cmpB = max(pos[dirV], straight.start[dirV]);

	return cmpB-cmpA;
}

int GetDirectionIndex(const IVector2D& vec)
{
	int rX[4] = ROADNEIGHBOUR_OFFS_X(0);
	int rY[4] = ROADNEIGHBOUR_OFFS_Y(0);

	IVector2D v = clamp(vec, IVector2D(-1,-1),IVector2D(1,1));

	for(int i = 0; i < 4; i++)
	{
		IVector2D r(rX[i], rY[i]);

		if(r == v)
			return i;
	}

	return 0;
}

int	GetDirectionIndexByAngles(const Vector3D& angles)
{
	Quaternion quat(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z));

	Vector3D dirForward = rotateVector(-vec3_forward, quat);
	float Yangle = RAD2DEG(atan2f(dirForward.z, dirForward.x));

	if (Yangle < 0.0) Yangle += 360.0f;
	Yangle = 360 - Yangle;

	int trafficDir = floor(Yangle / 90.0f);

	if(trafficDir > 3) 
		trafficDir -= 4;

	return trafficDir;
}

bool CompareDirection(int dirA, int dirB)
{
	int dA = dirA;
	if(dA < 0)
		dA += 4;

	if(dA > 3)
		dA -= 4;

	int dB = dirB;
	if(dB < 0)
		dB += 4;

	if(dB > 3)
		dB -= 4;

	return dA == dB;
}

bool IsOppositeDirectionTo(int dirA, int dirB)
{
	int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
	int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

	if( (dx[dirA] != dx[dirB] && (dx[dirA] + dx[dirB] == 0)) ||
		(dy[dirA] != dy[dirB] && (dy[dirA] + dy[dirB] == 0)))
		return true;

	return false;
}

int GetPerpendicularDir(int dir)
{
	int nDir = dir+1;

	if(nDir > 3)
		nDir -= 4;

	return nDir;
}

IVector2D GetPerpendicularDirVec(const IVector2D& vec)
{
	return IVector2D(vec.y,vec.x);
}