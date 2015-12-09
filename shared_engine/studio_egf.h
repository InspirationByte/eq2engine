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
public:
						CEngineStudioEGF();
						~CEngineStudioEGF();

	bool				LoadModel(const char* pszPath);
	const char*			GetPath();

//------------------------------------------------------------
// base methods
//------------------------------------------------------------
	
	void				DestroyModel();

	const char*			GetName();

	studiohwdata_t*		GetHWData();

	// selects a lod. returns index
	int					SelectLod(float dist_to_camera);

	Vector3D			GetBBoxMins();
	Vector3D			GetBBoxMaxs();

	// makes dynamic temporary decal
	studiotempdecal_t*	MakeTempDecal( const decalmakeinfo_t& info, Matrix4x4* jointMatrices);

	// instancing
	void				SetInstancer( IEqModelInstancer* instancer );
	IEqModelInstancer*	GetInstancer() const;

//------------------------------------
// Rendering
//------------------------------------

	void				DrawFull();
	void				DrawGroup(int nModel, int nGroup, bool preSetVBO = true);

	void				SetupVBOStream( int nStream );

	void				DrawDebug();

	int8				FindBone(const char* pszName);

	void				SetupBones();

	bool				PrepareForSkinning(Matrix4x4* jointMatrices);

	IMaterial*			GetMaterial(int nModel, int nTexGroup);

	// loads materials for studio
	void				LoadMaterials();

	Vector3D			GetModelInitialOrigin() {return Vector3D(0);}

	void				LoadPOD(const char* path); // loads physics object data

private:

	IEqModelInstancer*	m_instancer;

	EqString			m_szPath;

	studiohwdata_t*		m_pHardwareData;

	bool				m_bSoftwareSkinned;
	bool				m_bSoftwareSkinChanged;

	IVertexBuffer*		m_pVB;
	IIndexBuffer*		m_pIB;

	// array of material index for each group
	IMaterial*			m_pMaterials[MAX_STUDIOGROUPS];

	int					m_nNumMaterials;
	int					m_nNumGroups;

	int					m_numVertices;
	int					m_numIndices;

	EGFHwVertex_t*		m_pSWVertices;

	Vector3D			m_vBBOX[2];
};

//-------------------------------------------------------------------------------------
// Model cache implementation
//-------------------------------------------------------------------------------------

class CModelCache : public IModelCache
{
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
	DkList<IEqModel*>		m_pModels;
	IVertexFormat*			m_egfFormat;	// vertex format for streams
};

#endif // CENGINEMODEL_H