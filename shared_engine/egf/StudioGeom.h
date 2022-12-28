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

struct decalmakeinfo_t;
struct tempdecal_t;

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
	friend class		CStudioCache;
public:

	CEqStudioGeom();
	~CEqStudioGeom();

	const char*					GetName() const;
	int							GetLoadingState() const;	// EModelLoadingState

	// loads additional motion package
	void						LoadMotionPackage(const char* filename);

	// TODO: split studioHWData and remove later
	const studiohdr_t&			GetStudioHdr() const;
	const studioPhysData_t&		GetPhysData() const;
	const studioMotionData_t&	GetMotionData(int index) const;
	const studioJoint_t&		GetJoint(int index) const;

	int							GetMotionPackageCount() const { return m_motionData.numElem(); }
	int							GetMaterialCount() const { return m_materialCount; }
	int							GetMaterialGroupsCount() const { return m_materialGroupsCount; }

	// makes dynamic temporary decal
	tempdecal_t*				MakeTempDecal( const decalmakeinfo_t& info, Matrix4x4* jointMatrices);
	const BoundingBox&			GetBoundingBox() const;

	// instancing
	void						SetInstancer(CBaseEqGeomInstancer* instancer);
	CBaseEqGeomInstancer*		GetInstancer() const;

	// selects a lod. returns index
	int							SelectLod(float distance) const;

	void						DrawGroup(int modelDescId, int modelGroup, bool preSetVBO = true) const;

	void						SetupVBOStream( int nStream ) const;
	bool						PrepareForSkinning(Matrix4x4* jointMatrices);

	IMaterialPtr				GetMaterial(int materialIdx, int materialGroupIdx = 0) const;

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