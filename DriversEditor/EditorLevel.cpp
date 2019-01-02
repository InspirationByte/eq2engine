//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor level data
//////////////////////////////////////////////////////////////////////////////////

#include "EditorLevel.h"
#include "level.h"
#include "IMaterialSystem.h"
#include "IDebugOverlay.h"
#include "world.h"

#include "VirtualStream.h"

#include "EditorActionHistory.h"

#include "utils/strtools.h"

// layer model header
struct layerModelFileHdr_t
{
	int		layerId;
	//int		modelId;
	char	name[80];
	int		size;
};

buildLayer_t::~buildLayer_t()
{
	for(int i = 0; i < models.numElem(); i++)
	{
		delete models[i];
	}
}

void buildLayerColl_t::Save(IVirtualStream* stream, kvkeybase_t* kvs)
{
	int numModels = 0;
	layerModelFileHdr_t hdr;

	for(int i = 0; i < layers.numElem(); i++)
	{
		buildLayer_t& layer = layers[i];

		// save both model and texture to kvs info
		kvkeybase_t* layerKvs = kvs->AddKeyBase("layer");

		for (int j = 0; j < layer.models.numElem(); j++)
		{
			CLayerModel* model = layer.models[j];

			layerKvs->AddKey("model", model->m_name.c_str());
		}

		numModels += layer.models.numElem();
	}

	stream->Write(&numModels, 1, sizeof(int));

	CMemoryStream modelStream;

	for(int i = 0; i < layers.numElem(); i++)
	{
		buildLayer_t& layer = layers[i];

		// write all models of layer
		for (int j = 0; j < layer.models.numElem(); j++)
		{
			CLayerModel* model = layer.models[j];

			// write model to temporary stream
			modelStream.Open(NULL, VS_OPEN_READ | VS_OPEN_WRITE, 2048);
			model->m_model->Save(&modelStream);

			// prepare header
			memset(&hdr, 0, sizeof(hdr));
			hdr.layerId = i;
			hdr.size = modelStream.Tell();
			strncpy(hdr.name, model->m_name.c_str(), sizeof(hdr.name));

			// write header
			stream->Write(&hdr, 1, sizeof(hdr));

			// write model
			stream->Write(modelStream.GetBasePointer(), 1, hdr.size);

			modelStream.Close();
		}
	}
}

void buildLayerColl_t::Load(IVirtualStream* stream, kvkeybase_t* kvs)
{
	// read layers first
	for(int i = 0; i < kvs->keys.numElem(); i++)
	{
		if(stricmp(kvs->keys[i]->name, "layer"))
			continue;

		layers.append(buildLayer_t());
	}

	// read models
	int numModels = 0;
	stream->Read(&numModels, 1, sizeof(int));

	layerModelFileHdr_t hdr;

	for(int i = 0; i < numModels; i++)
	{
		stream->Read(&hdr, 1, sizeof(hdr));

		// make layer model
		CLayerModel* mod = new CLayerModel();
		layers[hdr.layerId].models.append(mod);

		mod->m_name = hdr.name;

		// read model
		int modOffset = stream->Tell();
		mod->m_model = new CLevelModel();
		mod->m_model->Load(stream);

		mod->SetDirtyPreview();
		mod->RefreshPreview();

		int modelSize = stream->Tell() - modOffset;

		// a bit paranoid
		ASSERT(hdr.size == modelSize);
	}
}

//-------------------------------------------------------

buildingSource_t::buildingSource_t(buildingSource_t& copyFrom)
{
	InitFrom(copyFrom);
}

buildingSource_t::~buildingSource_t()
{
	delete model;
}

void buildingSource_t::InitFrom(buildingSource_t& from)
{
	points.clear();

	layerColl = from.layerColl;
	order = from.order;

	for(DkLLNode<buildSegmentPoint_t>* lln = from.points.goToFirst(); lln != NULL; lln = from.points.goToNext())
	{
		points.addLast(lln->object);
	}

	// I do not copy model for re-generation reason
	model = NULL;
	modelPosition = vec3_zero;
}

void buildingSource_t::ToKeyValues(kvkeybase_t* kvs)
{
	kvs->SetName( layerColl->name.c_str() );
	kvs->SetKey("order", order);
	kvs->SetKey("objectId", regObjectId);

	kvkeybase_t* positionKey = kvs->AddKeyBase("position", nullptr, KVPAIR_FLOAT);
	positionKey->AddValue(modelPosition.x);
	positionKey->AddValue(modelPosition.x);
	positionKey->AddValue(modelPosition.x);

	kvkeybase_t* kvPoints = kvs->AddKeyBase("points");

	for(DkLLNode<buildSegmentPoint_t>* lln = points.goToFirst(); lln != NULL; lln = points.goToNext())
	{
		kvkeybase_t* kvp = kvPoints->AddKeyBase(varargs("%d", lln->object.layerId));
		kvp->AddValue((float)lln->object.position.x);
		kvp->AddValue((float)lln->object.position.y);
		kvp->AddValue((float)lln->object.position.z);
		kvp->AddValue(lln->object.scale);
		kvp->AddValue(lln->object.modelId);
	}
}

void buildingSource_t::FromKeyValues(kvkeybase_t* kvs)
{
	loadBuildingName = kvs->name;
	order = KV_GetValueInt(kvs->FindKeyBase("order"));
	regObjectId = KV_GetValueInt(kvs->FindKeyBase("objectId"));
	modelPosition = KV_GetVector3D(kvs->FindKeyBase("position"));

	kvkeybase_t* kvPoints = kvs->FindKeyBase("points");
	for(int i = 0; i < kvPoints->keys.numElem(); i++)
	{
		kvkeybase_t* pkv = kvPoints->keys[i];

		buildSegmentPoint_t newSeg;

		newSeg.layerId = atoi(pkv->name);
		newSeg.position = KV_GetVector3D(pkv, 0);
		newSeg.scale = KV_GetValueFloat(pkv, 3);
		newSeg.modelId = KV_GetValueInt(pkv, 4, 0);
		
		points.addLast( newSeg );
	}
}

//-------------------------------------------------------

void CalculateBuildingSegmentTransform(Matrix4x4& partTransform, 
	buildLayer_t& layer, 
	const Vector3D& startPoint, 
	const Vector3D& endPoint, 
	int order, 
	Vector3D& size, float scale,
	int iteration )
{
	// first we find angle
	Vector3D partDir = normalize(order > 0 ? (endPoint - startPoint) : (startPoint - endPoint));

	// make right vector by order
	Vector3D forwardVec = cross(vec3_up, partDir);

	Vector3D yoffset(0,size.y * 0.5f,0);

	partTransform = translate(startPoint + yoffset + partDir*size.x*scale*0.5f*float(order * (iteration*2 + 1))) * Matrix4x4(Matrix3x3(partDir, vec3_up, forwardVec)*scale3(-scale,1.0f,1.0f));
}

int GetLayerSegmentIterations(const buildSegmentPoint_t& start, const buildSegmentPoint_t& end, float layerXSize)
{
	float remainingLength = length(end.position - start.position);
	return (int)((remainingLength / layerXSize) + 0.5f);
}
/*
int GetLayerSegmentIterations(const buildSegmentPoint_t& start, const buildSegmentPoint_t& end, buildLayer_t& layer)
{
	float len = GetSegmentLength(layer) * start.scale;

	return GetLayerSegmentIterations(start, end, len);
}
*/
float GetSegmentLength( buildLayer_t& layer, int modelId )
{
	// model set must have equal dimensions
	// take the first floor
	CLevelModel* model = layer.models[modelId]->m_model;

	// compute iteration count from model width
	const BoundingBox& modelBox = model->GetAABB();

	Vector3D size = modelBox.GetSize();
	return size.x;
}

//
// Rendering the building
//
void RenderBuilding( buildingSource_t* building, buildSegmentPoint_t* extraSegment )
{
	if(building->model != NULL && !extraSegment)
	{
		Matrix4x4 transform = translate(building->modelPosition) * identity4();
		
		materials->SetMatrix(MATRIXMODE_WORLD, transform);

		// draw model
		BoundingBox aabb;

		building->model->Render(0,aabb);

		return;
	}

	if(building->points.getCount() == 0 || building->layerColl == NULL)
		return;

	// Render dynamic preview of the building we're making
	DkList<buildSegmentPoint_t> allPoints;

	for(DkLLNode<buildSegmentPoint_t>* lln = building->points.goToFirst(); lln != NULL; lln = building->points.goToNext())
	{
		allPoints.append(lln->object);
	}

	if(extraSegment)
	{
		buildSegmentPoint_t& lastPoint = allPoints[allPoints.numElem() - 1];

		// just copy render values
		lastPoint.modelId = extraSegment->modelId;
		lastPoint.layerId = extraSegment->layerId;
		lastPoint.scale = extraSegment->scale;

		allPoints.append(*extraSegment);
	}

	Matrix4x4 partTransform;
	BoundingBox tempBBox;

	for(int i = 1; i < allPoints.numElem(); i++)
	{
		buildSegmentPoint_t& start = allPoints[i-1];
		buildSegmentPoint_t& end = allPoints[i];

		// draw models or walls
		buildLayer_t& layer = building->layerColl->layers[ start.layerId ];

		CLevelModel* model = layer.models[start.modelId]->m_model;

		// compute iteration count from model width
		const BoundingBox& modelBox = model->GetAABB();

		Vector3D size = modelBox.GetSize();
		float modelLen = size.x*start.scale;

		int numIterations = GetLayerSegmentIterations(start, end, modelLen);

		// calculate transformation for each iteration
		for(int iter = 0; iter < numIterations; iter++)
		{
			CalculateBuildingSegmentTransform( partTransform, layer, start.position, end.position, building->order, size, start.scale, iter );

			materials->SetMatrix(MATRIXMODE_WORLD, partTransform);
			model->Render(0, tempBBox);
		}

	}
}

struct genBatch_t
{
	DkList<lmodeldrawvertex_t>	verts;
	DkList<uint16>				indices;
	IMaterial*					material;
};

class CBuildingModelGenerator
{
public:
	void				AppendModel( CLevelModel* model, const Matrix4x4& transform);
	CLevelModel*		GenerateModel();

	const BoundingBox&	GetAABB() const {return m_aabb;}

protected:

	genBatch_t*			GetBatch(IMaterial* material);

	DkList<genBatch_t*>	m_batches;

	BoundingBox m_aabb;
};

void CBuildingModelGenerator::AppendModel(CLevelModel* model, const Matrix4x4& transform )
{
	Matrix3x3 rotTransform = transform.getRotationComponent();

	for(int i = 0; i < model->m_numBatches; i++)
	{
		lmodel_batch_t* srcBatch = &model->m_batches[i];

		genBatch_t* destBatch = GetBatch( srcBatch->pMaterial );

		for (uint16 j = 0; j < srcBatch->numIndices; j++)
		{
			int srcIdx = model->m_indices[srcBatch->startIndex+ j ];
			lmodeldrawvertex_t destVtx = model->m_verts[srcIdx];

			destVtx.position = (transform*Vector4D(destVtx.position, 1.0f)).xyz();
			destVtx.normal = rotTransform*destVtx.normal;

			int destIdx = destBatch->verts.addUnique(destVtx);

			destBatch->indices.append(destIdx);

			// extend aabb
			m_aabb.AddVertex( destVtx.position );
		}
	}
}

CLevelModel* CBuildingModelGenerator::GenerateModel()
{
	CLevelModel* model = new CLevelModel();
	model->m_numBatches = m_batches.numElem();
	model->m_batches = new lmodel_batch_t[model->m_numBatches];

	DkList<lmodeldrawvertex_t>	allVerts;
	DkList<uint16>				allIndices;

	Vector3D center = m_aabb.GetCenter();

	// join the batches
	for(int i = 0; i < model->m_numBatches; i++)
	{
		// copy batch
		genBatch_t* srcBatch = m_batches[i];
		lmodel_batch_t& destBatch = model->m_batches[i];

		// setup
		destBatch.numVerts = srcBatch->verts.numElem();
		destBatch.numIndices = srcBatch->indices.numElem();

		destBatch.pMaterial = srcBatch->material;
		destBatch.pMaterial->Ref_Grab();

		// setup
		destBatch.startVertex = allVerts.numElem();
		destBatch.startIndex = allIndices.numElem();

		// copy contents
		uint16 firstIndex = allVerts.numElem();

		// add indices
		for(int j = 0; j < srcBatch->indices.numElem(); j++)
			allIndices.append(firstIndex + srcBatch->indices[j]);

		// add vertices
		for(int j = 0; j < srcBatch->verts.numElem(); j++)
		{
			lmodeldrawvertex_t vert = srcBatch->verts[j];
			vert.position -= center;

			allVerts.append( vert );
		}
	}

	// correct aabb
	m_aabb.minPoint -= center;
	m_aabb.maxPoint -= center;

	model->m_bbox = m_aabb;

	// copy vbo's
	model->m_numVerts = allVerts.numElem();
	model->m_numIndices = allIndices.numElem();

	model->m_verts = new lmodeldrawvertex_t[model->m_numVerts];
	model->m_indices = new uint16[model->m_numIndices];

	memcpy(model->m_verts, allVerts.ptr(), sizeof(lmodeldrawvertex_t)*model->m_numVerts);
	memcpy(model->m_indices, allIndices.ptr(), sizeof(uint16)*model->m_numIndices);

	model->GenereateRenderData();

	return model;
}

genBatch_t* CBuildingModelGenerator::GetBatch(IMaterial* material)
{
	for(int i = 0; i < m_batches.numElem(); i++)
	{
		if(m_batches[i]->material == material)
			return m_batches[i];
	}

	genBatch_t* newBatch = new genBatch_t();
	newBatch->material = material;

	m_batches.append(newBatch);

	return newBatch;
}

//
// Generates new levelmodel of building
// Returns local-positioned model, and it's position in the world
//
bool GenerateBuildingModel( buildingSource_t* building )
{
	if(building->points.getCount() == 0 || building->layerColl == NULL)
		return false;

	// Render dynamic preview of the building we're making
	DkList<buildSegmentPoint_t> allPoints;

	for(DkLLNode<buildSegmentPoint_t>* lln = building->points.goToFirst(); lln != NULL; lln = building->points.goToNext())
	{
		allPoints.append(lln->object);
	}

	Matrix4x4 partTransform;

	CBuildingModelGenerator generator;

	for(int i = 1; i < allPoints.numElem(); i++)
	{
		buildSegmentPoint_t& start = allPoints[i-1];
		buildSegmentPoint_t& end = allPoints[i];

		// draw models or walls
		buildLayer_t& layer = building->layerColl->layers[ start.layerId ];

		CLevelModel* model = layer.models[start.modelId]->m_model;

		// compute iteration count from model width
		const BoundingBox& modelBox = model->GetAABB();

		Vector3D size = modelBox.GetSize();
		float modelLen = size.x*start.scale;

		int numIterations = GetLayerSegmentIterations(start, end, modelLen);

		// calculate transformation for each iteration
		for(int iter = 0; iter < numIterations; iter++)
		{
			CalculateBuildingSegmentTransform( partTransform, layer, start.position, end.position, building->order, size, start.scale, iter );

			generator.AppendModel(model, partTransform);
		}
	}

	if(building->model != NULL)
	{
		if(building->model->Ref_Drop())
		{
			delete building->model;
			building->model = NULL;
		}
	}
	
	building->modelPosition = generator.GetAABB().GetCenter();
	building->model = generator.GenerateModel();

	if(building->model)
		building->model->Ref_Grab();

	return (building->model != NULL);
}

//-------------------------------------------------------------------------------------------

bool CEditorLevel::Load(const char* levelname, kvkeybase_t* kvDefs)
{
	bool result = CGameLevel::Load(levelname, kvDefs);

	if(result)
	{
		LoadEditorBuildings(levelname);
	}

	return result;
}

bool CEditorLevel::Save(const char* levelname, bool isFinal)
{
	IFile* pFile = g_fileSystem->Open(varargs("levels/%s.lev", levelname), "wb", SP_MOD);

	if(!pFile)
	{
		MsgError("can't open '%s' for write\n", levelname);
		return false;
	}

	Msg("Saving level '%s'...\n", levelname);

	levHdr_t hdr;
	hdr.ident = LEVEL_IDENT;
	hdr.version = LEVEL_VERSION;

	pFile->Write(&hdr, sizeof(levHdr_t), 1);

	// write models first if available
	WriteObjectDefsLump( pFile );

	// write region info
	WriteLevelRegions( pFile, isFinal );

	// editor-related stuff
	SaveEditorBuildings( levelname );

	levLump_t endLump;
	endLump.type = LEVLUMP_ENDMARKER;
	endLump.size = 0;

	// write lump header and data
	pFile->Write(&endLump, 1, sizeof(levLump_t));

	g_fileSystem->Close(pFile);

	m_levelName = levelname;

	return true;
}

bool CEditorLevel::LoadPrefab(const char* prefabName)
{
	IFile* pFile = g_fileSystem->Open(varargs("editor_prefabs/%s.pfb", prefabName), "rb", SP_MOD);

	if(!pFile)
	{
		MsgError("can't find prefab '%s'\n", prefabName);
		return false;
	}

	Msg("Loading prefab '%s'...\n", prefabName);

	m_levelName = prefabName;

	kvkeybase_t empty;
	return CGameLevel::_Load(pFile, &empty);
}

bool CEditorLevel::SavePrefab(const char* prefabName)
{
	IFile* pFile = g_fileSystem->Open(varargs("editor_prefabs/%s.pfb", prefabName), "wb", SP_MOD);

	if(!pFile)
	{
		MsgError("can't open '%s' for write\n", prefabName);
		return false;
	}

	Msg("Saving prefab '%s'...\n", prefabName);

	levHdr_t hdr;
	hdr.ident = LEVEL_IDENT;
	hdr.version = LEVEL_VERSION;

	pFile->Write(&hdr, sizeof(levHdr_t), 1);

	// write models first if available
	WriteObjectDefsLump( pFile );

	// write region info
	WriteLevelRegions( pFile, false );

	levLump_t endLump;
	endLump.type = LEVLUMP_ENDMARKER;
	endLump.size = 0;

	g_fileSystem->Close(pFile);

	return true;
}

void CEditorLevel::WriteObjectDefsLump(IVirtualStream* stream)
{
	if(m_objectDefs.numElem() == 0)
		return;

	CMemoryStream objectDefsLump;
	objectDefsLump.Open(NULL, VS_OPEN_WRITE, 2048);

	int numModels = m_objectDefs.numElem();
	objectDefsLump.Write(&numModels, 1, sizeof(int));

	// write model names
	CMemoryStream defNames;
	defNames.Open(NULL, VS_OPEN_WRITE, 2048);

	char nullSymbol = '\0';

	for(int i = 0; i < m_objectDefs.numElem(); i++)
	{
		defNames.Print(m_objectDefs[i]->m_name.c_str());
		defNames.Write(&nullSymbol, 1, 1);
	}

	defNames.Write(&nullSymbol, 1, 1);

	int defNamesLength = defNames.Tell();

	objectDefsLump.Write(&defNamesLength, 1, sizeof(int));
	objectDefsLump.Write(defNames.GetBasePointer(), 1, defNamesLength);

	// write model data
	for(int i = 0; i < m_objectDefs.numElem(); i++)
	{
		CMemoryStream modeldata;
		modeldata.Open(NULL, VS_OPEN_WRITE, 2048);

		levObjectDefInfo_t& defInfo = m_objectDefs[i]->m_info;

		if(defInfo.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			m_objectDefs[i]->m_model->Save( &modeldata );
		}

		defInfo.size = modeldata.Tell();

		objectDefsLump.Write(&defInfo, 1, sizeof(levObjectDefInfo_t));
		objectDefsLump.Write(modeldata.GetBasePointer(), 1, defInfo.size);
	}

	// compute lump size
	levLump_t modelLumpInfo;
	modelLumpInfo.type = LEVLUMP_OBJECTDEFS;
	modelLumpInfo.size = objectDefsLump.Tell();

	// write lump header and data
	stream->Write(&modelLumpInfo, 1, sizeof(levLump_t));
	stream->Write(objectDefsLump.GetBasePointer(), 1, modelLumpInfo.size);
}

void CEditorLevel::WriteHeightfieldsLump(IVirtualStream* stream)
{
	CMemoryStream hfielddata;
	hfielddata.Open(NULL, VS_OPEN_WRITE, 16384);

	// build region offsets
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			CLevelRegion* reg = &m_regions[idx];

			// write each region if not empty
			if( !reg->IsRegionEmpty() )
			{
				// heightfield index
				hfielddata.Write(&idx, 1, sizeof(int));

				int numFields = reg->GetNumHFields();

				int numNonEmptyFields = reg->GetNumNomEmptyHFields();
				hfielddata.Write(&numNonEmptyFields, 1, sizeof(int));

				// write region heightfield data
				for(int i = 0; i < numFields; i++)
				{
					if(	reg->m_heightfield[i] &&
						!reg->m_heightfield[i]->IsEmpty())
					{
						int hfieldIndex = i;
						hfielddata.Write(&hfieldIndex, 1, sizeof(int));

						reg->m_heightfield[i]->WriteToStream( &hfielddata );
					}
				}
			}
		}
	}

	// compute lump size
	levLump_t hfieldLumpInfo;
	hfieldLumpInfo.type = LEVLUMP_HEIGHTFIELDS;
	hfieldLumpInfo.size = hfielddata.Tell();

	// write lump header and data
	stream->Write(&hfieldLumpInfo, 1, sizeof(levLump_t));
	stream->Write(hfielddata.GetBasePointer(), 1, hfielddata.Tell());
}

struct zoneRegions_t
{
	EqString zoneName;
	DkList<int> regionList;
};

void CEditorLevel::WriteLevelRegions(IVirtualStream* file, bool isFinal)
{
	// ---------- LEVLUMP_REGIONINFO ----------
	levRegionMapInfo_t regMapInfoHdr;

	regMapInfoHdr.numRegionsWide = m_wide;
	regMapInfoHdr.numRegionsTall = m_tall;
	regMapInfoHdr.cellsSize = m_cellsSize;
	regMapInfoHdr.numRegions = 0; // to be proceed

	int* regionOffsetArray = new int[m_wide*m_tall];
	int* roadOffsetArray = new int[m_wide*m_tall];
	int* occluderLstOffsetArray = new int[m_wide*m_tall];

	// allocate writeable data streams
	CMemoryStream regionDataStream;
	regionDataStream.Open(NULL, VS_OPEN_WRITE, 2048);

	CMemoryStream roadDataStream;
	roadDataStream.Open(NULL, VS_OPEN_WRITE, 2048);

	CMemoryStream occluderDataStream;
	occluderDataStream.Open(NULL, VS_OPEN_WRITE, 2048);

	CMemoryStream zoneDataStream;
	zoneDataStream.Open(NULL, VS_OPEN_WRITE, 2048);

	DkList<zoneRegions_t> zoneRegionList;

	// build region offsets
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			CEditorLevelRegion& reg = *(CEditorLevelRegion*)&m_regions[idx];

			// write each region if not empty
			if( !reg.IsRegionEmpty() )
			{
				regMapInfoHdr.numRegions++;

				regionOffsetArray[idx] = regionDataStream.Tell();
				roadOffsetArray[idx] = roadDataStream.Tell();
				occluderLstOffsetArray[idx] = occluderDataStream.Tell();

				// write em all
				reg.WriteRegionData( &regionDataStream, m_objectDefs, isFinal );
				reg.WriteRegionRoads( &roadDataStream );
				reg.WriteRegionOccluders( &occluderDataStream );

				for(int i = 0; i < reg.m_zones.numElem(); i++)
				{
					regZone_t& zone = reg.m_zones[i];

					// find needed region list
					zoneRegions_t* zoneRegions = zoneRegionList.findFirst( [zone](const zoneRegions_t& x) {return !x.zoneName.CompareCaseIns(zone.zoneName);} );

					if(!zoneRegions) // and if not found - create
					{
						zoneRegions_t zr;
						zr.zoneName = zone.zoneName;

						int zrIdx = zoneRegionList.append( zr );
						zoneRegions = &zoneRegionList[zrIdx];
					}

					// add region index to this zone
					zoneRegions->regionList.append( idx );
				}
			}
			else
			{
				regionOffsetArray[idx] = -1;
				roadOffsetArray[idx] = -1;
				occluderLstOffsetArray[idx] = -1;
			}
		}
	}

	// LEVLUMP_REGIONMAPINFO
	{
		CMemoryStream regInfoLump;
		regInfoLump.Open(NULL, VS_OPEN_WRITE, 2048);

		// write regions header
		regInfoLump.Write(&regMapInfoHdr, 1, sizeof(levRegionMapInfo_t));

		// write region offset array
		regInfoLump.Write(regionOffsetArray, m_wide*m_tall, sizeof(int));

		// compute lump size
		levLump_t regInfoLumpInfo;
		regInfoLumpInfo.type = LEVLUMP_REGIONMAPINFO;
		regInfoLumpInfo.size = regInfoLump.Tell();

		// write lump header and data
		file->Write(&regInfoLumpInfo, 1, sizeof(levLump_t));
		file->Write(regInfoLump.GetBasePointer(), 1, regInfoLump.Tell());

		regInfoLump.Close();
	}

	// LEVLUMP_HEIGHTFIELDS
	{
		WriteHeightfieldsLump( file );
	}

	// LEVLUMP_ROADS
	{
		CMemoryStream roadsLump;
		roadsLump.Open(NULL, VS_OPEN_WRITE, 2048);

		// write road offset array
		roadsLump.Write(roadOffsetArray, m_wide*m_tall, sizeof(int));

		// write road data
		roadsLump.Write(roadDataStream.GetBasePointer(), 1, roadDataStream.Tell());

		// compute lump size
		levLump_t roadDataLumpInfo;
		roadDataLumpInfo.type = LEVLUMP_ROADS;
		roadDataLumpInfo.size = roadsLump.Tell();

		// write lump header and data
		file->Write(&roadDataLumpInfo, 1, sizeof(levLump_t));
		file->Write(roadsLump.GetBasePointer(), 1, roadsLump.Tell());

		file->Flush();
		roadsLump.Close();
	}

	// LEVLUMP_OCCLUDERS
	{
		CMemoryStream occludersLump;
		occludersLump.Open(NULL, VS_OPEN_WRITE, 2048);

		occludersLump.Write(occluderLstOffsetArray, m_wide*m_tall, sizeof(int));
		occludersLump.Write(occluderDataStream.GetBasePointer(), 1, occluderDataStream.Tell());

		levLump_t occludersLumpInfo;
		occludersLumpInfo.type = LEVLUMP_OCCLUDERS;
		occludersLumpInfo.size = occludersLump.Tell();

		// write lump header and data
		file->Write(&occludersLumpInfo, 1, sizeof(levLump_t));

		// write occluder offset array
		file->Write(occludersLump.GetBasePointer(), 1, occludersLump.Tell());

		file->Flush();
		occludersLump.Close();
	}

	// LEVLUMP_REGIONS
	{
		levLump_t regDataLumpInfo;
		regDataLumpInfo.type = LEVLUMP_REGIONS;
		regDataLumpInfo.size = regionDataStream.Tell();

		// write lump header and data
		file->Write(&regDataLumpInfo, 1, sizeof(levLump_t));
		file->Write(regionDataStream.GetBasePointer(), 1, regionDataStream.Tell());

		file->Flush();
		regionDataStream.Close();
	}

	// LEVLUMP_ZONES
	{
		CMemoryStream zonesRegionsLump;
		zonesRegionsLump.Open(NULL, VS_OPEN_WRITE, 2048);

		int zoneCount = zoneRegionList.numElem();
		zonesRegionsLump.Write(&zoneCount, 1, sizeof(int));

		levZoneRegions_t zoneHdr;

		// process all zones
		for(int i = 0; i < zoneRegionList.numElem(); i++)
		{
			memset(&zoneHdr, 0, sizeof(zoneHdr));
			strncpy(zoneHdr.name, zoneRegionList[i].zoneName.c_str(), sizeof(zoneHdr.name) );

			zoneHdr.numRegions = zoneRegionList[i].regionList.numElem();

			// write header
			zonesRegionsLump.Write(&zoneHdr, 1, sizeof(zoneHdr));

			// write region indexes
			zonesRegionsLump.Write(zoneRegionList[i].regionList.ptr(), 1, sizeof(int)*zoneHdr.numRegions);
		}

		levLump_t zoneLumpInfo;
		zoneLumpInfo.type = LEVLUMP_ZONES;
		zoneLumpInfo.size = zonesRegionsLump.Tell();

		// write lump header and data
		file->Write(&zoneLumpInfo, 1, sizeof(levLump_t));
		file->Write(zonesRegionsLump.GetBasePointer(), 1, zonesRegionsLump.Tell());

		file->Flush();
		zonesRegionsLump.Close();
	}

	delete [] regionOffsetArray;
	delete [] roadOffsetArray;
	delete [] occluderLstOffsetArray;

	//-------------------------------------------------------------------------------
}

void CEditorLevel::Ed_InitPhysics()
{
	if (!IsRunning())
		StartWorkerThread("LevelPhysicsInitializerThread");

	/*
	// build region offsets
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			CEditorLevelRegion* reg = (CEditorLevelRegion*)&m_regions[idx];

			reg->Ed_InitPhysics();
		}
	}
	*/
}

void CEditorLevel::Ed_DestroyPhysics()
{
	StopThread();

	// build region offsets
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			CEditorLevelRegion* reg = (CEditorLevelRegion*)&m_regions[idx];

			reg->Ed_DestroyPhysics();
		}
	}
}

void CEditorLevel::SaveEditorBuildings( const char* levelName )
{
	EqString path = varargs("levels/%s_editor/buildings.ekv", levelName);

	KeyValues kvs;
	kvkeybase_t* root = kvs.GetRootSection();

	// build region offsets
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			CEditorLevelRegion* reg = (CEditorLevelRegion*)&m_regions[idx];

			for(int i = 0; i < reg->m_buildings.numElem(); i++)
			{
				buildingSource_t* buildingSrc = reg->m_buildings[i];
				kvkeybase_t* bldKvs = root->AddKeyBase("", varargs("%d", i));

				bldKvs->SetKey("region", varargs("%d", idx));
				buildingSrc->ToKeyValues(bldKvs);
			}
		}
	}

	kvs.SaveToFile( path.c_str(), SP_MOD );
}

typedef std::pair<CEditorLevelRegion*, regionObject_t*> regionObjectPair_t;

void CEditorLevel::LoadEditorBuildings( const char* levelName )
{
	EqString path = varargs("levels/%s_editor/buildings.ekv", levelName);

	KeyValues kvs;
	if(!kvs.LoadFromFile(path.c_str(), SP_MOD))
		return;

	kvkeybase_t* root = kvs.GetRootSection();

	for(int i = 0; i < root->keys.numElem(); i++)
	{
		kvkeybase_t* bldKvs = root->keys[i];

		buildingSource_t* newBld = new buildingSource_t();
		newBld->FromKeyValues(bldKvs);

		int regIndex = KV_GetValueInt(bldKvs->FindKeyBase("region"));
		CEditorLevelRegion* reg = (CEditorLevelRegion*)&m_regions[regIndex];

		reg->m_buildings.append( newBld );
	}
}

void CEditorLevel::PostLoadEditorBuildings( DkList<buildLayerColl_t*>& buildingTemplates )
{
	//
	// Editor buildings must be restored here after loading building templates
	//

	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			CEditorLevelRegion& reg = *(CEditorLevelRegion*)&m_regions[idx];

			// generate models for every building
			for(int i = 0; i < reg.m_buildings.numElem(); i++)
			{
				EqString& name = reg.m_buildings[i]->loadBuildingName;

				buildLayerColl_t** layerColl = buildingTemplates.findFirst([name](buildLayerColl_t* c){ return !c->name.CompareCaseIns(name);} );

				if( layerColl )
				{
					reg.m_buildings[i]->layerColl = *layerColl;
					GenerateBuildingModel(reg.m_buildings[i]); // or it will waste draw calls
				}
				else
					MsgError("**ERROR** unknown building name '%s'!\n", name.c_str());
			}
		}
	}
}

void CEditorLevel::OnPreApplyMaterial( IMaterial* pMaterial )
{
	if( pMaterial->GetState() != MATERIAL_LOAD_OK )
		return;

	g_pShaderAPI->SetShaderConstantVector4D("SunColor", 0.25f);
	g_pShaderAPI->SetShaderConstantVector3D("SunDir", Vector3D(0.0f,1.0f,0.0f));

	g_pShaderAPI->SetShaderConstantFloat("GameTime", 1.0f);

	Vector2D envParams(0.0f);
	g_pShaderAPI->SetShaderConstantVector2D("ENVPARAMS", envParams);

	materials->SetDepthStates(true,false);
	materials->SetShaderParameterOverriden(SHADERPARAM_DEPTHSETUP, true);
}

void CEditorLevel::Ed_Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj)
{
	materials->SetMaterialRenderParamCallback(this);

	occludingFrustum_t bypassFrustum;
	bypassFrustum.bypass = true;

	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			if(m_regions[idx].m_isLoaded)
			{
				m_regions[idx].Render(cameraPosition,viewProj, bypassFrustum, 0);
				m_regions[idx].m_render = true;
			}
		}
	}

	materials->SetMaterialRenderParamCallback(nullptr);
}

CEditorLevel* CEditorLevel::CreatePrefab(const IVector2D& minCell, const IVector2D& maxCell, int flags)
{
	IVector2D prefabSize(maxCell-minCell + 1);

	const float halfTile = HFIELD_POINT_SIZE*0.5f;

	// make level with single region
	CEditorLevel* newLevel = new CEditorLevel();
	newLevel->Init(1,1, max(prefabSize.x, prefabSize.y), true);

	// make bounds for copying objects
	BoundingBox prefabBounds;
	prefabBounds.AddVertex( GlobalTilePointToPosition(minCell) - halfTile );
	prefabBounds.AddVertex( GlobalTilePointToPosition(maxCell) + halfTile );

	// region bounds
	IVector2D regionsMin, regionsMax;
	regionsMin = GlobalPointToRegionPosition(minCell);
	regionsMax = GlobalPointToRegionPosition(maxCell);

	Msg("---------------------\n");
	Msg("Prefab cells size: [%d %d], using biggest one\n", prefabSize.x, prefabSize.y);
	Msg("Generating prefab [%d %d] to [%d %d]\n", minCell.x,minCell.y, maxCell.x, maxCell.y);
	Msg("	prefab regions [%d %d] to [%d %d]\n", regionsMin.x, regionsMin.y, regionsMax.x, regionsMax.y);

	//
	// process work zones by regions
	//
	for(int x = regionsMin.x; x <= regionsMax.x; x++)
	{
		for(int y = regionsMin.y; y <= regionsMax.y; y++)
		{
			int regionIdx = y*m_wide+x;

			IVector2D regionStart = IVector2D(x,y)*m_cellsSize;
			IVector2D regionEnd = regionStart + m_cellsSize;

			// calculate the tile bounds
			IVector2D regionMinCell;
			IVector2D regionMaxCell;

			Msg("Cloning region [%d %d] (idx=%d)\n", x,y, regionIdx);

			regionMinCell = clamp(minCell, regionStart, regionEnd)-regionStart;
			regionMaxCell = clamp(maxCell, regionStart, regionEnd)-regionStart;

			regionMaxCell += 1;
			regionMaxCell = clamp(regionMaxCell, 0, m_cellsSize);

			IVector2D regionSelSize(regionMaxCell-regionMinCell);
			Msg("    cloning bounds: [%d %d] [%d %d] size=[%d %d]\n", regionMinCell.x, regionMinCell.y, regionMaxCell.x, regionMaxCell.y, regionSelSize.x, regionSelSize.y);

			if(flags & PREFAB_HEIGHTFIELDS)
				PrefabHeightfields(newLevel, regionStart-minCell, regionIdx, regionMinCell, regionMaxCell, flags & PREFAB_ROADS);

			if(flags & PREFAB_OBJECTS)
				PrefabObjects(newLevel, regionStart-minCell, regionIdx, prefabBounds);

			if(flags & PREFAB_OCCLUDERS)
				PrefabOccluders(newLevel, regionStart-minCell, regionIdx, prefabBounds);
		}
	}

	return newLevel;
}

void CEditorLevel::PrefabHeightfields(CEditorLevel* destLevel, const IVector2D& prefabOffset, int regionIdx, const IVector2D& regionMinCell, const IVector2D& regionMaxCell, bool cloneRoads)
{
	CEditorLevelRegion& srcRegion = m_regions[regionIdx];

	CLevelRegion& destRegion = destLevel->m_regions[0];

	for(int i = 0; i < ENGINE_REGION_MAX_HFIELDS; i++)
	{
		CHeightTileField* srcField = srcRegion.GetHField(i);
		CHeightTileFieldRenderable* destField = destRegion.GetHField(i);

		bool hasTiles = false;

		// copy heightfield cells
		for(int x = regionMinCell.x; x < regionMaxCell.x; x++)
		{
			for(int y = regionMinCell.y; y < regionMaxCell.y; y++)
			{
				IVector2D srcTileOfs(IVector2D(x,y));
				IVector2D destTileOfs(IVector2D(x,y)+prefabOffset);

				hfieldtile_t* srcTile = srcField->GetTile(srcTileOfs.x, srcTileOfs.y);

				if(!srcTile)
					continue;

				hfieldtile_t* destTile = destField->GetTile(destTileOfs.x, destTileOfs.y);

				//*destTile = *srcTile;
				destTile->height = srcTile->height;
				destTile->flags = srcTile->flags;
				destTile->rotatetex = srcTile->rotatetex;
				destTile->transition = srcTile->transition;

				if(srcTile->texture != -1)
				{
					IMaterial* material = srcField->m_materials[srcTile->texture]->material;

					destField->SetPointMaterial(destTileOfs.x, destTileOfs.y, material, srcTile->atlasIdx);
					hasTiles = true;
					//Msg("Set tile %d %d (%s)\n", x,y, material->GetName());
				}
			}
		}

		if(hasTiles)
		{
			destField->SetChanged();
			destField->GenereateRenderData();
		}
	}

	if(cloneRoads)
	{
		// copy road cells
		for(int x = regionMinCell.x; x < regionMaxCell.x; x++)
		{
			for(int y = regionMinCell.y; y < regionMaxCell.y; y++)
			{
				int srcTileIdx = y * m_cellsSize + x;
	
				IVector2D destTileOfs(IVector2D(x,y)+prefabOffset);

				int destTileIdx = destTileOfs.y * destLevel->m_cellsSize + destTileOfs.x;

				destRegion.m_roads[destTileIdx] = srcRegion.m_roads[srcTileIdx];
			}
		}
	}
}

void CEditorLevel::PrefabObjects(CEditorLevel* destLevel, const IVector2D& prefabOffset, int regionIdx, const BoundingBox& prefabBounds)
{
	CEditorLevelRegion* region = &m_regions[regionIdx];
}

void CEditorLevel::PrefabOccluders(CEditorLevel* destLevel, const IVector2D& prefabOffset, int regionIdx, const BoundingBox& prefabBounds)
{

}

void CEditorLevel::PrefabRoads(CEditorLevel* destLevel, const IVector2D& prefabOffset, int regionIdx, const IVector2D& regionMinCell, const IVector2D& regionMaxCell)
{
	CEditorLevelRegion* region = &m_regions[regionIdx];
}

//-----------------------------------------------------------------------------

void CEditorLevel::PlacePrefab(const IVector2D& globalTile, int height, int rotation, CEditorLevel* prefab, int flags /*EPrefabCreationFlags*/)
{
	IVector2D prefabSize(prefab->m_cellsSize);
	IVector2D prefabCenterTile(prefabSize/2);

	IRectangle tileBounds;
	tileBounds.AddVertex(globalTile-prefabCenterTile);
	tileBounds.AddVertex(globalTile+prefabCenterTile);

	// region bounds
	IVector2D regionsMin, regionsMax;
	regionsMin = GlobalPointToRegionPosition(tileBounds.vleftTop);
	regionsMax = GlobalPointToRegionPosition(tileBounds.vrightBottom);

	regionsMin.x = clamp(0,regionsMin.x,m_wide-1);
	regionsMin.y = clamp(0,regionsMin.y,m_tall-1);

	regionsMax.x = clamp(0,regionsMax.x,m_wide-1);
	regionsMax.y = clamp(0,regionsMax.y,m_tall-1);

	Msg("---------------------\n");
	Msg("Prefab cells size: [%d %d], using biggest one\n", prefabSize.x, prefabSize.y);
	Msg("Placing prefab [%d %d] to [%d %d]\n", tileBounds.vleftTop.x,tileBounds.vleftTop.y, tileBounds.vrightBottom.x, tileBounds.vrightBottom.y);
	Msg("	prefab regions [%d %d] to [%d %d]\n", regionsMin.x, regionsMin.y, regionsMax.x, regionsMax.y);

	/*
	int prefabCellsSize = prefab->m_cellsSize;
	short lowestHeight = 32760;
	
	for(int i = 0; i < ENGINE_REGION_MAX_HFIELDS; i++)
	{
		CHeightTileField* field = prefab->m_regions[0].GetHField(i);
		// get prefab lowest height level
		for(int x = 0; x < prefabCellsSize; x++)
		{
			for(int y = 0; y < prefabCellsSize; y++)
			{
				hfieldtile_t* tile = field->GetTile(x, y);

				if(tile->height < lowestHeight)
					lowestHeight = tile->height;
			}
		}
	}*/

	//
	// placing prefab to regions
	//
	for(int x = regionsMin.x; x <= regionsMax.x; x++)
	{
		for(int y = regionsMin.y; y <= regionsMax.y; y++)
		{
			int regionIdx = y*m_wide+x;

			IVector2D regionStart = IVector2D(x,y)*m_cellsSize;

			IVector2D localPosition = globalTile-regionStart;

			Msg("	prefab cell start on region %d [%d %d]\n", regionIdx, localPosition.x, localPosition.y);

			PlacePrefabHeightfields(localPosition, height, rotation, prefab, regionIdx);
		}
	}

	g_pEditorActionObserver->EndModify();
}

IVector2D RotatePoint(const IVector2D& point, int rotation)
{
	IVector2D result = point;

	while(rotation-- > 0)
		result = IVector2D(result.y, -result.x);

	return result;
}

void CEditorLevel::PlacePrefabHeightfields(const IVector2D& position, int height, int rotation, CEditorLevel* prefab, int regionIdx)
{
	CLevelRegion& srcRegion = prefab->m_regions[0];
	CEditorLevelRegion& destRegion = m_regions[regionIdx];

	int prefabCellsSize = prefab->m_cellsSize;

	IVector2D prefabCenter(prefabCellsSize/2);

	int centerTileHeight = srcRegion.GetHField(0)->GetTile(prefabCenter.x,prefabCenter.y)->height;

	height -= centerTileHeight;

	for(int i = 0; i < ENGINE_REGION_MAX_HFIELDS; i++)
	{
		CHeightTileField* srcField = srcRegion.GetHField(i);
		CHeightTileFieldRenderable* destField = destRegion.GetHField(i);

		float rotationDeg = -rotation*90.0f;
		Matrix2x2 rotateMatrix = rotate2(DEG2RAD(rotationDeg));

		// copy heightfield cells
		for(int x = 0; x < prefabCellsSize; x++)
		{
			for(int y = 0; y < prefabCellsSize; y++)
			{
				IVector2D srcTileOfs(IVector2D(x,y));
				IVector2D destTileOfs(RotatePoint(IVector2D(x,y)-prefabCenter, rotation)+position);

				hfieldtile_t* srcTile = srcField->GetTile(srcTileOfs.x, srcTileOfs.y);
				hfieldtile_t* destTile = destField->GetTile(destTileOfs.x, destTileOfs.y);

				if(!destTile)
					continue;

				// don't set any propery of tile if we has no texture
				if(srcTile->texture != -1)
				{
					destField->SetChanged();
					g_pEditorActionObserver->BeginModify(destField);

					//*destTile = *srcTile;
					destTile->height = srcTile->height + height;
					destTile->flags = srcTile->flags;
					destTile->transition = srcTile->transition;

					destTile->rotatetex = srcTile->rotatetex+rotation;

					if(destTile->rotatetex > 3)
						destTile->rotatetex -= 4;

					IMaterial* material = srcField->m_materials[srcTile->texture]->material;

					destField->SetPointMaterial(destTileOfs.x, destTileOfs.y, material, srcTile->atlasIdx);
				}
			}
		}
	}

	//if(cloneRoads)
	{
		// copy road cells
		for(int x = 0; x < prefabCellsSize; x++)
		{
			for(int y = 0; y < prefabCellsSize; y++)
			{
				int srcTileIdx = y * prefabCellsSize + x;
	
				IVector2D destTileOfs(RotatePoint(IVector2D(x,y)-prefabCenter, rotation)+position);

				if(!(destTileOfs.x >= 0 && destTileOfs.x < m_cellsSize) || !(destTileOfs.y >= 0 && destTileOfs.y < m_cellsSize))
					continue;

				int destTileIdx = destTileOfs.y * m_cellsSize + destTileOfs.x;

				// don't set empty roads
				if(srcRegion.m_roads[srcTileIdx].type == ROADTYPE_NOROAD)
					continue;

				destRegion.m_roads[destTileIdx] = srcRegion.m_roads[srcTileIdx];

				int newDirection = srcRegion.m_roads[srcTileIdx].direction + rotation;

				if(newDirection > 3)
					newDirection -= 4;

				destRegion.m_roads[destTileIdx].direction = newDirection;
			}
		}
	}
}

int CEditorLevel::Ed_SelectRefAndReg(const Vector3D& start, const Vector3D& dir, CEditorLevelRegion** reg, float& dist)
{
	float max_dist = DrvSynUnits::MaxCoordInUnits;
	int bestDistrefIdx = NULL;
	CEditorLevelRegion* bestReg = NULL;

	// build region offsets
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			CEditorLevelRegion& sreg = *(CEditorLevelRegion*)&m_regions[idx];

			float refdist = DrvSynUnits::MaxCoordInUnits;
			int foundIdx = sreg.Ed_SelectRef(start, dir, refdist);

			if(foundIdx != -1 && (refdist < max_dist))
			{
				max_dist = refdist;
				bestReg = &sreg;
				bestDistrefIdx = foundIdx;
			}
		}
	}

	*reg = bestReg;
	dist = max_dist;
	return bestDistrefIdx;
}

int	CEditorLevel::Ed_SelectBuildingAndReg(const Vector3D& start, const Vector3D& dir, CEditorLevelRegion** reg, float& dist)
{
	float max_dist = DrvSynUnits::MaxCoordInUnits;
	int bestDistrefIdx = NULL;
	CEditorLevelRegion* bestReg = NULL;

	// build region offsets
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			CEditorLevelRegion& sreg = *(CEditorLevelRegion*)&m_regions[idx];

			float refdist = DrvSynUnits::MaxCoordInUnits;
			int foundIdx = sreg.Ed_SelectBuilding(start, dir, refdist);

			if(foundIdx != -1 && (refdist < max_dist))
			{
				max_dist = refdist;
				bestReg = &sreg;
				bestDistrefIdx = foundIdx;
			}
		}
	}

	*reg = bestReg;
	dist = max_dist;
	return bestDistrefIdx;
}

void CEditorLevel::Ed_Prerender(const Vector3D& cameraPosition)
{
	Volume& frustum = g_pGameWorld->m_frustum;
	IVector2D camPosReg;

	// mark renderable regions
	if( PositionToRegionOffset(cameraPosition, camPosReg) )
	{
		CEditorLevelRegion* region = (CEditorLevelRegion*)GetRegionAt(camPosReg);

		// query this region
		if(region)
		{
			region->m_render = frustum.IsBoxInside(region->m_bbox.minPoint, region->m_bbox.maxPoint);

			if(region->m_render)
			{
				region->Ed_Prerender();
				region->m_render = false;
			}
		}

		int dx[8] = NEIGHBOR_OFFS_XDX(camPosReg.x, 1);
		int dy[8] = NEIGHBOR_OFFS_YDY(camPosReg.y, 1);

		// surrounding regions frustum
		for(int i = 0; i < 8; i++)
		{
			CEditorLevelRegion* nregion = (CEditorLevelRegion*)GetRegionAt(IVector2D(dx[i], dy[i]));

			if(nregion)
			{
				if(frustum.IsBoxInside(nregion->m_bbox.minPoint, nregion->m_bbox.maxPoint))
				{
					nregion->Ed_Prerender();
					nregion->m_render = false;
				}
			}
		}

		//SignalWork();
	}
}

void CEditorLevelRegion::Cleanup()
{
	Ed_DestroyPhysics();

	CLevelRegion::Cleanup();

	// Clear editor data
	for(int i = 0; i < m_buildings.numElem(); i++)
		delete m_buildings[i];

	m_buildings.clear();
}

CEditorLevelRegion::CEditorLevelRegion() : CLevelRegion(), m_physicsPreview(false)
{
}

void CEditorLevelRegion::Ed_InitPhysics()
{
	if (m_physicsPreview)
		return;

	m_physicsPreview = true;

	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i])
			g_pPhysics->AddHeightField( m_heightfield[i] );
	}

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		regionObject_t* ref = m_objects[i];

		if(ref->def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			// create collision objects and translate them
			CLevelModel* model = ref->def->m_model;

			model->CreateCollisionObject( ref );

			// add physics objects
			g_pPhysics->m_physics.AddStaticObject( ref->physObject );
		}
	}

	Msg("Generated physics for region %d\n", m_regionIndex);
}

void CEditorLevelRegion::Ed_DestroyPhysics()
{
	if (!m_physicsPreview)
		return;

	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i])
			g_pPhysics->RemoveHeightField( m_heightfield[i] );
	}

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		regionObject_t* ref = m_objects[i];

		if(ref->def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			// create collision objects and translate them
			CLevelModel* model = ref->def->m_model;

			g_pPhysics->m_physics.DestroyStaticObject( ref->physObject );
			ref->physObject = NULL;
		}
	}

	m_physicsPreview = false;
}

int FindObjectContainer(DkList<CLevObjectDef*>& listObjects, CLevObjectDef* container)
{
	for(int i = 0; i < listObjects.numElem(); i++)
	{
		if( listObjects[i] == container )
			return i;
	}

	ASSERTMSG(false, "Programmer error, cannot find object definition (is editor cached it?)");

	return -1;
}

void CEditorLevelRegion::WriteRegionData( IVirtualStream* stream, DkList<CLevObjectDef*>& listObjects, bool final )
{
	// create region model lists
	DkList<CLevObjectDef*>		regionDefs;
	DkList<levCellObject_t>		cellObjectsList;

	DkList<regionObject_t*>		regObjects;
	regObjects.append(m_objects);

	DkList<regionObject_t*>		tempObjects;

	//
	// SAVE generated buildings as cell objects
	//
	for(int i = 0; i < m_buildings.numElem(); i++)
	{
		CLevelModel* model = m_buildings[i]->model;

		CLevObjectDef* def = new CLevObjectDef();

		def->m_info.type = LOBJ_TYPE_INTERNAL_STATIC;
		def->m_info.modelflags = LMODEL_FLAG_UNIQUE | LMODEL_FLAG_GENERATED;
		def->m_info.level = 0;

		def->m_model = model;
		def->m_model->Ref_Grab();
		def->m_model->Ref_Grab();

		regionDefs.append(def);

		regionObject_t* tempObject = new regionObject_t();
		
		tempObject->def = def;
		tempObject->tile_x = 0xFFFF;
		tempObject->tile_y = 0xFFFF;
		tempObject->position = m_buildings[i]->modelPosition;
		tempObject->rotation = vec3_zero;

		tempObjects.append(tempObject);

		// make object id to save buildings
		m_buildings[i]->regObjectId = regObjects.append(tempObject);
	}

	cellObjectsList.resize(regObjects.numElem());

	// before we do PostprocessCellObject, make sure we remove all traffic light flags from straights
	for(int x = 0; x < m_heightfield[0]->m_sizew; x++)
	{
		for(int y = 0; y < m_heightfield[0]->m_sizeh; y++)
		{
			int idx = y*m_heightfield[0]->m_sizew + x;

			if(m_roads[idx].type == ROADTYPE_STRAIGHT)
				continue;

			m_roads[idx].flags &= ~ROAD_FLAG_TRAFFICLIGHT;
		}
	}

	//
	// collect models and cell objects
	//
	for(int i = 0; i < regObjects.numElem(); i++)
	{
		regionObject_t* robj = regObjects[i];
		CLevObjectDef* def = robj->def;

		// post process cell objects
		PostprocessCellObject(robj);

		levCellObject_t object;

		// copy name
		memset(object.name, 0, LEV_OBJECT_NAME_LENGTH);
		strncpy(object.name,robj->name.c_str(), LEV_OBJECT_NAME_LENGTH);

		object.tile_x = robj->tile_x;
		object.tile_y = robj->tile_y;
		object.position = robj->position;
		object.rotation = robj->rotation;
		object.flags = 0;

		if(	def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC && (def->m_info.modelflags & LMODEL_FLAG_UNIQUE))
		{
			int isGenerated = (def->m_info.modelflags & LMODEL_FLAG_GENERATED) ? CELLOBJ_GENERATED : 0;

			object.objectDefId = FindObjectContainer(regionDefs, def);
			object.flags = CELLOBJ_REGION_DEF | isGenerated;
		}
		else
			object.objectDefId = FindObjectContainer(listObjects, def); // uses shared object list

		// add
		cellObjectsList.append(object);
	}

	CMemoryStream modelData;
	CMemoryStream regionData;

	regionData.Open(NULL, VS_OPEN_WRITE, 8192);

	//
	// write REGION object definitions
	//
	for(int i = 0; i < regionDefs.numElem(); i++)
	{
		CLevObjectDef* def = regionDefs[i];

		modelData.Open(NULL, VS_OPEN_WRITE, 8192);

		// write model
		def->m_model->Save( &modelData );

		// save def info
		levObjectDefInfo_t& defInfo = def->m_info;
		defInfo.size = modelData.Tell();

		// now write in to region
		regionData.Write(&defInfo, 1, sizeof(levObjectDefInfo_t));
		regionData.Write(modelData.GetBasePointer(), 1, modelData.Tell());
	}

	//
	// write whole cell objects list
	//
	for(int i = 0; i < cellObjectsList.numElem(); i++)
	{
		regionData.Write(&cellObjectsList[i], 1, sizeof(levCellObject_t));
	}

	levRegionDataInfo_t regdatahdr;
	regdatahdr.numCellObjects = cellObjectsList.numElem();
	regdatahdr.numObjectDefs = regionDefs.numElem();
	regdatahdr.size = regionData.Tell();

	stream->Write(&regdatahdr, 1, sizeof(levRegionDataInfo_t));
	stream->Write(regionData.GetBasePointer(), 1, regionData.Tell());

	//
	// aftersave cleanup of generated data
	//
	for(int i = 0; i < tempObjects.numElem(); i++)
	{
		delete tempObjects[i]->def;
		delete tempObjects[i];
	}
}

void CEditorLevelRegion::PostprocessCellObject(regionObject_t* obj)
{
	if(!obj->def)
		return;

	if(obj->def->m_info.type == LOBJ_TYPE_OBJECT_CFG)
	{
		// process traffic lights
		if(!obj->def->m_defType.CompareCaseIns("trafficlight"))
		{
			int trafficDir = GetDirectionIndexByAngles( obj->rotation );
			int laneRowDir = GetPerpendicularDir(trafficDir);

			int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
			int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

			IVector2D forwardDir = IVector2D(dx[trafficDir],dy[trafficDir]);
			IVector2D rightDir = IVector2D(dx[laneRowDir],dy[laneRowDir]);

			IVector2D roadPos;
			if(m_level->Road_FindBestCellForTrafficLight(roadPos, obj->position, trafficDir, 24))
			{
				// move to first lane
				int laneIndex = m_level->Road_GetLaneIndexAtPoint( roadPos )-1;
				roadPos += rightDir*laneIndex;

				int roadWidth = m_level->Road_GetNumLanesAtPoint( roadPos );

				#define REPEAT_ITERATIONS 16

				for(int i = 0; i < roadWidth; i++)
				{
					IVector2D lanePos = roadPos-rightDir*i;

					Vector3D bestCellPos = g_pGameWorld->m_level.GlobalTilePointToPosition(lanePos);
					debugoverlay->Box3D(bestCellPos-2,bestCellPos+2, ColorRGBA(0,1,1,1), 25.0f);

					for(int j = 0; j < REPEAT_ITERATIONS; j++)
					{
						levroadcell_t* rcell = m_level->Road_GetGlobalTileAt(lanePos - forwardDir*j );
						if(rcell)
						{
							if(rcell->type != ROADTYPE_STRAIGHT)
								break;
							rcell->flags |= ROAD_FLAG_TRAFFICLIGHT;
						}
					}
				}
			}
		}
	}
}

void CEditorLevelRegion::WriteRegionRoads( IVirtualStream* stream )
{
	CMemoryStream cells;
	cells.Open(NULL, VS_OPEN_WRITE, 2048);

	int numRoadCells = 0;

	for(int x = 0; x < m_heightfield[0]->m_sizew; x++)
	{
		for(int y = 0; y < m_heightfield[0]->m_sizeh; y++)
		{
			int idx = y*m_heightfield[0]->m_sizew + x;

			if(m_roads[idx].type == ROADTYPE_NOROAD && m_roads[idx].flags == 0)
				continue;

			m_roads[idx].posX = x;
			m_roads[idx].posY = y;

			cells.Write(&m_roads[idx], 1, sizeof(levroadcell_t));
			numRoadCells++;
		}
	}

	stream->Write(&numRoadCells, 1, sizeof(int));
	stream->Write(cells.GetBasePointer(), 1, cells.Tell());
}

void CEditorLevelRegion::WriteRegionOccluders(IVirtualStream* stream)
{
	int numOccluders = m_occluders.numElem();

	stream->Write(&numOccluders, 1, sizeof(int));
	stream->Write(m_occluders.ptr(), 1, sizeof(levOccluderLine_t)*numOccluders);
}

void CEditorLevelRegion::ReadRegionBuildings( IVirtualStream* stream )
{

}

void CEditorLevelRegion::WriteRegionBuildings( IVirtualStream* stream )
{

}

float CheckStudioRayIntersection(IEqModel* pModel, Vector3D& ray_start, Vector3D& ray_dir)
{
	const BoundingBox& bbox = pModel->GetAABB();

	float f1,f2;
	if(!bbox.IntersectsRay(ray_start, ray_dir, f1,f2))
		return DrvSynUnits::MaxCoordInUnits;

	float best_dist = DrvSynUnits::MaxCoordInUnits;
	float fraction = 1.0f;

	studiohdr_t* pHdr = pModel->GetHWData()->studio;
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
			modelgroupdesc_t* pGroup = pHdr->pModelDesc(nModDescId)->pGroup(j);

			uint32 *pIndices = pGroup->pVertexIdx(0);

			int numTriangles = floor((float)pGroup->numIndices / 3.0f);
			int validIndexes = numTriangles * 3;

			for(int k = 0; k < validIndexes; k+=3)
			{
				Vector3D v0,v1,v2;

				v0 = pGroup->pVertex(pIndices[k])->point;
				v1 = pGroup->pVertex(pIndices[k+1])->point;
				v2 = pGroup->pVertex(pIndices[k+2])->point;

				float dist = DrvSynUnits::MaxCoordInUnits+1;

				if(IsRayIntersectsTriangle(v0,v1,v2, ray_start, ray_dir, dist))
				{
					if(dist < best_dist && dist > 0)
					{
						best_dist = dist;
						fraction = dist;

						//outPos = lerp(start, end, dist);
					}
				}
			}
		}
	}

	return fraction;
}

int CEditorLevelRegion::Ed_SelectRef(const Vector3D& start, const Vector3D& dir, float& dist)
{
	int bestDistrefIdx = -1;
	float fMaxDist = DrvSynUnits::MaxCoordInUnits;

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		Matrix4x4 wmatrix = GetModelRefRenderMatrix(this, m_objects[i]);

		Vector3D tray_start = ((!wmatrix)*Vector4D(start, 1)).xyz();
		Vector3D tray_dir = (!wmatrix.getRotationComponent())*dir;

		float raydist = DrvSynUnits::MaxCoordInUnits;

		CLevObjectDef* def = m_objects[i]->def;

		if(def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			raydist = def->m_model->Ed_TraceRayDist(tray_start, tray_dir);
		}
		else
		{
			raydist = CheckStudioRayIntersection(def->m_defModel, tray_start, tray_dir);
		}

		if(raydist < fMaxDist)
		{
			fMaxDist = raydist;
			bestDistrefIdx = i;
		}
	}

	dist = fMaxDist;

	return bestDistrefIdx;
}

int	CEditorLevelRegion::Ed_SelectBuilding(const Vector3D& start, const Vector3D& dir, float& dist)
{
	int bestDistrefIdx = -1;
	float fMaxDist = DrvSynUnits::MaxCoordInUnits;

	for(int i = 0; i < m_buildings.numElem(); i++)
	{
		Vector3D tray_start = start - m_buildings[i]->modelPosition;
		Vector3D tray_dir = dir;

		float raydist = DrvSynUnits::MaxCoordInUnits;

		buildingSource_t* building = m_buildings[i];

		raydist = building->model->Ed_TraceRayDist(tray_start, tray_dir);

		if(raydist < fMaxDist)
		{
			fMaxDist = raydist;
			bestDistrefIdx = i;
		}
	}

	dist = fMaxDist;

	return bestDistrefIdx;
}

int CEditorLevelRegion::Ed_ReplaceDefs(CLevObjectDef* whichReplace, CLevObjectDef* replaceTo)
{
	int numReplaced = 0;

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		if(m_objects[i]->def == whichReplace)
		{
			m_objects[i]->def = replaceTo;
			numReplaced++;
		}
	}

	return numReplaced;
}

void CEditorLevelRegion::Ed_Prerender()
{
	for(int i = 0; i < m_objects.numElem(); i++)
	{
		CLevObjectDef* cont = m_objects[i]->def;

		Matrix4x4 mat = GetModelRefRenderMatrix(this, m_objects[i]);

		DrawDefLightData(mat, cont->m_lightData, 1.0f);
	}
}

ConVar editor_objectnames_distance("ed_objNamesDist", "100.0f", nullptr, CV_ARCHIVE);
ConVar editor_objectnames_onlynamed("ed_objNamesOnlyNamed", "1", nullptr, CV_ARCHIVE);

void CEditorLevelRegion::Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& frustumOccluders, int nRenderFlags)
{
	CLevelRegion::Render(cameraPosition, viewProj, frustumOccluders, nRenderFlags);

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		if(m_objects[i]->def->m_info.type != LOBJ_TYPE_OBJECT_CFG)
			continue;

		if(m_objects[i]->name.Length() > 0)
		{
			debugoverlay->Text3D(m_objects[i]->position, editor_objectnames_distance.GetFloat(), ColorRGBA(1), 0.0f, varargs("%s '%s'", m_objects[i]->def->m_name.c_str(), m_objects[i]->name.c_str()) );
		}
		else if(!editor_objectnames_onlynamed.GetBool())
		{
			debugoverlay->Text3D(m_objects[i]->position, editor_objectnames_distance.GetFloat(), ColorRGBA(1), 0.0f, m_objects[i]->def->m_name.c_str());
		}
	}

	// render completed buildings
	for(int i = 0; i < m_buildings.numElem(); i++)
	{
		if(m_buildings[i]->hide)
			continue;

		RenderBuilding(m_buildings[i], NULL);
	}
}