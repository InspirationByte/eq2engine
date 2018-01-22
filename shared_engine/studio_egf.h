//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium model loader
//////////////////////////////////////////////////////////////////////////////////

#ifndef CENGINEMODEL_H
#define CENGINEMODEL_H

#include "IEqModel.h"
#include "modelloader_shared.h"
#include "materialsystem/IMaterialSystem.h"

// streams in studio models used exclusively in interpolation
class CEngineStudioEGF : public IEqModel
{
	friend class		CModelCache;

public:
						CEngineStudioEGF();
						~CEngineStudioEGF();

	static void ModelLoaderJob( void* data, int i );

	bool				LoadModel( const char* pszPath, bool useJob = true );

	const char*			GetName() const;

//------------------------------------------------------------
// base methods
//------------------------------------------------------------
	
	void				DestroyModel();

	studioHwData_t*		GetHWData() const;

	// selects a lod. returns index
	int					SelectLod(float dist_to_camera);

	const BoundingBox&	GetAABB() const;

	// makes dynamic temporary decal
	studiotempdecal_t*	MakeTempDecal( const decalmakeinfo_t& info, Matrix4x4* jointMatrices);

	// instancing
	void				SetInstancer( IEqModelInstancer* instancer );
	IEqModelInstancer*	GetInstancer() const;

	int					GetLoadingState() const {return m_readyState;}

//------------------------------------
// Rendering
//------------------------------------

	void				DrawFull();
	void				DrawGroup(int nModel, int nGroup, bool preSetVBO = true);

	void				SetupVBOStream( int nStream );

	void				DrawDebug();

	void				SetupBones();

	bool				PrepareForSkinning(Matrix4x4* jointMatrices);

	IMaterial*			GetMaterial(int materialIdx);

	// loads materials for studio
	void				LoadMaterials();

	Vector3D			GetModelInitialOrigin() {return Vector3D(0);}

private:

	bool				LoadFromFile();

	void				LoadPhysicsData(); // loads physics object data
	bool				LoadGenerateVertexBuffer();
	void				LoadMotionPackages();

	//-----------------------------------------------

	IEqModelInstancer*	m_instancer;
	studioHwData_t*		m_hwdata;

	IVertexBuffer*		m_pVB;
	IIndexBuffer*		m_pIB;

	// array of material index for each group
	IMaterial*			m_materials[MAX_STUDIOMATERIALS];

	bool				m_forceSoftwareSkinning;
	bool				m_skinningDirty;

	uint8				m_numMaterials;
	volatile short		m_readyState;

	int					m_numVertices;
	int					m_numIndices;

	EGFHwVertex_t*		m_softwareVerts;

	BoundingBox			m_aabb;
	EqString			m_szPath;

	int					m_cacheIdx;

};

//-------------------------------------------------------------------------------------
// Model cache implementation
//-------------------------------------------------------------------------------------

class CModelCache : public IModelCache
{
	friend class			CEngineStudioEGF;

public:
							CModelCache();

	// caches model and returns it's index
	int						PrecacheModel(const char* modelName);

	// returns count of cached models
	int						GetCachedModelCount() const;

	IEqModel*				GetModel(int index) const;
	const char* 			GetModelFilename(IEqModel* pModel) const;
	int						GetModelIndex(const char* modelName) const;
	int						GetModelIndex(IEqModel* pModel) const;

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