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

	return (building->model != NULL);
}