//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor level data
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORLEVEL_H
#define EDITORLEVEL_H

#include "math/Vector.h"
#include "TextureAtlas.h"
#include "EditorPreviewable.h"
#include "ppmem.h"
#include "utils/DkList.h"
#include "utils/DkLinkedList.h"

class CLevelModel;
class IMaterial;
class IVirtualStream;
struct kvkeybase_t;

class CLayerModel : public CEditorPreviewable
{
public:
	PPMEM_MANAGED_OBJECT()

	CLayerModel();
	~CLayerModel();

	void			RefreshPreview();

	CLevelModel*	m_model;
	EqString		m_name;
};

enum EBuildLayerType
{
	BUILDLAYER_TEXTURE = 0,
	BUILDLAYER_MODEL,
	BUILDLAYER_CORNER_MODEL
};

struct buildLayer_t
{
	buildLayer_t() 
		: size(16.0f), type(BUILDLAYER_TEXTURE), model(nullptr), material(nullptr)
	{
	}

	float					size;			// size in units
	int						type;			// ELayerType
	CLayerModel*			model;
	IMaterial*				material;
};

struct buildLayerColl_t
{
	~buildLayerColl_t();

	void					Save(IVirtualStream* stream, kvkeybase_t* kvs);
	void					Load(IVirtualStream* stream, kvkeybase_t* kvs);

	EqString				name;
	DkList<buildLayer_t>	layers;
};

struct buildSegmentPoint_t
{
	FVector3D	position;
	int			layerId;
	float		scale;
};

struct buildingSource_t
{
	buildingSource_t()
	{
		layerColl = NULL;
		order = 1;
		model = NULL;
		modelPosition = vec3_zero;
	}

	buildingSource_t(buildingSource_t& copyFrom);

	~buildingSource_t();

	DkLinkedList<buildSegmentPoint_t>	points;
	buildLayerColl_t*					layerColl; 
	int									order;

	CLevelModel*						model;
	Vector3D							modelPosition;
};

int GetLayerSegmentIterations(const buildSegmentPoint_t& start, const buildSegmentPoint_t& end, float layerXSize);
int GetLayerSegmentIterations(const buildSegmentPoint_t& start, const buildSegmentPoint_t& end, buildLayer_t& layer);
float GetSegmentLength(buildLayer_t& layer);

void CalculateBuildingSegmentTransform(	Matrix4x4& partTransform, 
										buildLayer_t& layer, 
										const Vector3D& startPoint, 
										const Vector3D& endPoint, 
										int order, 
										Vector3D& size, float scale,
										int iteration );

// Rendering the building
void RenderBuilding( buildingSource_t* building, buildSegmentPoint_t* extraSegment );

// Generates new levelmodel of building
// Returns local-positioned model, and it's position in the world
bool GenerateBuildingModel( buildingSource_t* building );

#endif // EDITORLEVEL_H
