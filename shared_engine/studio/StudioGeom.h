//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Studio Geometry Form
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/model.h"

class IMaterial;
class IVertexFormat;
class IVertexBuffer;
class IIndexBuffer;
class CBaseEqGeomInstancer;

using IMaterialPtr = CRefPtr<IMaterial>;

struct DecalMakeInfo;
struct DecalData;

enum EModelLoadingState
{
	MODEL_LOAD_ERROR = -1,
	MODEL_LOAD_IN_PROGRESS = 0,
	MODEL_LOAD_OK,
};

#define EGF_LOADING_CRITICAL_SECTION(m)	\
	while(m->GetLoadingState() != MODEL_LOAD_OK) {	g_parallelJobs->CompleteJobCallbacks(); Platform_Sleep(1); }

// streams in studio models used exclusively in interpolation
class CEqStudioGeom
{
	friend class CStudioCache;
	friend class CBaseEqGeomInstancer;
public:

	struct DrawProps;

	CEqStudioGeom();
	~CEqStudioGeom();

	int							GetCacheIndex() const { return m_cacheIdx; }

	const char*					GetName() const;
	int							GetLoadingState() const;	// EModelLoadingState
	void						LoadMotionPackage(const char* filename);

	int							GetMotionPackageCount() const { return m_motionData.numElem(); }
	int							GetMaterialCount() const { return m_materialCount; }
	int							GetMaterialGroupsCount() const { return m_materialGroupsCount; }

	const studiohdr_t&			GetStudioHdr() const;
	const studioPhysData_t&		GetPhysData() const;
	const studioMotionData_t&	GetMotionData(int index) const;
	const studioJoint_t&		GetJoint(int index) const;
	Matrix4x4					GetLocalTransformMatrix(int transformIdx) const;

	const BoundingBox&			GetBoundingBox() const;

	// Makes dynamic temporary decal
	CRefPtr<DecalData>			MakeDecal(const DecalMakeInfo& info, Matrix4x4* jointMatrices, int bodyGroupFlags, int lod = 0) const;

	// Checks ray-egf intersection. Ray must be in local space
	float						CheckIntersectionWithRay(const Vector3D& rayStart, const Vector3D& rayDir, int bodyGroupFlags, int lod = 0) const;

	// instancing
	void						SetInstancer(CBaseEqGeomInstancer* instancer);
	CBaseEqGeomInstancer*		GetInstancer() const;

	// selects a lod. returns index
	int							SelectLod(float distance) const;
	int							FindManualLod(float value) const;

	void						Draw(const DrawProps& drawProperties) const;
	void						SetupVBOStream( int nStream ) const;

	const IMaterialPtr&			GetMaterial(int materialIdx, int materialGroupIdx = 0) const;

private:

	struct HWGeomRef
	{
		// offset in hw index buffer to this lod, for each geometry group
		struct Group
		{
			int		firstIndex;
			int		indexCount;
			ushort	primType;
		} *groups{ nullptr };
	};

	bool					LoadModel(const char* pszPath, bool useJob = true);
	void					DestroyModel();

	static void				LoadModelJob(void* data, int i);
	static void				LoadVertsJob(void* data, int i);
	static void				LoadPhysicsJob(void* data, int i);
	static void				LoadMotionJob(void* data, int i);
	
	static void				OnLoadingJobComplete(struct eqParallelJob_t* job);
	
	bool					LoadFromFile();
	void					LoadMaterials();
	void					LoadPhysicsData(); // loads physics object data
	bool					LoadGenerateVertexBuffer();
	void					LoadMotionPackages();
	void					LoadSetupBones();

	//-----------------------------------------------

	// array of material index for each group
	FixedArray<IMaterialPtr, MAX_STUDIOMATERIALS>		m_materials;
	FixedArray<studioMotionData_t*, MAX_MOTIONPACKAGES>	m_motionData;

	Array<EqString>			m_additionalMotionPackages{ PP_SL };
	BoundingBox				m_boundingBox; // FIXME: bounding boxes for each groups?
	EqString				m_name;
	
	studioJoint_t*			m_joints{ nullptr };
	HWGeomRef*				m_hwGeomRefs{ nullptr };	// hardware representation of models (indices)

	CBaseEqGeomInstancer*	m_instancer{ nullptr };
	studiohdr_t*			m_studio{ nullptr };
	studioPhysData_t		m_physModel;

	IVertexBuffer*			m_vertexBuffer{ nullptr };
	IIndexBuffer*			m_indexBuffer{ nullptr };

	int						m_materialCount{ 0 };
	int						m_materialGroupsCount{ 0 };

	int						m_cacheIdx{ -1 };

	mutable int				m_loading{ MODEL_LOAD_ERROR };
	mutable int				m_readyState{ 0 };

	EGFHwVertex_t*			m_softwareVerts{ nullptr };
	bool					m_forceSoftwareSkinning{ false };
	bool					m_skinningDirty{ false };
};

struct CEqStudioGeom::DrawProps
{
	using DrawFunc = EqFunction<void(IMaterial* material, int bodyGroup)>;

	Matrix4x4*		boneTransforms{ nullptr };
	IVertexFormat*	vertexFormat{ nullptr };

	DrawFunc		preSetupFunc;
	DrawFunc		preDrawFunc;

	int				mainVertexStream{ 0 };
	int				bodyGroupFlags{ -1 };
	int				materialGroup{ 0 };
	int				lod{ 0 };

	int				materialFlags{ -1 };
	bool			excludeMaterialFlags{ false };
	bool			instanced{ false };
	bool			skipMaterials{ false };
};