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

	const char*				GetName() const;
	int						GetLoadingState() const;	// EModelLoadingState

	// TODO: split studioHWData and remove later
	const studioHwData_t*	GetHWData() const;

	const studiohdr_t&		GetStudioHdr() const;
	const studioPhysData_t&	GetPhysData() const;
	const BoundingBox&		GetBoundingBox() const;

	// loads additional motion package
	void					LoadMotionPackage(const char* filename);

	// makes dynamic temporary decal
	tempdecal_t*			MakeTempDecal( const decalmakeinfo_t& info, Matrix4x4* jointMatrices);

	// instancing
	void					SetInstancer(CBaseEqGeomInstancer* instancer);
	CBaseEqGeomInstancer*	GetInstancer() const;

	// selects a lod. returns index
	int						SelectLod(float dist_to_camera) const;

	void					DrawGroup(int nModel, int nGroup, bool preSetVBO = true) const;

	void					SetupVBOStream( int nStream ) const;
	bool					PrepareForSkinning(Matrix4x4* jointMatrices);

	IMaterialPtr			GetMaterial(int materialIdx, int materialGroupIdx = 0) const;

private:

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
	FixedArray<IMaterialPtr, MAX_STUDIOMATERIALS>	m_materials;
	Array<EqString>			m_additionalMotionPackages{ PP_SL };
	BoundingBox				m_boundingBox;
	EqString				m_name;

	CBaseEqGeomInstancer*	m_instancer;
	studioHwData_t*			m_hwdata;

	IVertexBuffer*			m_pVB;
	IIndexBuffer*			m_pIB;

	EGFHwVertex_t*			m_softwareVerts;

	int						m_numVertices;
	int						m_numIndices;

	int						m_cacheIdx;
	Threading::CEqInterlockedInteger	m_loading;
	volatile short			m_readyState;

	bool					m_forceSoftwareSkinning;
	bool					m_skinningDirty;


};

