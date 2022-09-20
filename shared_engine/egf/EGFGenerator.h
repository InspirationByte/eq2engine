//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Graphics File script compler and generator
//////////////////////////////////////////////////////////////////////////////////

#pragma once
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
	SharedModel::dsmskelbone_t*	referencebone{ nullptr };

	Array<cbone_t*>		childs{ PP_SL };
	cbone_t*			parent{ nullptr };
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
	char name[44]{ 0 };
	Array<ciklink_t> link_list{ PP_SL };
};

struct clodmodel_t
{
	SharedModel::dsmmodel_t*	lodmodels[MAX_MODEL_LODS]{ nullptr };
};

struct egfcaModel_t
{
	SharedModel::dsmmodel_t*		model{ nullptr };
	SharedModel::esmshapedata_t*	shapeData{ nullptr };

	int								shapeBy{ -1 };	// shape index
	int								used{ 0 };
};

struct egfcaMaterialDesc_t
{
	char				materialname[32]{ 0 };
	int					used{ 0 };
};

struct egfcaMaterialGroup_t
{
	Array<egfcaMaterialDesc_t> materials{ PP_SL };
};

class IVirtualStream;

//
// EGF model generator (EDITOR-friendly)
//
class CEGFGenerator
{
public:

	CEGFGenerator();
	virtual ~CEGFGenerator();

	bool		InitFromKeyValues(const char* filename);
	bool		InitFromKeyValues(KVSection* kvs);
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
	bool					PostProcessDSM( egfcaModel_t& mod );

	void						LoadModelsFromFBX(KVSection* pKeyBase);
	SharedModel::dsmmodel_t*	ParseAndLoadModels(KVSection* pKeyBase);
	bool						LoadModels(KVSection* pSection);
	void						ParseLodData(KVSection* pSection, int lodIdx);
	void						LoadLods(KVSection* pSection);
	bool						LoadBodyGroups(KVSection* pSection);
	bool						LoadMaterialGroups(KVSection* pSection);
	bool						LoadMaterialPaths(KVSection* pSection);
	bool						LoadMotionPackagePaths(KVSection* pSection);

	void					AddModelLodUsageReference(int modelLodIndex);

	void					ParseIKChain(KVSection* pSection);
	void					LoadIKChains(KVSection* pSection);
	void					LoadAttachments(KVSection* pSection);

	void					LoadPhysModels(KVSection* pSection);

	// preprocessing
	void					MergeBones();
	void					BuildBoneChains();

	int						UsedMaterialIndex(const char* pszName);

	// writing to stream	
	void					WriteGroup(studiohdr_t* header, IVirtualStream* stream, SharedModel::dsmgroup_t* srcGroup, SharedModel::esmshapekey_t* modShapeKey, modelgroupdesc_t* dstGroup);

	void					WriteModels(studiohdr_t* header, IVirtualStream* stream);
	void					WriteLods(studiohdr_t* header, IVirtualStream* stream);
	void					WriteBodyGroups(studiohdr_t* header, IVirtualStream* stream);
	void					WriteAttachments(studiohdr_t* header, IVirtualStream* stream);
	void					WriteIkChains(studiohdr_t* header, IVirtualStream* stream);
	void					WriteMaterialDescs(studiohdr_t* header, IVirtualStream* stream);
	void					WriteMaterialPaths(studiohdr_t* header, IVirtualStream* stream);
	void					WriteMotionPackageList(studiohdr_t* header, IVirtualStream* stream);
	void					WriteBones(studiohdr_t* header, IVirtualStream* stream);

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
