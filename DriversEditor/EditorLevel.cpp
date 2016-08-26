//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor level data
//////////////////////////////////////////////////////////////////////////////////

#include "EditorLevel.h"
#include "level.h"
#include "IMaterialSystem.h"
#include "IDebugOverlay.h"
#include "world.h"

#include "VirtualStream.h"

#include "utils/strtools.h"

// layer model header
struct layerModelFileHdr_t
{
	int		layerId;
	char	name[80];
	int		size;
};

buildLayerColl_t::~buildLayerColl_t()
{
	for(int i = 0; i < layers.numElem(); i++)
	{
		if(layers[i].type == BUILDLAYER_TEXTURE)
			materials->FreeMaterial(layers[i].material);
		else
			delete layers[i].model;
	}
}

void buildLayerColl_t::Save(IVirtualStream* stream, kvkeybase_t* kvs)
{
	int numModels = 0;
	layerModelFileHdr_t hdr;

	for(int i = 0; i < layers.numElem(); i++)
	{
		// save both model and texture to kvs info
		kvkeybase_t* layerKvs = kvs->AddKeyBase("layer");
		layerKvs->SetKey("type", varargs("%d", layers[i].type));
		layerKvs->SetKey("size", varargs("%d", layers[i].size));

		if(layers[i].type == BUILDLAYER_TEXTURE && layers[i].material != NULL)
			layerKvs->SetKey("material", layers[i].material->GetName());
		else if(layers[i].type != BUILDLAYER_TEXTURE && layers[i].model != NULL)
			layerKvs->SetKey("model", layers[i].model->m_name.c_str());

		if(layers[i].type == BUILDLAYER_TEXTURE)
			continue;

		numModels++;
	}

	stream->Write(&numModels, 1, sizeof(int));

	CMemoryStream modelStream;

	for(int i = 0; i < layers.numElem(); i++)
	{
		if(layers[i].type == BUILDLAYER_TEXTURE)
			continue;

		// write model to temporary stream
		modelStream.Open(NULL, VS_OPEN_READ | VS_OPEN_WRITE, 2048);
		layers[i].model->m_model->Save( &modelStream );

		// prepare header
		memset(&hdr, 0, sizeof(hdr));
		hdr.layerId = i;
		hdr.size = modelStream.Tell();
		strncpy(hdr.name, layers[i].model->m_name.c_str(), sizeof(hdr.name));

		// write header
		stream->Write(&hdr, 1, sizeof(hdr));

		// write model
		stream->Write(modelStream.GetBasePointer(), 1, hdr.size);

		modelStream.Close();
	}
}

void buildLayerColl_t::Load(IVirtualStream* stream, kvkeybase_t* kvs)
{
	// read layers first
	for(int i = 0; i < kvs->keys.numElem(); i++)
	{
		if(stricmp(kvs->keys[i]->name, "layer"))
			continue;

		kvkeybase_t* layerKvs = kvs->keys[i];

		int newLayer = layers.append(buildLayer_t());
		buildLayer_t& layer = layers[newLayer];

		layer.type = KV_GetValueInt(layerKvs->FindKeyBase("type"));
		layer.size = KV_GetValueInt(layerKvs->FindKeyBase("size"), 0, layer.size);

		if(layer.type == BUILDLAYER_TEXTURE)
		{
			layer.material = materials->FindMaterial(KV_GetValueString(layerKvs->FindKeyBase("material")));
			layer.material->Ref_Grab();
		}
		else
			layer.model = NULL;
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
		layers[hdr.layerId].model = mod;

		mod->m_name = hdr.name;

		// read model
		int modOffset = stream->Tell();
		mod->m_model = new CLevelModel();
		mod->m_model->Load(stream);

		mod->SetDirtyPreview();
		mod->RefreshPreview();

		int modelSize = stream->Tell() - modOffset;

		// a bit paranoid
		ASSERT(layers[hdr.layerId].type == BUILDLAYER_MODEL || layers[hdr.layerId].type == BUILDLAYER_CORNER_MODEL);
		ASSERT(hdr.size == modelSize);
	}
}

//-------------------------------------------------------

buildingSource_t::buildingSource_t(buildingSource_t& copyFrom)
{
	layerColl = copyFrom.layerColl;
	order = copyFrom.order;

	for(DkLLNode<buildSegmentPoint_t>* lln = copyFrom.points.goToFirst(); lln != NULL; lln = copyFrom.points.goToNext())
	{
		points.addLast(lln->object);
	}

	//points.append( copyFrom.points );

	// I do not copy model for re-generation reason
	model = NULL;
	modelPosition = vec3_zero;
}

buildingSource_t::~buildingSource_t()
{
	delete model;
}

void buildingSource_t::ToKeyValues(kvkeybase_t* kvs)
{
	kvs->SetName( layerColl->name.c_str() );
	kvs->SetKey("order", order);
	kvs->SetKey("objectId", regObjectId);
	kvs->SetKey("position", modelPosition);

	kvkeybase_t* kvPoints = kvs->AddKeyBase("points");

	for(DkLLNode<buildSegmentPoint_t>* lln = points.goToFirst(); lln != NULL; lln = points.goToNext())
	{
		kvkeybase_t* kvp = kvPoints->AddKeyBase(varargs("%d", lln->object.layerId));
		kvp->AppendValue(lln->object.position);
		kvp->AppendValue(lln->object.scale);
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

int GetLayerSegmentIterations(const buildSegmentPoint_t& start, const buildSegmentPoint_t& end, buildLayer_t& layer)
{
	float len = GetSegmentLength(layer) * start.scale;

	return GetLayerSegmentIterations(start, end, len);
}

float GetSegmentLength( buildLayer_t& layer )
{
	float sizeX = layer.size;

	if(layer.type == BUILDLAYER_MODEL || layer.type == BUILDLAYER_CORNER_MODEL)
	{
		CLevelModel* model = layer.model->m_model;

		// compute iteration count from model width
		const BoundingBox& modelBox = model->GetAABB();

		Vector3D size = modelBox.GetSize();
		sizeX = size.x;
	}

	return sizeX;
}

//
// Rendering the building
//
void RenderBuilding( buildingSource_t* building, buildSegmentPoint_t* extraSegment )
{
	if(building->model != NULL)
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
		allPoints[allPoints.numElem()-1].layerId = extraSegment->layerId;
		allPoints[allPoints.numElem()-1].scale = extraSegment->scale;
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

		if(layer.type == BUILDLAYER_MODEL || layer.type == BUILDLAYER_CORNER_MODEL)
		{
			CLevelModel* model = layer.model->m_model;

			// compute iteration count from model width
			const BoundingBox& modelBox = model->GetAABB();

			Vector3D size = modelBox.GetSize();
			float modelLen = size.x*start.scale;

			int numIterations = GetLayerSegmentIterations(start, end, modelLen);

			// calculate transformation for each iteration
			for(int iter = 0; iter < numIterations; iter++)
			{
				CLevelModel* model = layer.model->m_model;

				CalculateBuildingSegmentTransform( partTransform, layer, start.position, end.position, building->order, size, start.scale, iter );

				materials->SetMatrix(MATRIXMODE_WORLD, partTransform);
				model->Render(0, tempBBox);
			}
		}
		else
		{
		
		}
	}
}

class CBuildingModelGenerator
{
public:
	void				AppendModel( CLevelModel* model, const Matrix4x4& transform);
	CLevelModel*		GenerateModel();

	const BoundingBox&	GetAABB() const {return m_aabb;}

protected:

	lmodel_batch_t&		GetBatch(IMaterial* material);

	DkList<lmodel_batch_t> m_batches;
	DkList<lmodeldrawvertex_t> m_verts;
	DkList<uint16> m_indices;
	BoundingBox m_aabb;
};

void CBuildingModelGenerator::AppendModel(CLevelModel* model, const Matrix4x4& transform )
{
	Matrix3x3 rotTransform = transform.getRotationComponent();

	for(int i = 0; i < model->m_numBatches; i++)
	{
		lmodel_batch_t* srcBatch = &model->m_batches[i];

		lmodel_batch_t& destBatch = GetBatch( srcBatch->pMaterial );

		for (uint16 j = 0; j < srcBatch->numIndices; j++)
		{
			int srcIdx = model->m_indices[srcBatch->startIndex+ j ];
			lmodeldrawvertex_t destVtx = model->m_verts[srcIdx];

			destVtx.position = (transform*Vector4D(destVtx.position, 1.0f)).xyz();
			destVtx.normal = rotTransform*destVtx.normal;

			int destIdx = m_verts.addUnique(destVtx);

			m_indices.append(destIdx);

			// extend aabb
			m_aabb.AddVertex( destVtx.position );
		}

		destBatch.numIndices = m_indices.numElem() - destBatch.startIndex;
		destBatch.numVerts = m_verts.numElem() - destBatch.startVertex;
	}
}

CLevelModel* CBuildingModelGenerator::GenerateModel()
{
	CLevelModel* model = new CLevelModel();
	model->m_numBatches = m_batches.numElem();
	model->m_batches = new lmodel_batch_t[model->m_numBatches];

	for(int i = 0; i < model->m_numBatches; i++)
	{
		// copy batch
		lmodel_batch_t& destBatch = model->m_batches[i];
		destBatch = m_batches[i];

		// or it would crash
		destBatch.pMaterial->Ref_Grab();
	}

	Vector3D center = m_aabb.GetCenter();

	// correct aabb
	m_aabb.minPoint -= center;
	m_aabb.maxPoint -= center;

	// move verts
	for(int i = 0; i < m_verts.numElem(); i++)
		m_verts[i].position -= center;

	model->m_bbox = m_aabb;

	// copy vbo's
	model->m_numVerts = m_verts.numElem();
	model->m_numIndices = m_indices.numElem();

	model->m_verts = new lmodeldrawvertex_t[model->m_numVerts];
	model->m_indices = new uint16[model->m_numIndices];

	memcpy(model->m_verts, m_verts.ptr(), sizeof(lmodeldrawvertex_t)*model->m_numVerts);
	memcpy(model->m_indices, m_indices.ptr(), sizeof(uint16)*model->m_numIndices);

	model->GenereateRenderData();

	return model;
}

lmodel_batch_t& CBuildingModelGenerator::GetBatch(IMaterial* material)
{
	for(int i = 0; i < m_batches.numElem(); i++)
	{
		if(m_batches[i].pMaterial == material)
			return m_batches[i];
	}

	lmodel_batch_t newBatch;
	newBatch.pMaterial = material;
	newBatch.startIndex = m_indices.numElem();
	newBatch.startVertex = m_verts.numElem();
	newBatch.numIndices = 0;
	newBatch.numVerts = 0;

	int newIdx = m_batches.append(newBatch);

	return m_batches[newIdx];
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

		if(layer.type == BUILDLAYER_MODEL || layer.type == BUILDLAYER_CORNER_MODEL)
		{
			CLevelModel* model = layer.model->m_model;

			// compute iteration count from model width
			const BoundingBox& modelBox = model->GetAABB();

			Vector3D size = modelBox.GetSize();
			float modelLen = size.x*start.scale;

			int numIterations = GetLayerSegmentIterations(start, end, modelLen);

			// calculate transformation for each iteration
			for(int iter = 0; iter < numIterations; iter++)
			{
				CLevelModel* model = layer.model->m_model;

				CalculateBuildingSegmentTransform( partTransform, layer, start.position, end.position, building->order, size, start.scale, iter );

				generator.AppendModel(model, partTransform);
			}
		}
		else
		{
		
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
		MsgError("can't open level '%s' for write\n", levelname);
		return false;
	}

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

void CEditorLevel::WriteObjectDefsLump(IVirtualStream* stream)
{
	// if(m_finalBuild)
	//	return;

	if(m_objectDefs.numElem() == 0)
		return;

	CMemoryStream modelsLump;
	modelsLump.Open(NULL, VS_OPEN_WRITE, 2048);

	int numModels = m_objectDefs.numElem();

	modelsLump.Write(&numModels, 1, sizeof(int));

	// write model names
	CMemoryStream modelNamesData;
	modelNamesData.Open(NULL, VS_OPEN_WRITE, 2048);

	char nullSymbol = '\0';

	for(int i = 0; i < m_objectDefs.numElem(); i++)
	{
		modelNamesData.Print(m_objectDefs[i]->m_name.c_str());
		modelNamesData.Write(&nullSymbol, 1, 1);
	}

	modelNamesData.Write(&nullSymbol, 1, 1);

	int modelNamesSize = modelNamesData.Tell();

	modelsLump.Write(&modelNamesSize, 1, sizeof(int));
	modelsLump.Write(modelNamesData.GetBasePointer(), 1, modelNamesSize);

	// write model data
	for(int i = 0; i < m_objectDefs.numElem(); i++)
	{
		CMemoryStream modeldata;
		modeldata.Open(NULL, VS_OPEN_WRITE, 2048);

		if(m_objectDefs[i]->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			m_objectDefs[i]->m_model->Save( &modeldata );
		}
		else if(m_objectDefs[i]->m_info.type == LOBJ_TYPE_OBJECT_CFG)
		{
			// do nothing
		}

		int modelSize = modeldata.Tell();

		m_objectDefs[i]->m_info.size = modelSize;

		modelsLump.Write(&m_objectDefs[i]->m_info, 1, sizeof(levObjectDefInfo_t));
		modelsLump.Write(modeldata.GetBasePointer(), 1, modelSize);
	}

	// compute lump size
	levLump_t modelLumpInfo;
	modelLumpInfo.type = LEVLUMP_OBJECTDEFS;
	modelLumpInfo.size = modelsLump.Tell();

	// write lump header and data
	stream->Write(&modelLumpInfo, 1, sizeof(levLump_t));
	stream->Write(modelsLump.GetBasePointer(), 1, modelsLump.Tell());
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

						int idx = zoneRegionList.append( zr );
						zoneRegions = &zoneRegionList[idx];
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

	DkList<regionObjectPair_t> deletingObjects;
	deletingObjects.resize( root->keys.numElem() );

	for(int i = 0; i < root->keys.numElem(); i++)
	{
		kvkeybase_t* bldKvs = root->keys[i];

		buildingSource_t* newBld = new buildingSource_t();
		newBld->FromKeyValues(bldKvs);

		int regIndex = KV_GetValueInt(bldKvs->FindKeyBase("region"));
		CEditorLevelRegion* reg = (CEditorLevelRegion*)&m_regions[regIndex];

		reg->m_buildings.append( newBld );
		
		MsgInfo("** removing generated building cell object %d (total=%d)\n", newBld->regObjectId, reg->m_objects.numElem());
		deletingObjects.append( regionObjectPair_t(reg, reg->m_objects[newBld->regObjectId]) );
	}

	// now we can freely remove them
	for(int i = 0; i < deletingObjects.numElem(); i++)
	{
		CEditorLevelRegion* reg = deletingObjects[i].first;
		regionObject_t* ref = deletingObjects[i].second;

		reg->m_objects.fastRemove(ref);
		delete ref;
	}
}

void CEditorLevel::PostLoadEditorBuilding( DkList<buildLayerColl_t*>& buildingTemplates )
{
	// build region offsets
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

int CEditorLevel::Ed_SelectRefAndReg(const Vector3D& start, const Vector3D& dir, CLevelRegion** reg, float& dist)
{
	float max_dist = MAX_COORD_UNITS;
	int bestDistrefIdx = NULL;
	CLevelRegion* bestReg = NULL;

	// build region offsets
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			CEditorLevelRegion& reg = *(CEditorLevelRegion*)&m_regions[idx];

			float refdist = MAX_COORD_UNITS;
			int foundIdx = reg.Ed_SelectRef(start, dir, refdist);

			if(foundIdx != -1 && (refdist < max_dist))
			{
				max_dist = refdist;
				bestReg = &reg;
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
	if( GetPointAt(cameraPosition, camPosReg) )
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
	CLevelRegion::Cleanup();

	// Clear editor data
	for(int i = 0; i < m_buildings.numElem(); i++)
		delete m_buildings[i];

	m_buildings.clear();
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
	DkList<CLevelModel*>		modelList;
	DkList<levCellObject_t>		cellObjectsList;

	DkList<regionObject_t*>		regObjects;
	regObjects.append(m_objects);

	DkList<regionObject_t*>		tempObjects;

	//
	// save buildings also
	//
	for(int i = 0; i < m_buildings.numElem(); i++)
	{
		regionObject_t* tempObject = new regionObject_t();
		tempObjects.append(tempObject);

		tempObject->def = new CLevObjectDef();
		tempObject->def->m_info.type = LOBJ_TYPE_INTERNAL_STATIC;
		tempObject->def->m_info.modelflags = LMODEL_FLAG_UNIQUE;
		tempObject->def->m_model = m_buildings[i]->model;
		tempObject->def->m_model->Ref_Grab();
		tempObject->def->m_model->Ref_Grab();

		tempObject->tile_x = 0xFFFF;
		tempObject->tile_y = 0xFFFF;
		tempObject->position = m_buildings[i]->modelPosition;
		tempObject->rotation = vec3_zero;

		// make object id to save buildings
		m_buildings[i]->regObjectId = regObjects.append(tempObject);
	}

	cellObjectsList.resize(regObjects.numElem());

	// collect models and cell objects
	for(int i = 0; i < regObjects.numElem(); i++)
	{
		regionObject_t* robj = regObjects[i];
		CLevObjectDef* def = robj->def;

		levCellObject_t object;

		if(	def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC && (def->m_info.modelflags & LMODEL_FLAG_UNIQUE))
		{
			object.objectDefId = modelList.addUnique( def->m_model );
			object.uniqueRegionModel = 1;
		}
		else
		{
			object.objectDefId = FindObjectContainer(listObjects, def);
			object.uniqueRegionModel = 0;
		}

		// copy name
		memset(object.name, 0, LEV_OBJECT_NAME_LENGTH);
		strncpy(object.name,robj->name.c_str(), LEV_OBJECT_NAME_LENGTH);

		object.tile_x = robj->tile_x;
		object.tile_y = robj->tile_y;
		object.position = robj->position;
		object.rotation = robj->rotation;

		// add
		cellObjectsList.append(object);
	}

	CMemoryStream regionData;
	regionData.Open(NULL, VS_OPEN_WRITE, 8192);

	// write model data
	for(int i = 0; i < modelList.numElem(); i++)
	{
		CMemoryStream modelData;
		modelData.Open(NULL, VS_OPEN_WRITE, 8192);

		modelList[i]->Save( &modelData );

		int modelSize = modelData.Tell();

		regionData.Write(&modelSize, 1, sizeof(int));
		regionData.Write(modelData.GetBasePointer(), 1, modelData.Tell());
	}

	// write cell objects list
	for(int i = 0; i < cellObjectsList.numElem(); i++)
	{
		regionData.Write(&cellObjectsList[i], 1, sizeof(levCellObject_t));
	}

	levRegionDataInfo_t regdatahdr;
	regdatahdr.numCellObjects = cellObjectsList.numElem();
	regdatahdr.numObjectDefs = modelList.numElem();
	regdatahdr.size = regionData.Tell();

	stream->Write(&regdatahdr, 1, sizeof(levRegionDataInfo_t));
	stream->Write(regionData.GetBasePointer(), 1, regionData.Tell());

	// aftersave cleanup
	for(int i = 0; i < tempObjects.numElem(); i++)
	{
		delete tempObjects[i]->def;
		delete tempObjects[i];
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
		return MAX_COORD_UNITS;

	float best_dist = MAX_COORD_UNITS;
	float fraction = 1.0f;

	studiohdr_t* pHdr = pModel->GetHWData()->pStudioHdr;
	int nLod = 0;

	for(int i = 0; i < pHdr->numbodygroups; i++)
	{
		int nLodableModelIndex = pHdr->pBodyGroups(i)->lodmodel_index;
		int nModDescId = pHdr->pLodModel(nLodableModelIndex)->lodmodels[nLod];

		while(nLod > 0 && nModDescId != -1)
		{
			nLod--;
			nModDescId = pHdr->pLodModel(nLodableModelIndex)->lodmodels[nLod];
		}

		if(nModDescId == -1)
			continue;

		for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numgroups; j++)
		{
			modelgroupdesc_t* pGroup = pHdr->pModelDesc(nModDescId)->pGroup(j);

			uint32 *pIndices = pGroup->pVertexIdx(0);

			int numTriangles = floor((float)pGroup->numindices / 3.0f);
			int validIndexes = numTriangles * 3;

			for(int k = 0; k < validIndexes; k+=3)
			{
				Vector3D v0,v1,v2;

				v0 = pGroup->pVertex(pIndices[k])->point;
				v1 = pGroup->pVertex(pIndices[k+1])->point;
				v2 = pGroup->pVertex(pIndices[k+2])->point;

				float dist = MAX_COORD_UNITS+1;

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
	float fMaxDist = MAX_COORD_UNITS;

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		Matrix4x4 wmatrix = GetModelRefRenderMatrix(this, m_objects[i]);

		Vector3D tray_start = ((!wmatrix)*Vector4D(start, 1)).xyz();
		Vector3D tray_dir = (!wmatrix.getRotationComponent())*dir;

		float raydist = MAX_COORD_UNITS;

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

void CEditorLevelRegion::Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& frustumOccluders, int nRenderFlags)
{
	CLevelRegion::Render(cameraPosition, viewProj, frustumOccluders, nRenderFlags);

	// render completed buildings
	for(int i = 0; i < m_buildings.numElem(); i++)
	{
		RenderBuilding(m_buildings[i], NULL);
	}
}