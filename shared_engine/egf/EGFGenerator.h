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
	struct GenBone_t
	{
		SharedModel::dsmskelbone_t* refBone{ nullptr };

		Array<GenBone_t*>	childs{ PP_SL };
		GenBone_t*			parent{ nullptr };
	};

	struct GenIKLink_t
	{
		Vector3D	mins;
		Vector3D	maxs;

		GenBone_t*	bone;

		float		damping;
	};

	struct GenIKChain_t
	{
		char name[44]{ 0 };
		Array<GenIKLink_t> link_list{ PP_SL };
	};

	struct GenModel_t
	{
		SharedModel::dsmmodel_t*		model{ nullptr };
		SharedModel::esmshapedata_t*	shapeData{ nullptr };

		int								shapeBy{ -1 };	// shape index
		int								used{ 0 };
	};

	struct GenLODList_t
	{
		SharedModel::dsmmodel_t*		lodmodels[MAX_MODEL_LODS]{ nullptr };
	};

	struct GenMaterialDesc_t
	{
		char				materialname[32]{ 0 };
		int					used{ 0 };
	};

	struct GenMaterialGroup_t
	{
		Array<GenMaterialDesc_t> materials{ PP_SL };
	};

	GenModel_t*				GetDummyModel();

	// helper functions
	GenBone_t*				FindBoneByName(const char* pszName);
	GenLODList_t*			FindModelLodGroupByName(const char* pszName);
	GenModel_t*				FindModelByName(const char* pszName);

	int						FindModelLodIdGroupByName(const char* pszName);
	int						GetMaterialIndex(const char* pszName);
	int						GetReferenceIndex(SharedModel::dsmmodel_t* pRef);

	// loader functions
	GenModel_t				LoadModel(const char* pszFileName);
	void					FreeModel(GenModel_t& mod );
	bool					PostProcessDSM(GenModel_t& mod );

	void						LoadModelsFromFBX(KVSection* pKeyBase);
	SharedModel::dsmmodel_t*	ParseAndLoadModels(KVSection* pKeyBase);

	bool					ParseModels(KVSection* pSection);
	void					ParseLodData(KVSection* pSection, int lodIdx);
	void					ParseLods(KVSection* pSection);
	bool					ParseBodyGroups(KVSection* pSection);
	bool					ParseMaterialGroups(KVSection* pSection);
	bool					ParseMaterialPaths(KVSection* pSection);
	bool					ParseMotionPackagePaths(KVSection* pSection);
	void					ParseIKChain(KVSection* pSection);
	void					ParseIKChains(KVSection* pSection);
	void					ParseAttachments(KVSection* pSection);
	void					ParsePhysModels(KVSection* pSection);

	void					AddModelLodUsageReference(int modelLodIndex);

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
	Array<GenModel_t>				m_modelrefs{ PP_SL };		// all loaded model references

	Array<GenLODList_t>				m_modelLodLists{ PP_SL };	// all LOD reference models including main LOD
	Array<studiolodparams_t>		m_lodparams{ PP_SL };		// lod parameters
	Array<motionpackagedesc_t>		m_motionpacks{ PP_SL };		// motion packages
	Array<materialpathdesc_t>		m_matpathes{ PP_SL };		// material paths
	Array<GenIKChain_t>				m_ikchains{ PP_SL };		// ik chain list
	Array<GenBone_t>				m_bones{ PP_SL };			// bone list
	Array<studioattachment_t>		m_attachments{ PP_SL };		// attachment list
	Array<studiobodygroup_t>		m_bodygroups{ PP_SL };		// body group list
	Array<GenMaterialDesc_t>		m_materials{ PP_SL };		// materials that referenced by models

	// only participates in write
	Array<GenMaterialDesc_t*>		m_usedMaterials{ PP_SL };	// materials that used by models referenced by body groups
	Array<GenMaterialGroup_t*>		m_matGroups{ PP_SL };		// material groups

	// settings
	Vector3D						m_modelScale{ 1.0f };
	Vector3D						m_modelOffset{ 0.0f };
	bool							m_notextures{ false };

	EqString						m_refsPath;
	EqString						m_outputFilename;

	CEGFPhysicsGenerator			m_physModels;
};
