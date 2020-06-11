//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers level data and renderer
//				You might consider it as some ugly code :(
//////////////////////////////////////////////////////////////////////////////////

#include "world.h"

#include "heightfield.h"

#include "levfile.h"
#include "level.h"
#include "physics.h"
#include "utils/VirtualStream.h"

#include "utils/GeomTools.h"

#include "../shared_engine/physics/BulletConvert.h"

#include "eqGlobalMutex.h"

using namespace EqBulletUtils;
using namespace Threading;

ConVar r_enableLevelInstancing("r_enableLevelInstancing", "1", "Enable level models instancing if available", CV_ARCHIVE);
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
	m_occluderOffsets(NULL),
	m_numRegions(0),
	m_regionDataLumpOffset(0),
	m_occluderDataLumpOffset(0),
	m_levelName("Unnamed"),
	m_mutex(GetGlobalMutex(MUTEXPURPOSE_LEVEL_LOADER)),
	m_navOpenSet(1024)
{
	memset(m_instanceBuffer, 0, sizeof(m_instanceBuffer));
}

void CGameLevel::Cleanup()
{
	if(!m_regions)
		return;

	//DevMsg(DEVMSG_CORE, "Stopping loader thread...\n");
	//StopThread();

	WaitForThread();

	// remove regions first
	int num = m_wide*m_tall;

	DevMsg(DEVMSG_CORE, "Unloading regions...\n");

	UnloadRegions();

	delete [] m_regions;
	m_regions = NULL;

	if(m_regionOffsets)
		delete [] m_regionOffsets;
	m_regionOffsets = NULL;

	if(m_occluderOffsets)
		delete [] m_occluderOffsets;
	m_occluderOffsets = NULL;

	DevMsg(DEVMSG_CORE, "Unloading object defs...\n");
	for(int i = 0; i < m_objectDefs.numElem(); i++)
		delete m_objectDefs[i];

	m_objectDefs.clear();
	m_objectDefsCfg.clear();

	for (int i = 0; i < LEVEL_INSTANCE_BUFFERS; i++)
	{
		if (m_instanceBuffer[i])
			g_pShaderAPI->DestroyVertexBuffer(m_instanceBuffer[i]);

		m_instanceBuffer[i] = NULL;
	}

	m_wide = 0;
	m_tall = 0;
	m_occluderDataLumpOffset = 0;
	m_numRegions = 0;
	m_regionDataLumpOffset = 0;
}

bool CGameLevel::Load(const char* levelname)
{
	IFile* pFile = g_fileSystem->Open(varargs(LEVELS_PATH "%s.lev", levelname), "rb", SP_MOD);

	if(!pFile)
	{
		MsgError("can't find level '%s'\n", levelname);
		return false;
	}

	m_levelName = levelname;

	return CGameLevel::_Load(pFile);
}

bool CGameLevel::_Load(IFile* pFile)
{
	Cleanup();

	levHdr_t hdr;
	pFile->Read(&hdr, sizeof(levHdr_t), 1);

	if(hdr.ident != LEVEL_IDENT)
	{
		MsgError("**ERROR** invalid level '%s' \n", m_levelName.c_str());
		g_fileSystem->Close(pFile);
		return false;
	}

	if(hdr.version != LEVEL_VERSION)
	{
		MsgError("**ERROR** '%s' - unsupported lev file version\n", m_levelName.c_str());
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
			ReadRoadsLump(pFile);

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

			// then read and validate in lev file
			ReadObjectDefsLump(pFile);
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
	if(!IsRunning())
		StartWorkerThread( "LevelLoaderThread" );
#else
	// regenerate nav grid
	Nav_ClearCellStates( NAV_CLEAR_WORKSTATES );
#endif // EDITOR

	g_fileSystem->Close(pFile);

	float loadTime = Platform_GetCurrentTime()-startLoadTime;
	DevMsg(DEVMSG_GAME, "*** Level file read for %g seconds\n", loadTime);

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
		m_occluderOffsets = new int[m_wide*m_tall];
		m_regionDataLumpOffset = 0;
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
				m_regions[idx].m_isLoaded = clean;
			}

			m_regions[idx].m_regionIndex = idx;
			m_regions[idx].m_level = this;
			m_regions[idx].Init(m_cellsSize, IVector2D(x, y), Vector3D(x*nStepSize, 0, y*nStepSize) - center);
		}
	}

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	if(caps.isInstancingSupported && r_enableLevelInstancing.GetBool())
	{
		for (int i = 0; i < LEVEL_INSTANCE_BUFFERS; i++)
		{
			m_instanceBuffer[i] = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_MODEL_INSTANCES, sizeof(regObjectInstance_t));
			m_instanceBuffer[i]->SetFlags(VERTBUFFER_FLAG_INSTANCEDATA);
		}
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
	// init heightfield materials
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
	IFile* pFile = g_fileSystem->Open(varargs(LEVELS_PATH "%s.lev", m_levelName.c_str()), "rb", SP_MOD);

	if(!pFile)
		return;

	int regIdx = y*m_wide+x;

	LoadRegionAt(regIdx, pFile);

	g_fileSystem->Close(pFile);
}

//-------------------------------------------------------------------------------------------------

bool CGameLevel::LoadObjectDefs(const char* name)
{
	KeyValues kvs;
	if (!kvs.LoadFromFile(name))
		return false;

	kvkeybase_t* root = kvs.GetRootSection();
	for (int i = 0; i < root->keys.numElem(); i++)
	{
		kvkeybase_t* kbase = root->keys[i];
		if (!stricmp(kbase->name, "include") && kbase->ValueCount() > 0)
		{
			if (!LoadObjectDefs(KV_GetValueString(kbase)))
				MsgError("Cannot include object def file '%s'!\n", KV_GetValueString(kbase));

			continue;
		}

		CLevObjectDef* def = FindObjectDefByName(KV_GetValueString(kbase));

		// we have to add new def
		if (def)
		{
			MsgWarning("Object def '%s' is already registered\n", KV_GetValueString(kbase));
			continue;
		}

		def = new CLevObjectDef();

		def->m_name = KV_GetValueString(kbase, 0, "unnamed_def");
		def->m_defType = kbase->name;
		def->m_info.type = LOBJ_TYPE_OBJECT_CFG;

		m_objectDefsCfg.append(def);

		InitObjectDefFromKeyValues(def, kbase);
	}

	return true;
}

void CGameLevel::InitObjectDefFromKeyValues(CLevObjectDef* def, kvkeybase_t* defDesc)
{
	def->m_model = NULL;

	def->m_defType = defDesc->name;

	def->m_defKeyvalues = new kvkeybase_t();
	def->m_defKeyvalues->MergeFrom(defDesc, true);

#ifdef EDITOR
	def->m_placeable = KV_GetValueBool(defDesc->FindKeyBase("placeable"), 0, true);

	LoadDefLightData(def->m_lightData, defDesc);

	kvkeybase_t* editorModelKey = defDesc->FindKeyBase("editormodel");

	// use editor model
	if (editorModelKey)
	{
		// try precache model
		int modelIdx = g_studioModelCache->PrecacheModel(KV_GetValueString(editorModelKey));
		def->m_defModel = g_studioModelCache->GetModel(modelIdx);
	}
	else
#endif // EDITOR
	{
		kvkeybase_t* modelKey = defDesc->FindKeyBase("model");

		// try precache model
		// TODO: dynamic method instead
		if (modelKey)
		{
			// try precache model
			int modelIdx = g_studioModelCache->PrecacheModel(KV_GetValueString(modelKey));
			def->m_defModel = g_studioModelCache->GetModel(modelIdx);
		}
	}
}

CLevObjectDef* CGameLevel::FindObjectDefByName(const char* name) const
{
	for (int i = 0; i < m_objectDefsCfg.numElem(); i++)
	{
		if (!m_objectDefsCfg[i]->m_name.CompareCaseIns(name))
			return m_objectDefsCfg[i];
	}

	return nullptr;
}

void CGameLevel::ReadObjectDefsLump(IVirtualStream* stream)
{
#ifdef EDITOR
	m_objectDefIdCounter = 0;
#endif // EDITOR

	// load level object definition file
	EqString defFileName(varargs("scripts/levels/%s_objects.def", m_levelName.c_str()));

	if (!LoadObjectDefs(defFileName.c_str()))
	{
		MsgWarning("WARNING! '%s' cannot be loaded or not found\n", defFileName.c_str());

		defFileName = varargs("scripts/levels/%s_objects.def", "default");
		if (!LoadObjectDefs(defFileName.c_str()))
			CrashMsg("ERROR! '%s' cannot be loaded or not found\n", defFileName.c_str());
	}

	// now read the lump

	int numDefs = 0;
	int objectDefNamesSize = 0;
	stream->Read(&numDefs, 1, sizeof(int));
	stream->Read(&objectDefNamesSize, 1, sizeof(int));

	char* objectDefNames = new char[objectDefNamesSize];
	stream->Read(objectDefNames, 1, objectDefNamesSize);

	char* objectDefNamesPtr = objectDefNames;

	// load level models and associate objects from <levelname>_objects.txt
	for(int i = 0; i < numDefs; i++)
	{
		levObjectDefInfo_t defInfo;
		stream->Read(&defInfo, 1, sizeof(defInfo));

		CLevObjectDef* def = nullptr;

		if (defInfo.type == LOBJ_TYPE_OBJECT_CFG)
		{
			// validate object def
			def = FindObjectDefByName(objectDefNamesPtr);

			if (!def)
			{
				// add empty def to not mess up indexes
				def = new CLevObjectDef();
				def->m_name = objectDefNamesPtr;
				def->m_info = defInfo;
				def->m_defType = "INVALID";
				def->m_defModel = g_studioModelCache->GetModel(0);

				MsgError("Cannot find object def '%s'\n", objectDefNamesPtr);
			}

			m_objectDefs.append(def);
		}
		else if (defInfo.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			// read object def info
			def = new CLevObjectDef();
			def->m_name = objectDefNamesPtr;
			def->m_info = defInfo;

			def->m_modelOffset = stream->Tell();

#ifndef EDITOR
			stream->Seek(def->m_info.size, VS_SEEK_CUR);
#else
			def->PreloadModel(stream);
#endif // EDITOR

			m_objectDefs.append(def);
		}

#ifdef EDITOR
		if (def)
		{
			def->Ref_Grab();	// grab reference for editor to ensure that model will be not removed
			def->m_id = m_objectDefIdCounter++;
		}
#endif // EDITOR

		objectDefNamesPtr += strlen(objectDefNamesPtr)+1;
	}

	// add object defs that are new
	for (int i = 0; i < m_objectDefsCfg.numElem(); i++)
	{
		CLevObjectDef* def = m_objectDefsCfg[i];

		if (m_objectDefs.findIndex(def) == -1)
		{
#ifdef EDITOR
			def->Ref_Grab();
			def->m_id = m_objectDefIdCounter++;
#endif // EDITOR
			m_objectDefs.append(def);
		}
	}

	delete [] objectDefNames;
}

void CGameLevel::ReadHeightfieldsLump( IVirtualStream* stream )
{
	const int nStepSize = HFIELD_POINT_SIZE*m_cellsSize;
	const Vector3D center = Vector3D(m_wide*nStepSize, 0, m_tall*nStepSize)*0.5f - Vector3D(HFIELD_POINT_SIZE, 0, HFIELD_POINT_SIZE)*0.5f;

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

void CGameLevel::ReadRoadsLump(IVirtualStream* stream)
{
	int* roadOffsets = new int[m_wide*m_tall];
	stream->Read(roadOffsets, m_wide*m_tall, sizeof(int));

	int roadDataOffset = stream->Tell();

	// load roads for all regions
	for (int x = 0; x < m_wide; x++)
	{
		for (int y = 0; y < m_tall; y++)
		{
			int idx = y * m_wide + x;

			if (roadOffsets[idx] == -1)
				continue;

			int roadsOffset = roadDataOffset + roadOffsets[idx];
			stream->Seek(roadsOffset, VS_SEEK_SET);

			m_regions[idx].ReadLoadRoads(stream);
		}
	}

	delete[] roadOffsets;
}

CLevelRegion* CGameLevel::GetRegionAtPosition(const Vector3D& pos) const
{
	IVector2D regXY;

	if( PositionToRegionOffset(pos, regXY) )
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


bool CGameLevel::PositionToRegionOffset(const Vector3D& pos, IVector2D& outXYPos) const
{
	int cellSize = HFIELD_POINT_SIZE*m_cellsSize;
	float p_size = (1.0f / (float)cellSize);

	Vector2D center(m_wide*cellSize*-0.5f, m_tall*cellSize*-0.5f);

	Vector2D xz_pos = (pos.xz() - center) * p_size;

	outXYPos.x = xz_pos.x;
	outXYPos.y = xz_pos.y;

	if(outXYPos.x < 0 || outXYPos.x >= m_wide)
		return false;

	if(outXYPos.y < 0 || outXYPos.y >= m_tall)
		return false;

	return true;
}

void CGameLevel::FindRegionBoxRange(const BoundingBox& bbox, IVector2D& cr_min, IVector2D& cr_max, float extTolerance) const
{
	int cellSize = HFIELD_POINT_SIZE * m_cellsSize;
	float p_size = (1.0f / (float)cellSize);

	Vector2D center(m_wide*cellSize*-0.5f, m_tall*cellSize*-0.5f);

	const Vector2D xz_pos1 = (bbox.minPoint.xz() - center) * p_size;
	const Vector2D xz_pos2 = (bbox.minPoint.xz() - center) * p_size;

	if (extTolerance > 0)
	{
		const float EXT_TOLERANCE = extTolerance;	// the percentage of cell size
		const float EXT_TOLERANCE_REC = 1.0f - EXT_TOLERANCE;

		const float dx1 = xz_pos1.x - floor(xz_pos1.x);
		const float dy1 = xz_pos1.y - floor(xz_pos1.y);

		const float dx2 = xz_pos2.x - floor(xz_pos2.x);
		const float dy2 = xz_pos2.y - floor(xz_pos2.y);

		cr_min.x = (dx1 < EXT_TOLERANCE) ? (floor(xz_pos1.x) - 1) : floor(xz_pos1.x);
		cr_min.y = (dy1 < EXT_TOLERANCE) ? (floor(xz_pos1.y) - 1) : floor(xz_pos1.y);

		cr_max.x = (dx2 > EXT_TOLERANCE_REC) ? (floor(xz_pos2.x) + 1) : floor(xz_pos2.x);
		cr_max.y = (dy2 > EXT_TOLERANCE_REC) ? (floor(xz_pos2.y) + 1) : floor(xz_pos2.y);
	}
	else
	{
		cr_min.x = floor(xz_pos1.x);
		cr_min.y = floor(xz_pos1.y);
		cr_max.x = floor(xz_pos2.x);
		cr_max.y = floor(xz_pos2.y);
	}
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

	CScopedMutex m(m_mutex);

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

IVector2D CGameLevel::GlobalPointToRegionPosition(const IVector2D& point) const
{
	IVector2D regPos;
	regPos.x = floor((float)point.x / (float)(m_cellsSize));
	regPos.y = floor((float)point.y / (float)(m_cellsSize));

	return regPos;
}

void CGameLevel::GlobalToLocalPoint( const IVector2D& point, IVector2D& outLocalPoint, CLevelRegion** pRegion ) const
{
	IVector2D regPos = GlobalPointToRegionPosition(point);

	IVector2D globalStart;
	globalStart = regPos * m_cellsSize;

	outLocalPoint = point-globalStart;

	(*pRegion) = GetRegionAt(regPos);
}

void CGameLevel::LocalToGlobalPoint( const IVector2D& point, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const
{
	outGlobalPoint = pRegion->m_heightfield[0]->m_regionPos * m_cellsSize + point;
}

float CGameLevel::GetWaterLevel(const Vector3D& pos) const
{
	IVector2D tile = PositionToGlobalTilePoint(pos);
	return GetWaterLevelAt(tile);
}

float CGameLevel::GetWaterLevelAt(const IVector2D& tilePos) const
{
	// check tiles of all heightfields at this position
	CLevelRegion* pRegion = NULL;
	IVector2D localTile;
	GlobalToLocalPoint(tilePos, localTile, &pRegion);

	if(pRegion)
	{
		for(int i = 0; i < pRegion->GetNumHFields(); i++)
		{
			CHeightTileField* field = pRegion->GetHField(i);
			if(!field)
				continue;

			hfieldtile_t* tile = field->GetTile(localTile.x, localTile.y);

			if(tile == NULL || tile && !field->m_materials.inRange(tile->texture))
				continue;

			hfieldmaterial_t* mat = field->m_materials[tile->texture];
			if(!mat->material)
				continue;

			if(mat->material->GetFlags() & MATERIAL_FLAG_WATER)
				return (float)tile->height * HFIELD_HEIGHT_STEP;
		}
	}

	return -DrvSynUnits::MaxCoordInUnits;
}

//------------------------------------------------------------------------------------------------------------------------------------


straight_t CGameLevel::Road_GetStraightAtPoint( const IVector2D& point, int numIterations ) const
{
	CLevelRegion* pRegion = NULL;

	straight_t straight;
	IVector2D localPos;

	if( GetRegionAndTileAt(point, &pRegion, localPos ))
	{
		if (!pRegion->m_roads)
			return straight;

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
				break;

			if (!pNextRegion->m_roads)
				break;

			levroadcell_t& roadCell = pNextRegion->m_roads[nextTileIdx];

			// don't iterate through non-road surfaces
			if(roadCell.type == ROADTYPE_NOROAD)
				break;

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

straight_t CGameLevel::Road_GetStraightAtPos( const Vector3D& pos, int numIterations ) const
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return Road_GetStraightAtPoint(globPos, numIterations);
}

roadJunction_t CGameLevel::Road_GetJunctionAtPoint( const IVector2D& point, int numIterations ) const
{
//	CScopedMutex m(m_mutex);

	const int MAX_ITERATIONS_TO_JUNCTION_START = 0;

	CLevelRegion* pRegion = NULL;

	roadJunction_t junction;
	junction.breakIter = 0;
	junction.startIter = -1;

	IVector2D localPos;

	if( GetRegionAndTileAt(point, &pRegion, localPos ) )
	{
		if (!pRegion->m_roads)
			return junction;

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
		if(IsJunctionType((ERoadType)startCell.type))
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

			if (!pNextRegion)
				break;

			if (!pNextRegion->m_roads)
				break;

			levroadcell_t& roadCell = pNextRegion->m_roads[nextTileIdx];

			if( !IsJunctionType((ERoadType)roadCell.type) && junction.startIter != -1 )
				break;

			if(	!roadCell.type )
				break;

			if( IsJunctionType((ERoadType)roadCell.type) && junction.startIter == -1 )
			{
				if (i > MAX_ITERATIONS_TO_JUNCTION_START)
					break;

				junction.startIter = i;
			}

			// set the original direction
			junction.end = xyPos;
			LocalToGlobalPoint(xyPos, pNextRegion, junction.end);
		}
	}

	return junction;
}

roadJunction_t CGameLevel::Road_GetJunctionAtPos( const Vector3D& pos, int numIterations ) const
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return Road_GetJunctionAtPoint(globPos, numIterations);
}

void Road_GetJunctionExits(DkList<straight_t>& exits, const straight_t& road, const roadJunction_t& junc)
{
	bool gotSameDirection = false;
	bool sideJuncFound = false;

	if (junc.startIter >= 0)
	{
		for (int i = junc.startIter; i < junc.breakIter + 2; i++)
		{
			IVector2D dir = GetDirectionVecBy(junc.start, junc.end);

			IVector2D checkPos = junc.start + dir * i;

			if (!gotSameDirection)
			{
				straight_t straightroad = g_pGameWorld->m_level.Road_GetStraightAtPoint(checkPos, 16);

				if (straightroad.direction != -1 &&
					straightroad.breakIter > 1 &&
					!IsOppositeDirectionTo(road.direction, straightroad.direction) &&
					(straightroad.end != road.end) &&
					(straightroad.start != road.start))
				{
					straightroad.lane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint(straightroad.start);

					exits.append(straightroad);
					gotSameDirection = true;
				}
			}

			IVector2D dirCheckVec = GetPerpendicularDirVec(dir);

			// left and right
			for (int j = 0; j < 32; j++)
			{
				int checkDir = j;

				if (j >= 15)
					checkDir = -(checkDir - 16);

				IVector2D checkStraightPos = checkPos + dirCheckVec * checkDir;

				levroadcell_t* rcell = g_pGameWorld->m_level.Road_GetGlobalTileAt(checkStraightPos);

				if (rcell && rcell->type == ROADTYPE_NOROAD)
				{
					if (j >= 15)
						break;
					else
						j = 15;

					continue;
				}

				int dirIdx = GetDirectionIndex(dirCheckVec*sign(checkDir));

				// calc steering dir
				straight_t sideroad = g_pGameWorld->m_level.Road_GetStraightAtPoint(checkStraightPos, 8);

				if (sideroad.direction != -1 &&
					sideroad.direction == dirIdx &&
					(sideroad.end != road.end) &&
					(sideroad.start != road.start))
				{
					sideroad.lane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint(sideroad.end);

					sideJuncFound = true;
					exits.append(sideroad);
					break;
				}
			}
		}
	}
}

int	CGameLevel::Road_GetLaneIndexAtPoint( const IVector2D& point, int numIterations)
{
//	CScopedMutex m(m_mutex);

	CLevelRegion* pRegion = NULL;

	IVector2D localPos;

	int nLane = 0;

	if( GetRegionAndTileAt(point, &pRegion, localPos ) )
	{
		if (!pRegion->m_roads)
			return -1;

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

			if (!pNextRegion)
				break;

			int nextTileIdx = xyPos.y * m_cellsSize + xyPos.x;

			if (!pNextRegion->m_roads)
				break;

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

int	CGameLevel::Road_GetLaneIndexAtPos( const Vector3D& pos, int numIterations)
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return Road_GetLaneIndexAtPoint(globPos, numIterations);
}

int	CGameLevel::Road_GetWidthInLanesAtPoint( const IVector2D& point, int numIterations, int iterationsOnEmpty )
{
	//CScopedMutex m(m_mutex);

	CLevelRegion* pRegion = NULL;

	IVector2D localPos;

	// trace to the right
	int nLanes = Road_GetLaneIndexAtPoint(point, numIterations);

	if(nLanes == -1)
		return 0;

	// and left
	if( GetRegionAndTileAt(point, &pRegion, localPos ) )
	{
		if (!pRegion->m_roads)
			return -1;

		int tileIdx = localPos.y * m_cellsSize + localPos.x;

		levroadcell_t& startCell = pRegion->m_roads[tileIdx];

		if(	startCell.type != ROADTYPE_STRAIGHT &&
			startCell.type != ROADTYPE_PARKINGLOT )
			return -1;

		int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
		int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

		int laneDir = startCell.direction;
		int laneRowDir = GetPerpendicularDir(startCell.direction);

		int skeptLanes = 0;

		for(int i = 1; i < numIterations; i++)
		{
			IVector2D nextPos = localPos - IVector2D(dx[laneRowDir],dy[laneRowDir])*i;//localPos + IVector2D(dx[roadDir],dy[roadDir])*i;

			CLevelRegion* pNextRegion;

			nextPos = pRegion->GetTileAndNeighbourRegion(nextPos.x, nextPos.y, &pNextRegion);

			if (!pNextRegion)
				return -1;

			int nextTileIdx = nextPos.y * m_cellsSize + nextPos.x;

			if (!pNextRegion->m_roads)
				break;

			levroadcell_t& roadCell = pNextRegion->m_roads[nextTileIdx];

			if(	roadCell.type != ROADTYPE_STRAIGHT &&
				roadCell.type != ROADTYPE_PARKINGLOT )
			{
				iterationsOnEmpty--;

				if(iterationsOnEmpty >= 0)
				{
					skeptLanes++;
					continue;
				}
				else
					break;
			}

			// only parallels
			if( (roadCell.direction % 2) != (laneDir % 2))
				break;

			nLanes += skeptLanes;
			skeptLanes = 0;

			nLanes++;
		}
	}

	return nLanes;
}

int	CGameLevel::Road_GetWidthInLanesAtPos( const Vector3D& pos, int numIterations )
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return Road_GetWidthInLanesAtPoint(globPos, numIterations);
}

int	CGameLevel::Road_GetNumLanesAtPoint( const IVector2D& point, int numIterations )
{
	//CScopedMutex m(m_mutex);

	CLevelRegion* pRegion = NULL;

	IVector2D localPos;

	int nLanes = Road_GetLaneIndexAtPoint(point, numIterations);

	if(nLanes <= 0)
		return 0;

	if( GetRegionAndTileAt(point, &pRegion, localPos ))
	{
		if (!pRegion->m_roads)
			return 0;

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

			if(!pNextRegion)
				break;

			xyPos = pRegion->GetTileAndNeighbourRegion(xyPos.x, xyPos.y, &pNextRegion);
			int nextTileIdx = xyPos.y * m_cellsSize + xyPos.x;

			if (!pNextRegion->m_roads)
				break;

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

int	CGameLevel::Road_GetNumLanesAtPos( const Vector3D& pos, int numIterations )
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return Road_GetNumLanesAtPoint(globPos, numIterations);
}

levroadcell_t* CGameLevel::Road_GetGlobalTile(const Vector3D& pos, CLevelRegion** pRegion) const
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return Road_GetGlobalTileAt(globPos, pRegion);
}

levroadcell_t* CGameLevel::Road_GetGlobalTileAt(const IVector2D& point, CLevelRegion** pRegion) const
{
//	CScopedMutex m(m_mutex);

	IVector2D outXYPos;

	CLevelRegion* reg = NULL;

	if( GetRegionAndTileAt(point, &reg, outXYPos) )
	{
		if(pRegion)
			*pRegion = reg;

		if (!reg->m_roads)
			return NULL;

		int ridx = outXYPos.y*m_cellsSize + outXYPos.x;

		return &reg->m_roads[ridx];
	}

	return NULL;
}

bool CGameLevel::Road_FindBestCellForTrafficLight( IVector2D& out, const Vector3D& origin, int trafficDir, int juncIterations, bool leftHanded)
{

	IVector2D cellPos = PositionToGlobalTilePoint(origin);
	int laneRowDir = GetPerpendicularDir(trafficDir);

	int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
	int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

	IVector2D forwardDir = IVector2D(dx[trafficDir],dy[trafficDir]);

	// Left-handed road system simply by changing 'rightDir' to negative...
	IVector2D rightDir = IVector2D(dx[laneRowDir], dy[laneRowDir]) * (leftHanded ? -1 : 1);

	levroadcell_t* roadTile = NULL;
	
	/*
	first method
		Find passing straight from left (right) and back one step
	*/

	// do some more left steps
	for (int r = 0; r < 2; r++)
	{
		IVector2D roadTilePos = cellPos - rightDir * (r + 1) - forwardDir;
		
		roadTile = Road_GetGlobalTileAt(roadTilePos);

		if (roadTile &&
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

		for (int i = 0; i < juncIterations; i++)
		{
			IVector2D checkTilePos = roadTilePos - forwardDir * (i + 1);

			roadTile = Road_GetGlobalTileAt(checkTilePos);

			if (roadTile)
			{
				// don't iterate through non-road surfaces
				if (roadTile->type == ROADTYPE_NOROAD)
					break;

				if ((roadTile->type == ROADTYPE_STRAIGHT || roadTile->type == ROADTYPE_PARKINGLOT) &&
					roadTile->direction == trafficDir)
				{
					out = checkTilePos;
					return true;
				}
			}
		}

		/*
		third method
			Find passing straight using only backwards iteration through junctions
			If we have found straight in wrong direction (not perpendicular), just try searching to right (left)
		*/

		IVector2D checkTilePos = cellPos;

		for (int i = 0; i < juncIterations; i++)
		{
			checkTilePos -= forwardDir;

			roadTile = Road_GetGlobalTileAt(checkTilePos);

			if (roadTile && (roadTile->type == ROADTYPE_STRAIGHT || roadTile->type == ROADTYPE_PARKINGLOT))
			{
				// it's just beautiful we've found it
				if (roadTile->direction == trafficDir)
				{
					out = checkTilePos;
					return true;
				}

				// but what if we have wrong direction???
				if ((roadTile->direction % 2) == (trafficDir % 2))
				{
					// search to the right
					checkTilePos += rightDir;
					continue;
				}

				// has to break here
				break;
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------------------------------------------------------------------

CLevelRegion* CGameLevel::QueryNearestRegions( const Vector3D& pos, bool waitLoad )
{
	IVector2D posXY;
	if(PositionToRegionOffset(pos,posXY))
	{
		return QueryNearestRegions(posXY, waitLoad);
	}

	return nullptr;
}

CLevelRegion* CGameLevel::QueryNearestRegions( const IVector2D& point, bool waitLoad )
{
#ifdef EDITOR
	CEditorLevelRegion* region = (CEditorLevelRegion*)GetRegionAt(point);

	if (!region)
		return nullptr;

	int numNeedToLoad = !region->m_physicsPreview && (m_regionOffsets[point.y*m_wide + point.x] != -1);
#else
	CLevelRegion* region = GetRegionAt(point);

	if (!region)
		return nullptr;

	{
		CScopedMutex m(m_mutex);

		// if center region was not loaded, force wait
		if (!region->m_isLoaded && waitLoad)
			waitLoad = true;
		else
			waitLoad = false;
	}

	int numNeedToLoad = !region->m_isLoaded && (m_regionOffsets[point.y*m_wide + point.x] != -1);
#endif // EDITOR

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
			{
				CScopedMutex m(m_mutex);
#ifdef EDITOR
				CEditorLevelRegion* neditorreg = (CEditorLevelRegion*)nregion;

				if (!neditorreg->m_physicsPreview)
					numNeedToLoad += (m_regionOffsets[dy[i] * m_wide + dx[i]] != -1);
#else
				if (!nregion->m_isLoaded)
					numNeedToLoad += (m_regionOffsets[dy[i] * m_wide + dx[i]] != -1);
#endif // EDITOR
			}

			nregion->m_queryTimes.Increment();
		}
	}

	if( numNeedToLoad > 0 )
	{
		// signal loader
		SignalWork();

		if( waitLoad )
			WaitForThread();
	}

	return region;
}

void CGameLevel::CollectVisibleOccluders(occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition)
{
	CScopedMutex m(m_mutex);

	frustumOccluders.Clear();

	frustumOccluders.frustum = g_pGameWorld->m_frustum;

	// don't render too far?
	IVector2D camPosReg;

	// mark renderable regions
	if( PositionToRegionOffset(cameraPosition, camPosReg) )
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

void CGameLevel::Render(const Vector3D& cameraPosition, const occludingFrustum_t& occlFrustum, int nRenderFlags)
{
	bool renderTranslucency = (nRenderFlags & RFLAG_TRANSLUCENCY) > 0;

	// don't render too far?
	IVector2D camPosReg;

	// mark renderable regions
	if( PositionToRegionOffset(cameraPosition, camPosReg) )
	{
		CLevelRegion* region = GetRegionAt(camPosReg);

		// query this region
		if(region)
		{
			bool render = !(renderTranslucency && !region->m_hasTransparentSubsets); 

			if(render)
				region->Render( cameraPosition, occlFrustum, nRenderFlags );
		}
		
		int dx[8] = NEIGHBOR_OFFS_XDX(camPosReg.x, 1);
		int dy[8] = NEIGHBOR_OFFS_YDY(camPosReg.y, 1);

		// surrounding regions frustum
		for(int i = 0; i < 8; i++)
		{
			CLevelRegion* nregion = GetRegionAt(IVector2D(dx[i], dy[i]));

			if(nregion && !(renderTranslucency && !nregion->m_hasTransparentSubsets))
			{
				bool render = occlFrustum.IsBoxVisible( nregion->m_bbox );

				if(render)
					nregion->Render( cameraPosition, occlFrustum, nRenderFlags );
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

		// walk thru all defs
		int numObjectDefs = m_objectDefs.numElem();

		int instanceBufIdx = 0;

		for(int i = 0; i < numObjectDefs; i += LEVEL_INSTANCE_BUFFERS)
		{
			for (int j = 0; j < LEVEL_INSTANCE_BUFFERS; j++)
			{
				if (i+j >= numObjectDefs)
					break;

				if (m_objectDefs[i+j]->m_info.type != LOBJ_TYPE_INTERNAL_STATIC)
					continue;

				if (renderTranslucency && !m_objectDefs[i+j]->m_model->m_hasTransparentSubsets)
					continue;

				levObjInstanceData_t* instData = m_objectDefs[i+j]->m_instData;

				if (instData && instData->numInstances)
				{
					IVertexBuffer* instanceBuf = m_instanceBuffer[j];
					instanceBuf->Update(instData->instances, instData->numInstances, 0, true);
				}
			}

			for (int j = 0; j < LEVEL_INSTANCE_BUFFERS; j++)
			{
				if (i + j >= numObjectDefs)
					break;

				if (m_objectDefs[i+j]->m_info.type != LOBJ_TYPE_INTERNAL_STATIC)
					continue;

				if (renderTranslucency && !m_objectDefs[i+j]->m_model->m_hasTransparentSubsets)
					continue;

				levObjInstanceData_t* instData = m_objectDefs[i+j]->m_instData;

				if (instData && instData->numInstances)
				{
					g_pShaderAPI->SetVertexBuffer(m_instanceBuffer[j], 2);

					m_objectDefs[i+j]->m_model->Render(nRenderFlags);
					instData->numInstances = 0;
				}
			}
			
			/*
			CLevObjectDef* def = m_objectDefs[i];

			if(def->m_info.type != LOBJ_TYPE_INTERNAL_STATIC)
				continue;

			CLevelModel* model = def->m_model;
			levObjInstanceData_t* instData = def->m_instData;

			if(!model || !instData)
				continue;

			if(instData->numInstances == 0)
				continue;	
			
			if(renderTranslucency && !model->m_hasTransparentSubsets)
				continue;

			IVertexBuffer* instanceBuf = m_instanceBuffer[instanceBufIdx++];
			instanceBuf->Update(instData->instances, instData->numInstances, 0, true);

			instanceBufIdx = (instanceBufIdx+1) % LEVEL_INSTANCE_BUFFERS;

			// before lock we have to unbind our buffer
			g_pShaderAPI->ChangeVertexBuffer(NULL, 2);

			// set vertex buffer
			g_pShaderAPI->SetVertexBuffer(instanceBuf, 2 );

			model->Render(nRenderFlags);

			// reset instance count
			instData->numInstances = 0;

			// disable this vertex buffer or our cars will be drawn many times
			g_pShaderAPI->SetVertexBuffer( NULL, 2 );*/
		}

		materials->SetInstancingEnabled( false );

		// disable instancing
		g_pShaderAPI->SetVertexBuffer( NULL, 2 );
	}
}

int	CGameLevel::Run()
{
	UpdateRegionLoading();

	return 0;
}

int CGameLevel::UpdateRegionLoading()
{
	int numLoadedRegions = 0;

	float startLoadTime = Platform_GetCurrentTime();

	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;
#ifdef EDITOR
			// try preloading region
			if( !m_regions[idx].m_physicsPreview &&
				(m_regions[idx].m_queryTimes.GetValue() > 0) &&
				m_regionOffsets[idx] != -1 )
			{
				CEditorLevelRegion* reg = (CEditorLevelRegion*)&m_regions[idx];
				reg->Ed_InitPhysics();
#else
			{
				CScopedMutex m(m_mutex);
				if (m_regions[idx].m_isLoaded)
					continue;
			}

			// try preloading region
			if (!w_freeze.GetBool() &&
				(m_regions[idx].m_queryTimes.GetValue() > 0) &&
				m_regionOffsets[idx] != -1)
			{
				PreloadRegion(x,y);
#endif // EDITOR
				numLoadedRegions++;
				DevMsg(DEVMSG_CORE, "Region %d loaded\n", idx);
			}
		}
	}

	float loadTime = Platform_GetCurrentTime()-startLoadTime;
	if (numLoadedRegions)
		DevMsg(DEVMSG_CORE, "*** %d regions loaded for %g seconds\n", numLoadedRegions, loadTime);

	// wait for matsystem
	if (numLoadedRegions)
	{
		// force complete decals
		while (g_worldGlobals.decalsQueue.GetValue())
			Threading::Yield();

		materials->Wait();
	}

	return numLoadedRegions;
}

void CGameLevel::UnloadRegions()
{
	int numFreedRegions = 0;

	// unloading only available after loading
	if(!IsWorkDone())
		return;

	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			CLevelRegion& region = m_regions[idx];

			{
				CScopedMutex m(m_mutex);
				if (!region.m_isLoaded)
					continue;
			}

			if(m_regionOffsets[idx] != -1 )
			{
				// unload region
				region.Cleanup();
				region.m_scriptEventCallbackCalled = false;

				numFreedRegions++;
			}

			region.m_queryTimes.SetValue(0);
		}
	}
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

			CLevelRegion& region = m_regions[idx];

#ifndef EDITOR

			m_mutex.Lock();

			if(!w_freeze.GetBool() &&
				region.m_isLoaded &&
				(region.m_queryTimes.GetValue() <= 0) &&
				m_regionOffsets[idx] != -1 )
			{
				m_mutex.Unlock();

				// unload region
				region.Cleanup();
				region.m_scriptEventCallbackCalled = false;

				numFreedRegions++;
			}
			else
			{
				m_mutex.Unlock();
#pragma todo("SPAWN objects from UpdateRegions, not from loader thread")
			}
#endif // EDITOR

			region.m_queryTimes.SetValue(0);

			if( !region.m_scriptEventCallbackCalled )
			{
				if(func)
					(func)(&region, idx);

				region.m_scriptEventCallbackCalled = true;
			}
		}
	}

	return numFreedRegions;
}

void CGameLevel::RespawnAllObjects()
{
	CScopedMutex m(m_mutex);

	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;
			m_regions[idx].RespawnObjects();
		}
	}
}

extern ConVar nav_debug_map;

void CGameLevel::UpdateDebugMaps()
{
	if(!nav_debug_map.GetBool())
		return;

	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;
			m_regions[idx].UpdateDebugMaps();
		}
	}
}

// searches for object on level
bool CGameLevel::FindObjectOnLevel(levCellObject_t& objectInfo, const char* name, const char* defName) const
{
	if(name == NULL)
		return false;

	CLevObjectDef* def = NULL;
	int defIdx = -1;

	if(defName)
	{
		for(int i = 0; i < m_objectDefs.numElem(); i++)
		{
			if(!m_objectDefs[i]->m_name.CompareCaseIns(defName))
			{
				defIdx = i;
				def = m_objectDefs[i];
				break;
			}
		}
	}

	bool found = false;

	// first we try to find object on loaded regions
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int regIdx = y*m_wide+x;

			if(m_regionOffsets[regIdx] == -1)
				continue;
			
			// online objects
			found = m_regions[regIdx].FindObject(objectInfo, name, def);
			objectInfo.objectDefId = defIdx;

			if(found)
				break;
		}

		if(found)
			break;
	}

	if(found)
		return true;

	// open level file
	IFile* stream = g_fileSystem->Open(varargs(LEVELS_PATH "%s.lev", m_levelName.c_str()), "rb", SP_MOD);

	if(!stream)
		return false;

	// next we try to load list from regions
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int regIdx = y*m_wide+x;

			if(m_regionOffsets[regIdx] == -1)
				continue;
			
			// don't check loaded regions
			{
				CScopedMutex m(m_mutex);
				if (m_regions[regIdx].m_isLoaded)
					continue;
			}

			// offline objects

			// Read region header
			int regOffset = m_regionDataLumpOffset + m_regionOffsets[regIdx];
			stream->Seek(regOffset, VS_SEEK_SET);

			levRegionDataInfo_t	regdatahdr;
			stream->Read(&regdatahdr, 1, sizeof(levRegionDataInfo_t));

			// skip the defs
			for(int i = 0; i < regdatahdr.numObjectDefs; i++)
			{
				levObjectDefInfo_t defInfo;
				stream->Read(&defInfo, 1, sizeof(levObjectDefInfo_t));

				stream->Seek(defInfo.size, VS_SEEK_CUR);
			}

			levCellObject_t cellObj;

			// find cell object by reading it
			for(int i = 0; i < regdatahdr.numCellObjects; i++)
			{
				stream->Read(&cellObj, 1, sizeof(levCellObject_t));

				if(cellObj.name[0] == 0)
					continue;

				if((defIdx == -1 || defIdx >= 0 && cellObj.objectDefId == defIdx) && !stricmp(cellObj.name, name))
				{
					objectInfo = cellObj;
					found = true;
					break;
				}
			}
		}

		if(found)
			break;
	}

	g_fileSystem->Close(stream);

	return found;
}

#define OBSTACLE_STATIC_MAX_HEIGHT	(4.0f)
#define OBSTACLE_PROP_MAX_HEIGHT	(4.0f)

void CGameLevel::Nav_ClearDynamicObstacleMap()
{
	Nav_ClearCellStates(NAV_CLEAR_DYNAMIC_OBSTACLES);
}

void CGameLevel::Nav_AddObstacle(CLevelRegion* reg, regionObject_t* ref)
{
	if(ref == NULL)
	{
		MsgError("NULL reference of object found in region %d\n", reg->m_regionIndex);
		return;
	}

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

	CHeightTileField* hfield = reg->GetHField();

	if(def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
		// static model processing

		if((def->m_info.modelflags & LMODEL_FLAG_ISGROUND) ||
			(def->m_info.modelflags & LMODEL_FLAG_NOCOLLIDE))
			return;

		CLevelModel* model = def->m_model;

		if (!model)
			return;

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

			// check height
			{
				IVector2D tileXY0 = reg->PositionToCell(v0);
				IVector2D tileXY1 = reg->PositionToCell(v1);
				IVector2D tileXY2 = reg->PositionToCell(v2);

				hfieldtile_t* tile0 = hfield->GetTile(tileXY0.x, tileXY0.y);
				float tileHeight0 = tile0 ? tile0->height*HFIELD_HEIGHT_STEP : 0;

				hfieldtile_t* tile1 = hfield->GetTile(tileXY1.x, tileXY1.y);
				float tileHeight1 = tile1 ? tile1->height*HFIELD_HEIGHT_STEP : 0;

				hfieldtile_t* tile2 = hfield->GetTile(tileXY2.x, tileXY2.y);
				float tileHeight2 = tile2 ? tile2->height*HFIELD_HEIGHT_STEP : 0;

				if ((v0.y - tileHeight0) > OBSTACLE_STATIC_MAX_HEIGHT &&
					(v1.y - tileHeight1) > OBSTACLE_STATIC_MAX_HEIGHT &&
					(v2.y - tileHeight2) > OBSTACLE_STATIC_MAX_HEIGHT)
					continue;
			}

			Vector3D normal;
			ComputeTriangleNormal(v0, v1, v2, normal);

			//if(fabs(dot(normal,vec3_up)) > 0.8f)
			//	continue;

			//debugoverlay->Polygon3D(v0, v1, v2, ColorRGBA(1, 0, 1, 0.25f), 100.0f);

			BoundingBox vertbox;
			vertbox.AddVertex(v0);
			vertbox.AddVertex(v1);
			vertbox.AddVertex(v2);

			// other thread will not bother this op
			for (int i = 0; i < 2; i++)
			{
				// get a cell range
				IVector2D min, max;
				Nav_GetCellRangeFromAABB(vertbox.minPoint, vertbox.maxPoint, min, max, 1.5f, i);

				// in this range do...
				for (int y = min.y; y < max.y; y++)
				{
					for (int x = min.x; x < max.x; x++)
					{
						//Vector3D pointPos = Nav_GlobalPointToPosition(IVector2D(x, y));
						//debugoverlay->Box3D(pointPos - 0.5f, pointPos + 0.5f, ColorRGBA(1, 0, 1, 0.1f));

						ubyte& tile = Nav_GetTileAtGlobalPoint(IVector2D(x, y), false, i);
						tile = 0;
					}
				}
			}
		}

	}
	else if(def->m_defModel != NULL)
	{
		if(	def->m_defType == "debris" ||
			def->m_defType == "sheets" ||
			def->m_defType == "misc" ||
			def->m_defType == "scripted")
			return;

		// studio model
		studioPhysData_t* physData = &def->m_defModel->GetHWData()->physModel;

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

			// check height
			{
				IVector2D tileXY0 = reg->PositionToCell(v0);
				IVector2D tileXY1 = reg->PositionToCell(v1);
				IVector2D tileXY2 = reg->PositionToCell(v2);

				hfieldtile_t* tile0 = hfield->GetTile(tileXY0.x, tileXY0.y);
				float tileHeight0 = tile0 ? tile0->height*HFIELD_HEIGHT_STEP : 0;

				hfieldtile_t* tile1 = hfield->GetTile(tileXY1.x, tileXY1.y);
				float tileHeight1 = tile1 ? tile1->height*HFIELD_HEIGHT_STEP : 0;

				hfieldtile_t* tile2 = hfield->GetTile(tileXY2.x, tileXY2.y);
				float tileHeight2 = tile2 ? tile2->height*HFIELD_HEIGHT_STEP : 0;

				if ((v0.y - tileHeight0) > OBSTACLE_PROP_MAX_HEIGHT &&
					(v1.y - tileHeight1) > OBSTACLE_PROP_MAX_HEIGHT &&
					(v2.y - tileHeight2) > OBSTACLE_PROP_MAX_HEIGHT)
					continue;
			}

			Vector3D normal;
			ComputeTriangleNormal(v0, v1, v2, normal);

			//if (fabs(dot(normal, vec3_up)) > 0.8f)
			//	continue;

			//debugoverlay->Polygon3D(v0,v1,v2, ColorRGBA(1,1,0,0.25f), 100.0f);

			BoundingBox vertbox;
			vertbox.AddVertex(v0);
			vertbox.AddVertex(v1);
			vertbox.AddVertex(v2);

			// other thread will not bother this op
			for (int i = 0; i < 2; i++)
			{
				// get a cell range
				IVector2D min, max;
				Nav_GetCellRangeFromAABB(vertbox.minPoint, vertbox.maxPoint, min, max, 2.0f, i);

				// in this range do...
				for (int y = min.y; y < max.y; y++)
				{
					for (int x = min.x; x < max.x; x++)
					{
						//Vector3D pointPos = Nav_GlobalPointToPosition(IVector2D(x, y));
						//debugoverlay->Box3D(pointPos - 0.5f, pointPos + 0.5f, ColorRGBA(1, 0, 1, 0.1f));

						ubyte& tile = Nav_GetTileAtGlobalPoint(IVector2D(x, y), false, i);
						tile = 0;
					}
				}
			}
		}
	}
}

void CGameLevel::Nav_GetCellRangeFromAABB(const Vector3D& mins, const Vector3D& maxs, IVector2D& xy1, IVector2D& xy2, float offs, int subGrid) const
{
	xy1 = Nav_PositionToGlobalNavPoint(mins-offs, subGrid);
	xy2 = Nav_PositionToGlobalNavPoint(maxs+offs, subGrid);
}

void CGameLevel::Nav_GlobalToLocalPoint(const IVector2D& point, IVector2D& outLocalPoint, CLevelRegion** pRegion, int subGrid) const
{
	int navGridSize = m_cellsSize*s_navGridScales[subGrid];

	IVector2D regPos = point / navGridSize;

	outLocalPoint = point - (regPos * navGridSize);

	(*pRegion) = GetRegionAt(regPos);
}

void CGameLevel::Nav_LocalToGlobalPoint(const IVector2D& point, const CLevelRegion* pRegion, IVector2D& outGlobalPoint, int subGrid) const
{
	int navGridSize = m_cellsSize*s_navGridScales[subGrid];
	outGlobalPoint = pRegion->m_heightfield[0]->m_regionPos * navGridSize + point;
}

#define NAV_POINT_SIZE(subGrid) (HFIELD_POINT_SIZE * s_invNavGridScales[subGrid])

//
// conversions
//
IVector2D CGameLevel::Nav_PositionToGlobalNavPoint(const Vector3D& pos, int subGrid) const
{
	IVector2D gridWorldSize(m_wide, m_tall);
	gridWorldSize *= m_cellsSize*s_navGridScales[subGrid];

	float p_size = (1.0f / NAV_POINT_SIZE(subGrid));

	Vector2D xz_pos = pos.xz() * p_size + gridWorldSize/2;

	IVector2D out;

	out.x = xz_pos.x;// + 0.5f;
	out.y = xz_pos.y;// + 0.5f;

	return out;
}

Vector3D CGameLevel::Nav_GlobalPointToPosition(const IVector2D& point, int subGrid) const
{
	IVector2D outXYPos;
	CLevelRegion* pRegion;

	Nav_GlobalToLocalPoint(point, outXYPos, &pRegion, subGrid);

	if (pRegion)
	{
		CHeightTileField& defField = *pRegion->m_heightfield[0];

		hfieldtile_t* tile = defField.GetTile(outXYPos.x * s_invNavGridScales[subGrid], outXYPos.y * s_invNavGridScales[subGrid]);

		Vector3D tile_position;

		if (tile)
			tile_position = defField.m_position + Vector3D(outXYPos.x*NAV_POINT_SIZE(subGrid), tile->height*HFIELD_HEIGHT_STEP, outXYPos.y*NAV_POINT_SIZE(subGrid));
		else
			tile_position = defField.m_position + Vector3D(outXYPos.x*NAV_POINT_SIZE(subGrid), 0, outXYPos.y*NAV_POINT_SIZE(subGrid));

		float offsetFactor = float(s_navGridScales[subGrid]) - 1.0f;

		tile_position.x -= NAV_POINT_SIZE(subGrid)*0.5f*offsetFactor;
		tile_position.z -= NAV_POINT_SIZE(subGrid)*0.5f*offsetFactor;

		return tile_position;
	}

	return Vector3D(0, 0, 0);
}

navcell_t& CGameLevel::Nav_GetCellStateAtGlobalPoint(const IVector2D& point, int subGrid)
{
	int navSize = m_cellsSize*s_navGridScales[subGrid];

	IVector2D localPoint;
	CLevelRegion* reg;

	Nav_GlobalToLocalPoint(point, localPoint, &reg, subGrid);

	CScopedMutex m(m_mutex);

	if (reg && reg->m_isLoaded)
		return reg->m_navGrid[subGrid].cellStates[localPoint.y*navSize + localPoint.x];

	static navcell_t emptyCell;
	emptyCell.f = 10000.0f;
	emptyCell.g = 10000.0f;
	emptyCell.h = 10000.0f;
	emptyCell.flags = 0;
	emptyCell.parentDir = 7;

	return emptyCell;
}

ubyte& CGameLevel::Nav_GetTileAtGlobalPoint(const IVector2D& point, bool obstacles, int subGrid)
{
	int navSize = m_cellsSize*s_navGridScales[subGrid];

	IVector2D localPoint;
	CLevelRegion* reg;

	Nav_GlobalToLocalPoint(point, localPoint, &reg, subGrid);

	if (reg && reg->m_navGrid[subGrid].staticObst)
	{
		int idx = localPoint.y*navSize + localPoint.x;

		return obstacles ? reg->m_navGrid[subGrid].dynamicObst[idx] : reg->m_navGrid[subGrid].staticObst[idx];
	}

	return m_defaultNavTile;
}

ubyte& CGameLevel::Nav_GetTileAtPosition(const Vector3D& position, bool obstacles, int subGrid)
{
	IVector2D point = Nav_PositionToGlobalNavPoint(position, subGrid);

	return Nav_GetTileAtGlobalPoint(point, obstacles, subGrid);
}

navcell_t& CGameLevel::Nav_GetTileAndCellAtGlobalPoint(const IVector2D& point, ubyte& tile, int subGrid)
{
	int navSize = m_cellsSize*s_navGridScales[subGrid];

	IVector2D localPoint;
	CLevelRegion* reg;

	Nav_GlobalToLocalPoint(point, localPoint, &reg, subGrid);

	CScopedMutex m(m_mutex);

	if (reg && reg->m_isLoaded && reg->m_navGrid[subGrid].staticObst)
	{
		int idx = localPoint.y*navSize + localPoint.x;

		navGrid_t& grid = reg->m_navGrid[subGrid];

		tile = grid.staticObst[idx];//min(grid.dynamicObst[idx], grid.staticObst[idx]);
		return grid.cellStates[idx];
	}

	tile = 0;

	static navcell_t emptyCell;
	emptyCell.f = 10000.0f;
	emptyCell.g = 10000.0f;
	emptyCell.h = 10000.0f;
	emptyCell.flags = 0;
	emptyCell.parentDir = 7;

	return emptyCell;
}


void CGameLevel::Nav_ClearCellStates(ECellClearStateMode mode)
{
	for (int x = 0; x < m_wide; x++)
	{
		for (int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide + x;

			CLevelRegion& reg = m_regions[idx];

			{
				CScopedMutex m(m_mutex);
				if (!reg.m_isLoaded)
					continue;
			}

			if(reg.m_navGrid[0].staticObst && reg.m_navGrid[1].staticObst)
			{
				for (int i = 0; i < 2; i++)
				{
					switch (mode)
					{
					case NAV_CLEAR_WORKSTATES:
						reg.m_navGrid[i].ResetStates();
						break;
					case NAV_CLEAR_DYNAMIC_OBSTACLES:
						reg.m_navGrid[i].ResetDynamicObst();
						break;
					}
				}
			}
		}
	}
}

bool CGameLevel::Nav_FindPath(const Vector3D& start, const Vector3D& end, pathFindResult_t& result, int iterationLimit, int subGrid)
{
	IVector2D startPoint = Nav_PositionToGlobalNavPoint(start, subGrid);
	IVector2D endPoint = Nav_PositionToGlobalNavPoint(end, subGrid);

	return Nav_FindPath2D(startPoint, endPoint, result, iterationLimit, subGrid);
}

// d: cost
// d2: diagonal cost
float estimateDist(const IVector2D& src, const IVector2D& dest, float d, float d2)
{
	// use Euclidean method, result looks best for the driving game
	//return length(Vector2D(dest) - Vector2D(src));

	float dx = abs(src.x - dest.x);
	float dy = abs(src.y - dest.y);

	return d * (dx + dy) + (d2 - 2 * d) * min(dx, dy);
}

int sortOpenCells(const cellpoint_t& a, const cellpoint_t& b)
{
	return b.cell->f < a.cell->f;
}

bool CGameLevel::Nav_FindPath2D(const IVector2D& start, const IVector2D& end, pathFindResult_t& result, int iterationLimit, int subGrid)
{
	if(start == end)
		return false;

	result.gridSelector = subGrid;
	result.points.setNum(0,false);

	// directions
	int dx[] = NEIGHBOR_OFFS_XDX(0, (1));
	int dy[] = NEIGHBOR_OFFS_YDY(0, (1));

	// clear states before we proceed
	Nav_ClearCellStates( NAV_CLEAR_WORKSTATES );

	ubyte startTile = 0;
	ubyte endTile = Nav_GetTileAtGlobalPoint(end, false, subGrid);
	navcell_t& startCell = Nav_GetTileAndCellAtGlobalPoint(start, startTile, subGrid);

	// don't search from empty tiles
	if(startTile == 0 || endTile == 0)
		return false;

	m_navOpenSet.setNum(0, false);
	m_navOpenSet.append( cellpoint_t{start, &startCell} );

	bool foundPath = false;

	while (m_navOpenSet.numElem() > 0)
	{
		m_navOpenSet.shellSort( sortOpenCells, 0, m_navOpenSet.numElem()-1);

		IVector2D curPoint = m_navOpenSet[0].point;

		if(curPoint == end)
		{
			// goto path reconstruction
			foundPath = true;
			break;
		}

		ubyte curTile = 0;
		navcell_t& curCell = Nav_GetTileAndCellAtGlobalPoint(curPoint, curTile, subGrid);

		// make it closed
		curCell.flags |= 0x1;

		m_navOpenSet.fastRemoveIndex(0);

		iterationLimit--;
		if(iterationLimit <= 0)
			break;

		// go through all neighbours
		for (int i = 0; i < 8; i++)
		{
			IVector2D neighbourPoint(curPoint + IVector2D(dx[i], dy[i]));

			ubyte neighbourTile;
			navcell_t& neighbourCell = Nav_GetTileAndCellAtGlobalPoint(neighbourPoint, neighbourTile, subGrid);

			if(neighbourTile == 0)	// wall?
				continue;

			float distBetween = estimateDist(curPoint, neighbourPoint, 0.5f, 0.95f);

			float g = curCell.g + distBetween;  // dist from start + distance between the two nodes

			// if open or closed
			if(((neighbourCell.flags & 0x1) || (neighbourCell.flags & 0x2)) && neighbourCell.g < g)
				continue; // consider next successor

			float h = estimateDist(neighbourPoint, end, 0.5f, 0.95f);
			float f = g + h; // compute f(n')
			neighbourCell.f = f;
			neighbourCell.g = g;
			neighbourCell.h = h;

			// set direction to the parent
			neighbourCell.parentDir = i;

			// if it were closed, remove closed state
			if(neighbourCell.flags & 0x1)
				neighbourCell.flags &= ~0x1;

			// add to open set
			if(!(neighbourCell.flags & 0x2))
			{
				m_navOpenSet.append(cellpoint_t{neighbourPoint, &neighbourCell});
				neighbourCell.flags |= 0x2;
				/*
				// DEBUG DRAW
				Vector3D offset(0.0f, 0.25f, 0.0f);
				Vector3D pointPos = Nav_GlobalPointToPosition(neighbourPoint, subGrid) + offset;
				Vector3D pointPos2 = Nav_GlobalPointToPosition(curPoint, subGrid) + offset;

				float dispalyTime = 0.1f;

				debugoverlay->Box3D(pointPos - 0.15f, pointPos + 0.15f, ColorRGBA(1, 1, 0, 1.0f), dispalyTime);
				debugoverlay->Line3D(pointPos, pointPos + (pointPos2-pointPos)*0.25f, ColorRGBA(1, 1, 0, 1), ColorRGBA(1, 1, 0, 1), dispalyTime);
				*/
			}
		}
	}

	// reconstruct the path
	if(foundPath)
	{
		IVector2D testPoint = end;
		navcell_t lastCell = Nav_GetCellStateAtGlobalPoint(testPoint, subGrid);

		int lastDir = -1;

		IVector2D nonFailedPoint = testPoint;

		do
		{
			IVector2D prevPoint = testPoint - IVector2D(dx[lastCell.parentDir], dy[lastCell.parentDir]);

			result.points.append(testPoint);

			if(testPoint == start)
				break;

			lastDir = lastCell.parentDir;

			testPoint = prevPoint;
			lastCell = Nav_GetCellStateAtGlobalPoint(testPoint, subGrid);
		}
		while(true); // FIXME: limit!
	}

	return (result.points.numElem() > 1);
}

float CGameLevel::Nav_TestLine(const Vector3D& start, const Vector3D& end, bool obstacles, int subGrid)
{
	IVector2D startPoint = Nav_PositionToGlobalNavPoint(start, subGrid);
	IVector2D endPoint = Nav_PositionToGlobalNavPoint(end, subGrid);

	return Nav_TestLine2D(startPoint, endPoint, obstacles, subGrid);
}

float CGameLevel::Nav_TestLine2D(const IVector2D& start, const IVector2D& end, bool obstacles, int subGrid)
{
	int x1,y1,x2,y2;

	x1 = start.x;
	y1 = start.y;
	x2 = end.x;
	y2 = end.y;

    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;

	bool initializedInObstacle = Nav_GetTileAtGlobalPoint(start, obstacles, subGrid) == 0;

    for (;;)
	{
		ubyte& navCellValue = Nav_GetTileAtGlobalPoint(IVector2D(x1,y1), obstacles, subGrid);

		if(navCellValue == 0)
		{
			if(!initializedInObstacle)
				break;
		}
		else
			initializedInObstacle = false;

        if (x1 == x2 && y1 == y2)
			break;

        e2 = 2 * err;

        // EITHER horizontal OR vertical step (but not both!)
        if (e2 > dy)
		{
            err += dy;
            x1 += sx;
        }
		else if (e2 < dx)
		{
            err += dx;
            y1 += sy;
        }
    }

	if(start == IVector2D(x1,y1))
		return 0.0f;

	return clamp(lineProjection((Vector2D)start, (Vector2D)end, Vector2D(x1,y1)), 0.0f, 1.0f);
}

//------------------------------------------------------

void CGameLevel::GetDecalPolygons(decalPrimitives_t& polys, occludingFrustum_t* frustum)
{
	polys.indices.setNum(0, false);
	polys.verts.setNum(0, false);

	IVector2D rMin, rMax;
	FindRegionBoxRange(polys.settings.bounds, rMin, rMax, 0.15f);

	clamp(rMin, IVector2D(0, 0), IVector2D(m_wide-1, m_tall-1));
	clamp(rMax, IVector2D(0, 0), IVector2D(m_wide-1, m_tall-1));

	for (int x = rMin.x; x < rMax.x+1; x++)
	{
		for (int y = rMin.y; y < rMax.y+1; y++)
		{
			int idx = y*m_wide + x;

			CLevelRegion& reg = m_regions[idx];

			{
				CScopedMutex m(m_mutex);
				if (!reg.m_isLoaded)
					continue;
			}


			if(!polys.settings.clipVolume.IsBoxInside(reg.m_bbox.minPoint, reg.m_bbox.maxPoint))
				continue;

			if (frustum)
			{
				CScopedMutex m(m_mutex); // because CollectVisibleOccluders could be happening

				if(!frustum->IsBoxVisible(reg.m_bbox))
					continue;
			}

			reg.GetDecalPolygons(polys, frustum);
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

	if (dirIdx < 0)
		dirIdx += 4;

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

int GetCellsBeforeStraightStart(const IVector2D& pos, const straight_t& straight)
{
	if(straight.direction == -1)
		return 1000;

	int dirV = 1 - (straight.direction % 2);

	int cmpA = min(pos[dirV], straight.start[dirV]);
	int cmpB = max(pos[dirV], straight.start[dirV]);

	return cmpB-cmpA;
}

int GetCellsBeforeStraightEnd(const IVector2D& pos, const straight_t& straight)
{
	if(straight.direction == -1)
		return 1000;

	int dirV = 1 - (straight.direction % 2);

	int cmpA = min(pos[dirV], straight.end[dirV]);
	int cmpB = max(pos[dirV], straight.end[dirV]);

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

//---------------------------------------------------------------------------

void pathFindResult3D_t::InitFrom(pathFindResult_t& path, CEqCollisionObject* ignore)
{
	start = g_pGameWorld->m_level.Nav_GlobalPointToPosition(path.start, path.gridSelector);
	end = g_pGameWorld->m_level.Nav_GlobalPointToPosition(path.end, path.gridSelector);

	points.clear();

	btSphereShape _sphere(2.5f);
	eqPhysCollisionFilter collFilter;
	collFilter.type = EQPHYS_FILTER_TYPE_EXCLUDE;
	collFilter.flags = EQPHYS_FILTER_FLAG_DYNAMICOBJECTS | EQPHYS_FILTER_FLAG_FORCE_RAYCAST;
	collFilter.AddObject(ignore);

	CollisionData_t coll;

	Vector3D prevPoint = start;

	for (int i = 0; i < path.points.numElem(); i++)
	{
		Vector4D point(g_pGameWorld->m_level.Nav_GlobalPointToPosition(path.points[i], path.gridSelector), 0.0f);

		if (ignore != nullptr)
		{
			if (g_pPhysics->TestConvexSweep(&_sphere, identity(), prevPoint, point.xyz(), coll, OBJECTCONTENTS_SOLID_OBJECTS, &collFilter))
				point = Vector4D(point.xyz() + coll.normal * 2.0f, 0.0f);

			// store narowness factor
			point.w = coll.fract;
		}

		points.append(point);

		prevPoint = point.xyz();
	}
}