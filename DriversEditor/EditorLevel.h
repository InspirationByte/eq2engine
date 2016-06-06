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
	}

	DkList<buildSegmentPoint_t>	points;
	buildLayerColl_t*			layerColl;
	int							order;
};

void CalculateBuildingModelTransform(Matrix4x4& partTransform, 
									buildLayer_t& layer, 
									const Vector3D& startPoint, 
									const Vector3D& endPoint, 
									int order, 
									Vector3D& size, float scale,
									int iteration );

#endif // EDITORLEVEL_H
