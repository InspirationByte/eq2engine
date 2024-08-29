//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Studio Geometry Form
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/model.h"
#include "StudioVertex.h"
#include "materialsystem1/renderers/IGPUBuffer.h"

class IMaterial;
using IMaterialPtr = CRefPtr<IMaterial>;

class IGPUBuffer;
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

class IVertexFormat;
class CBaseEqGeomInstancer;
struct MeshInstanceData;
struct RenderDrawCmd;
struct DecalMakeInfo;
struct DecalData;
struct RenderBoneTransform;
struct RenderPassContext;

enum EModelLoadingState
{
	MODEL_LOAD_ERROR = -1,
	MODEL_LOAD_IN_PROGRESS = 0,
	MODEL_LOAD_OK,
};

#define EGF_LOADING_CRITICAL_SECTION(m)	\
	while(m->GetLoadingState() != MODEL_LOAD_OK) { Platform_Sleep(1); }

// streams in studio models used exclusively in interpolation
class CEqStudioGeom : public RefCountedObject<CEqStudioGeom>
{
	friend class CStudioCache;
public:
	struct DrawProps;
	struct HWGeomRef;

	static void					SetInstanceFormatId(int instanceFormatId);

	CEqStudioGeom();
	~CEqStudioGeom();

	int						GetCacheId() const { return m_nameHash; }
	const char*				GetName() const;
	EModelLoadingState		GetLoadingState() const;	// EModelLoadingState
	Future<bool>			GetLoadingFuture() const;

	void					AddMotionPackage(const char* filename);

	const studioHdr_t&		GetStudioHdr() const;
	const StudioPhysData&	GetPhysData() const;
	ArrayCRef<StudioJoint>	GetJoints() const;
	ArrayCRef<int>			GetMotionDataIdxs() const;
	ArrayCRef<HWGeomRef>	GetHwGeomRefs() const { return m_hwGeomRefs; }
	

	const IMaterialPtr&		GetMaterial(int materialIdx, int materialGroupIdx = 0) const;
	ArrayCRef<IMaterialPtr>	GetMaterials(int materialGroupIdx = 0) const;
	int						GetMaterialGroupsCount() const { return m_materialGroupsCount; }

	// instancing
	void					SetInstancer(CBaseEqGeomInstancer* instancer);
	CBaseEqGeomInstancer*	GetInstancer() const;

	// selects a lod. returns index
	int						SelectLod(float distance) const;
	int						FindManualLod(float value) const;

	IGPUBufferPtr			GetVertexBuffer(EGFHwVertex::VertexStreamId vertStream) const;
	IGPUBufferPtr			GetIndexBuffer() const { return m_indexBuffer; }
	int						GetIndexFormat() const { return m_indexFmt; }
	int						ConvertBoneMatricesToQuaternions(const Matrix4x4* boneMatrices, RenderBoneTransform* bquats) const;

	void					Draw(const DrawProps& drawProperties, const MeshInstanceData& instData, const RenderPassContext& passContext) const;

	const BoundingBox&		GetBoundingBox() const;
	CRefPtr<DecalData>		MakeDecal(const DecalMakeInfo& info, Matrix4x4* jointMatrices, int bodyGroupFlags, int lod = 0) const;
	float					CheckIntersectionWithRay(const Vector3D& rayStart, const Vector3D& rayDir, int bodyGroupFlags, int lod = 0) const;
private:
	void					Ref_DeleteObject() override;

	bool					LoadModel(const char* pszPath, bool useJob = true);
	void					DestroyModel();
	
	bool					LoadFromFile();
	void					LoadMaterials();
	void					LoadPhysicsData(); // loads physics object data
	bool					LoadGenerateVertexBuffer();
	void					LoadMotionPackages();
	void					LoadSetupBones();

	//-----------------------------------------------

	using MaterialList = FixedArray<IMaterialPtr, MAX_STUDIOMATERIALS>;
	using MotionDataList = FixedArray<int, MAX_MOTIONPACKAGES>;

	Future<bool>			m_loadingFuture;

	// array of material index for each group
	MaterialList			m_materials;
	MotionDataList			m_motionData;

	Array<EqString>			m_additionalMotionPackages{ PP_SL };
	BoundingBox				m_boundingBox; // FIXME: bounding boxes for each groups?
	EqString				m_name;
	int						m_nameHash{ 0 };
	int						m_cacheIdx{ 0 };

	StudioJoint*			m_joints{ nullptr };
	ArrayRef<HWGeomRef>		m_hwGeomRefs{ nullptr };	// hardware representation of models (indices)

	CBaseEqGeomInstancer*	m_instancer{ nullptr };
	studioHdr_t*			m_studio{ nullptr };
	StudioPhysData			m_physModel;

	IGPUBufferPtr			m_vertexBuffers[EGFHwVertex::VERT_COUNT];
	IGPUBufferPtr			m_indexBuffer;
	int						m_indexFmt{ -1 };

	int						m_materialCount{ 0 };
	int						m_materialGroupsCount{ 0 };

	volatile int			m_readyState{ 0 };
};

extern ArrayCRef<EGFHwVertex::VertexStreamId> g_defaultVertexStreamMapping;

struct CEqStudioGeom::DrawProps
{
	using SetupDrawFunc = EqFunction<void(RenderDrawCmd& drawCmd)>;
	using BodyGroupFunc = EqFunction<void(RenderDrawCmd& drawCmd, IMaterial* material, int bodyGroup, int meshIndex)>;

	// DEPRECATED
	ArrayCRef<EGFHwVertex::VertexStreamId> vertexStreamMapping{ g_defaultVertexStreamMapping };
	IVertexFormat*			vertexFormat{ nullptr };
	// END DEPRECATED

	//MeshInstanceFormatRef	instFormat;
	GPUBufferView			boneTransforms; // BSKN uniform buffer

	SetupDrawFunc			setupDrawCmd;	// called once before entire EGF is drawn
	BodyGroupFunc			setupBodyGroup;	// called multiple times before body group is drawn
	
	int						bodyGroupFlags{ -1 };
	int						materialGroup{ 0 };
	int						lod{ 0 };

	int						materialFlags{ -1 };
	bool					excludeMaterialFlags{ false };
	bool					skipMaterials{ false };
};

struct CEqStudioGeom::HWGeomRef
{
	// offset in hw index buffer to this lod, for each geometry group
	struct MeshRef
	{
		int		firstIndex{ 0 };
		int		indexCount{ 0 };
		uint8	primType{ 0 };
		uint8	materialIdx{ 0xff };
		bool	supportsSkinning{ false };
	};
	ArrayRef<MeshRef>	meshRefs{ nullptr };
};