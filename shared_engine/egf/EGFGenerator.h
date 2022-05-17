//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Graphics File script compler and generator
//////////////////////////////////////////////////////////////////////////////////

#ifndef EGFGENERATOR_H
#define EGFGENERATOR_H

#include "egf/model.h"
#include "EGFPhysicsGenerator.h"

namespace SharedModel
{
	struct dsmmodel_t;
	struct dsmskelbone_t;
	struct dsmgroup_t;
	struct dsmvertex_t;
	struct dsmweight_t;

	struct esmshapedata_t;
	struct esmshapekey_t;
};

struct cbone_t
{
	SharedModel::dsmskelbone_t*	referencebone;

	Array<cbone_t*>		childs{ PP_SL };
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

	Array<ciklink_t> link_list{ PP_SL };
};

struct clodmodel_t
{
	SharedModel::dsmmodel_t*	lodmodels[MAX_MODELLODS];
};

struct egfcaModel_t
{
	egfcaModel_t()
	{
		model = nullptr;
		shapeData = nullptr;
		shapeBy = -1;
		used = 0;
	}

	SharedModel::dsmmodel_t*		model;
	SharedModel::esmshapedata_t*	shapeData;

	int								shapeBy;	// shape index

	int								used;
};

struct egfcaMaterialDesc_t
{
	egfcaMaterialDesc_t()
	{
		used = 0;
	}

	char				materialname[32];
	int					used;
};

struct egfcaMaterialGroup_t
{
	Array<egfcaMaterialDesc_t> materials{ PP_SL };
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
	egfcaModel_t			GetDummyModel();

	// helper functions
	cbone_t*				FindBoneByName(const char* pszName);
	clodmodel_t*			FindModelLodGroupByName(const char* pszName);
	egfcaModel_t*			FindModelByName(const char* pszName);

	int						FindModelLodIdGroupByName(const char* pszName);
	int						GetMaterialIndex(const char* pszName);
	int						GetReferenceIndex(SharedModel::dsmmodel_t* pRef);

	// loader functions
	egfcaModel_t			LoadModel(const char* pszFileName);
	void					FreeModel( egfcaModel_t& mod );

	SharedModel::dsmmodel_t*			ParseAndLoadModels(kvkeybase_t* pKeyBase);
	bool					LoadModels(kvkeybase_t* pSection);
	void					ParseLodData(kvkeybase_t* pSection, int lodIdx);
	void					LoadLods(kvkeybase_t* pSection);
	bool					LoadBodyGroups(kvkeybase_t* pSection);
	bool					LoadMaterialGroups(kvkeybase_t* pSection);
	bool					LoadMaterialPaths(kvkeybase_t* pSection);
	bool					LoadMotionPackagePaths(kvkeybase_t* pSection);

	void					AddModelLodUsageReference(int modelLodIndex);

	void					ParseIKChain(kvkeybase_t* pSection);
	void					LoadIKChains(kvkeybase_t* pSection);
	void					LoadAttachments(kvkeybase_t* pSection);

	void					LoadPhysModels(kvkeybase_t* pSection);

	// preprocessing
	void					MergeBones();
	void					BuildBoneChains();

	int						UsedMaterialIndex(const char* pszName);

	// writing to stream	
	void					WriteGroup(CMemoryStream* stream, SharedModel::dsmgroup_t* srcGroup, SharedModel::esmshapekey_t* modShapeKey, modelgroupdesc_t* dstGroup);
	void					WriteModels(CMemoryStream* stream);
	void					WriteLods(CMemoryStream* stream);
	void					WriteBodyGroups(CMemoryStream* stream);
	void					WriteAttachments(CMemoryStream* stream);
	void					WriteIkChains(CMemoryStream* stream);
	void					WriteMaterialDescs(CMemoryStream* stream);
	void					WriteMaterialPaths(CMemoryStream* stream);
	void					WriteMotionPackageList(CMemoryStream* stream);
	void					WriteBones(CMemoryStream* stream);

	// data
	Array<egfcaModel_t>				m_modelrefs{ PP_SL };	// all loaded model references

	Array<clodmodel_t>				m_modellodrefs{ PP_SL };	// all LOD reference models including main LOD
	Array<studiolodparams_t>		m_lodparams{ PP_SL };	// lod parameters
	Array<motionpackagedesc_t>		m_motionpacks{ PP_SL };	// motion packages
	Array<materialpathdesc_t>		m_matpathes{ PP_SL };	// material paths
	Array<ikchain_t>				m_ikchains{ PP_SL };	// ik chain list
	Array<cbone_t>					m_bones{ PP_SL };		// bone list
	Array<studioattachment_t>		m_attachments{ PP_SL };	// attachment list
	Array<studiobodygroup_t>		m_bodygroups{ PP_SL };	// body group list

	Array<egfcaMaterialDesc_t>		m_materials{ PP_SL };	// materials that referenced by models
	Array<egfcaMaterialDesc_t*>		m_usedMaterials{ PP_SL };	// materials that used by models referenced by body groups
	Array<egfcaMaterialGroup_t*>	m_matGroups{ PP_SL };	// material groups

	Vector3D						m_modelScale;
	Vector3D						m_modelOffset;
	bool							m_notextures;

	EqString						m_refsPath;
	EqString						m_outputFilename;

	CEGFPhysicsGenerator			m_physModels;
};

#endif // EGFGENERATOR_H