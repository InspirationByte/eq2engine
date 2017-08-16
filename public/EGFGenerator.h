//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Geometry File script compler and generator
//////////////////////////////////////////////////////////////////////////////////

#ifndef EGFGENERATOR_H
#define EGFGENERATOR_H

#include "dsm_loader.h"
#include "dsm_obj_loader.h"
#include "model.h"

struct cbone_t
{
	dsmskelbone_t*		referencebone;

	DkList<cbone_t*>	childs;
	cbone_t*			parent;
};

struct ciklink_t
{
	Vector3D			mins;
	Vector3D			maxs;

	cbone_t*			bone;

	float				damping;
};

struct ikchain_t
{
	char name[44];

	DkList<ciklink_t> link_list;
};

struct clodmodel_t
{
	dsmmodel_t*			lodmodels[MAX_MODELLODS];
};

struct esmshapedata_t;
struct esmshapekey_t;

struct egfcamodel_t
{
	egfcamodel_t()
	{
		model = nullptr;
		shapeData = nullptr;
		shapeBy = -1;
	}

	dsmmodel_t*			model;
	esmshapedata_t*		shapeData;

	int					shapeBy;	// shape index
};

class CMemoryStream;

//
// EGF model generator (EDITOR-friendly)
//
class CEGFGenerator
{
public:

	CEGFGenerator();
	virtual ~CEGFGenerator();

	bool		InitFromKeyValues(const char* filename);
	bool		InitFromKeyValues(kvkeybase_t* kvs);
	void		Cleanup();

	void		SetRefsPath(const char* path);
	void		SetOutputFilename(const char* filename);

	bool		GenerateEGF();
	bool		GeneratePOD();

protected:
	// helper functions
	cbone_t*			FindBoneByName(const char* pszName);
	clodmodel_t*		FindModelLodGroupByName(const char* pszName);
	egfcamodel_t*		FindModelByName(const char* pszName);


	int					FindModelLodIdGroupByName(const char* pszName);
	int					GetMaterialIndex(const char* pszName);
	int					GetReferenceIndex(dsmmodel_t* pRef);

	// loader functions
	egfcamodel_t		LoadModel(const char* pszFileName);
	void				FreeModel( egfcamodel_t& mod );

	dsmmodel_t*			ParseAndLoadModels(kvkeybase_t* pKeyBase);
	bool				LoadModels(kvkeybase_t* pSection);
	void				ParseLodData(kvkeybase_t* pSection, int lodIdx);
	void				LoadLods(kvkeybase_t* pSection);
	bool				LoadBodyGroups(kvkeybase_t* pSection);
	bool				LoadMaterialPaths(kvkeybase_t* pSection);
	bool				LoadMotionPackagePatchs(kvkeybase_t* pSection);

	void				ParseIKChain(kvkeybase_t* pSection);
	void				LoadIKChains(kvkeybase_t* pSection);
	void				LoadAttachments(kvkeybase_t* pSection);

	// preprocessing
	void				MergeBones();
	void				BuildBoneChains();

	// writing to stream
	void				WriteGroup(CMemoryStream* stream, dsmgroup_t* srcGroup, esmshapekey_t* modShapeKey, modelgroupdesc_t* dstGroup);
	void				WriteModels(CMemoryStream* stream);
	void				WriteLods(CMemoryStream* stream);
	void				WriteBodyGroups(CMemoryStream* stream);
	void				WriteAttachments(CMemoryStream* stream);
	void				WriteIkChains(CMemoryStream* stream);
	void				WriteMaterialDescs(CMemoryStream* stream);
	void				WriteMaterialPaths(CMemoryStream* stream);
	void				WriteMotionPackageList(CMemoryStream* stream);
	void				WriteBones(CMemoryStream* stream);

	// data
	DkList<egfcamodel_t>			m_modelrefs;	// all loaded model references

	DkList<clodmodel_t>				m_modellodrefs;	// all LOD reference models including main LOD
	DkList<studiolodparams_t>		m_lodparams;	// lod parameters
	DkList<motionpackagedesc_t>		m_motionpacks;	// motion packages
	DkList<materialpathdesc_t>		m_matpathes;	// material paths
	DkList<ikchain_t>				m_ikchains;		// ik chain list
	DkList<cbone_t>					m_bones;		// bone list
	DkList<studioattachment_t>		m_attachments;	// attachment list
	DkList<studiobodygroup_t>		m_bodygroups;	// body group list
	DkList<studiomaterialdesc_t>	m_materials;	// materials that this model uses

	Vector3D						m_modelScale;
	Vector3D						m_modelOffset;
	bool							m_notextures;

	EqString						m_refsPath;
	EqString						m_outputFilename;

	kvkeybase_t*					m_physicsParams;
};

#endif // EGFGENERATOR_H