//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium model loader
//////////////////////////////////////////////////////////////////////////////////

#ifndef CENGINEMODEL_H
#define CENGINEMODEL_H

#include "egf/IEqModel.h"
#include "modelloader_shared.h"
#include "utils/eqthread.h"

class IVertexBuffer;
class IIndexBuffer;


// streams in studio models used exclusively in interpolation
class CEngineStudioEGF : public IEqModel
{
	friend class		CStudioModelCache;
public:

//------------------------------------------------------------

						CEngineStudioEGF();
						~CEngineStudioEGF();

	// model type
	EModelType			GetModelType() const					{return MODEL_TYPE_STUDIO;}

//------------------------------------------------------------
// base methods
//------------------------------------------------------------
	
	bool				LoadModel(const char* pszPath, bool useJob = true);

	void				DestroyModel();

	const char*			GetName() const;

	const BoundingBox&	GetAABB() const;

	studioHwData_t*		GetHWData() const;

	// makes dynamic temporary decal
	tempdecal_t*		MakeTempDecal( const decalmakeinfo_t& info, Matrix4x4* jointMatrices);

	int					GetLoadingState() const;

	// loads materials for studio
	void				LoadMaterials();
	void				LoadMotionPackage(const char* filename);

//------------------------------------
// Rendering
//------------------------------------

	// instancing
	void				SetInstancer(IEqModelInstancer* instancer);
	IEqModelInstancer*	GetInstancer() const;

	// selects a lod. returns index
	int					SelectLod(float dist_to_camera);

	void				DrawFull();
	void				DrawGroup(int nModel, int nGroup, bool preSetVBO = true);

	void				SetupVBOStream( int nStream );

	bool				PrepareForSkinning(Matrix4x4* jointMatrices);

	IMaterial*			GetMaterial(int materialIdx, int materialGroupIdx = 0);

private:

	static void			LoadModelJob(void* data, int i);
	
	static void			LoadVertsJob(void* data, int i);
	static void			LoadPhysicsJob(void* data, int i);
	static void			LoadMotionJob(void* data, int i);
	
	static void			OnLoadingJobComplete(struct eqParallelJob_t* job);
	
	bool				LoadFromFile();

	void				LoadPhysicsData(); // loads physics object data
	bool				LoadGenerateVertexBuffer();
	void				LoadMotionPackages();

	void				LoadSetupBones();

	//-----------------------------------------------

	// array of material index for each group
	IMaterial*			m_materials[MAX_STUDIOMATERIALS];
	DkList<EqString>	m_additionalMotionPackages;
	BoundingBox			m_aabb;
	EqString			m_szPath;

	IEqModelInstancer*	m_instancer;
	studioHwData_t*		m_hwdata;

	IVertexBuffer*		m_pVB;
	IIndexBuffer*		m_pIB;

	EGFHwVertex_t*		m_softwareVerts;

	bool				m_forceSoftwareSkinning;
	bool				m_skinningDirty;

	int					m_numMaterials;

	int					m_numVertices;
	int					m_numIndices;

	int					m_cacheIdx;

	volatile short						m_readyState;
	Threading::CEqInterlockedInteger	m_loading;
};

//-------------------------------------------------------------------------------------
// Model cache implementation
//-------------------------------------------------------------------------------------

class CStudioModelCache : public IStudioModelCache
{
	friend class			CEngineStudioEGF;

public:
							CStudioModelCache();

	// caches model and returns it's index
	int						PrecacheModel(const char* modelName);

	// returns count of cached models
	int						GetCachedModelCount() const;

	IEqModel*				GetModel(int index) const;
	const char* 			GetModelFilename(IEqModel* pModel) const;
	int						GetModelIndex(const char* modelName) const;
	int						GetModelIndex(IEqModel* pModel) const;

	void					FreeCachedModel(IEqModel* pModel);

	void					ReleaseCache();
	void					ReloadModels();

	IVertexFormat*			GetEGFVertexFormat() const;		// returns EGF vertex format

	// prints loaded models to console
	void					PrintLoadedModels() const;

private:

	DkList<IEqModel*>		m_cachedList;
	IVertexFormat*			m_egfFormat;	// vertex format for streams
};

#endif // CENGINEMODEL_H