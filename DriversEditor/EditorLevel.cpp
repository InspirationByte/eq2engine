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

void CalculateBuildingModelTransform(Matrix4x4& partTransform, 
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
	if(building->points.numElem() == 0 || building->layerColl == NULL)
		return;

	// Render dynamic preview of the building we're making
	DkList<buildSegmentPoint_t> allPoints;
	allPoints.append(building->points);

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

				CalculateBuildingModelTransform( partTransform, layer, start.position, end.position, building->order, size, start.scale, iter );

				materials->SetMatrix(MATRIXMODE_WORLD, partTransform);
				model->Render(0, tempBBox);
			}
		}
		else
		{
		
		}
	}
}

//
// Generates new levelmodel of building
// Returns local-positioned model, and it's position in the world
//
bool GenerateBuildingModel( buildingSource_t* building )
{
	if(building->points.numElem() == 0 || building->layerColl == NULL)
		return false;

	return true;
}