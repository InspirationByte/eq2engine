//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor level data
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORLEVEL_H
#define EDITORLEVEL_H

#include "math/Vector.h"
#include "TextureAtlas.h"
#include "EditorPreviewable.h"
#include "utils/DkLinkedList.h"

#include "level_generator.h"
#include "region.h"
#include "Level.h"

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
		hide = false;
	}

	buildingSource_t(buildingSource_t& copyFrom);
	~buildingSource_t();

	void InitFrom(buildingSource_t& copyFrom);

	void ToKeyValues(kvkeybase_t* kvs);
	void FromKeyValues(kvkeybase_t* kvs);

	EqString							loadBuildingName;

	DkLinkedList<buildSegmentPoint_t>	points;
	buildLayerColl_t*					layerColl; 
	int									order;

	CLevelModel*						model;
	Vector3D							modelPosition;
	int									regObjectId;	// region object index

	bool								hide;
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

//-----------------------------------------------------------------------------------

class CEditorLevel : public CGameLevel
{
	friend class CEditorLevelRegion;
public:

	bool	Load(const char* levelname, kvkeybase_t* kvDefs);

	bool	Save(const char* levelname, bool isfinal = false);

	void	Ed_Prerender(const Vector3D& cameraPosition);

	int		Ed_SelectRefAndReg(const Vector3D& start, const Vector3D& dir, CEditorLevelRegion** reg, float& dist);
	int		Ed_SelectBuildingAndReg(const Vector3D& start, const Vector3D& dir, CEditorLevelRegion** reg, float& dist);

	bool	Ed_GenerateMap(LevelGenParams_t& genParams, const CImage* img);

	void	WriteLevelRegions(IVirtualStream* stream, bool isFinal);
	void	WriteObjectDefsLump(IVirtualStream* stream);
	void	WriteHeightfieldsLump(IVirtualStream* stream);

	void	SaveEditorBuildings( const char* levelName );
	void	LoadEditorBuildings( const char* levelName );

	void	PostLoadEditorBuildings( DkList<buildLayerColl_t*>& buildingTemplates );
};

//-----------------------------------------------------------------------------------

class CEditorLevelRegion : public CLevelRegion
{
	friend class CEditorLevel;
public:

	void						Cleanup();

	void						Ed_Prerender();

	void						WriteRegionData(IVirtualStream* stream, DkList<CLevObjectDef*>& models, bool isFinal);
	void						WriteRegionOccluders(IVirtualStream* stream);
	void						WriteRegionRoads(IVirtualStream* stream);
		
	void						ReadRegionBuildings( IVirtualStream* stream );
	void						WriteRegionBuildings( IVirtualStream* stream );

	int							Ed_SelectRef(const Vector3D& start, const Vector3D& dir, float& dist);
	int							Ed_SelectBuilding(const Vector3D& start, const Vector3D& dir, float& dist);

	int							Ed_ReplaceDefs(CLevObjectDef* whichReplace, CLevObjectDef* replaceTo);
	void						Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& frustumOccluders, int nRenderFlags);

	bool						m_modified; // needs saving?

	DkList<buildingSource_t*>	m_buildings;
};

#endif // EDITORLEVEL_H
