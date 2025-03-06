//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Graphics File script compler and generator
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "EGFPhysicsGenerator.h"

namespace SharedModel
{
	struct DSModel;
	using DSModelPtr = CRefPtr<DSModel>;

	struct DSBone;
	struct DSMesh;
	struct DSVertex;
	struct DSWeight;

	struct DSShapeData;
	struct DSShapeKey;
	using DSShapeDataPtr = CRefPtr<DSShapeData>;
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
	bool		InitFromKeyValues(const KVSection* kvs);
	void		Cleanup();

	void		SetRefsPath(const char* path);
	void		SetOutputFilename(const char* filename);

	bool		GenerateEGF();
	bool		GeneratePOD();

protected:
	struct GenLODList;
	struct GenModel;
	struct GenIKChain;
	struct GenIKLink;
	struct GenBone;
	struct GenMaterialDesc;
	struct GenMaterialGroup;

	// helper functions
	GenBone*				FindBoneByName(const char* pszName) const;
	GenLODList*				FindModelLodGroupByName(const char* pszName) const;
	int						FindModelIndexByName(const char* pszName) const;
	GenModel*				FindModelByName(const char* pszName) const;

	int						FindModelLodIdGroupByName(const char* pszName) const;
	int						GetMaterialIndex(const char* pszName) const;

	// loader functions
	bool					LoadModel(const char* pszFileName, GenModel& mod);
	void					FreeModel(GenModel& mod );
	bool					PostProcessDSM(GenModel& mod );

	void					LoadModelsFromFBX(const KVSection* pKeyBase);
	int						ParseAndLoadModels(const KVSection* pKeyBase);

	bool					ParseModels(const KVSection* pSection);
	void					ParseLodData(const KVSection* pSection, int lodIdx);
	void					ParseLods(const KVSection* pSection);
	bool					ParseBodyGroups(const KVSection* pSection);
	bool					ParseMaterialGroups(const KVSection* pSection);
	bool					ParseMaterialPaths(const KVSection* pSection);
	bool					ParseMotionPackagePaths(const KVSection* pSection);
	void					ParseIKChain(const KVSection* pSection);
	void					ParseIKChains(const KVSection* pSection);
	void					ParseAttachments(const KVSection* pSection);
	void					ParsePhysModels(const KVSection* pSection);

	// preprocessing
	void					MergeBones();
	void					BuildBoneChains();

	int						UsedMaterialIndex(const char* pszName);

	// writing to stream	
	void					WriteGroup(studioHdr_t* header, IVirtualStream* stream, SharedModel::DSMesh* srcGroup, SharedModel::DSShapeKey* modShapeKey, float simplifyThreshold, studioMeshDesc_t* dstGroup);

	void					WriteModels(studioHdr_t* header, IVirtualStream* stream);
	void					WriteLods(studioHdr_t* header, IVirtualStream* stream);
	void					WriteBodyGroups(studioHdr_t* header, IVirtualStream* stream);
	void					WriteAttachments(studioHdr_t* header, IVirtualStream* stream);
	void					WriteIkChains(studioHdr_t* header, IVirtualStream* stream);
	void					WriteMaterialDescs(studioHdr_t* header, IVirtualStream* stream);
	void					WriteMaterialPaths(studioHdr_t* header, IVirtualStream* stream);
	void					WriteMotionPackageList(studioHdr_t* header, IVirtualStream* stream);
	void					WriteBones(studioHdr_t* header, IVirtualStream* stream);

	void					Validate(studioHdr_t* header, const char* stage);

	// data
	Array<GenModel>				m_modelrefs{ PP_SL };		// all loaded model references

	Array<GenLODList>			m_modelLodLists{ PP_SL };	// all LOD reference models including main LOD
	Array<studioLodParams_t>	m_lodparams{ PP_SL };		// lod parameters
	Array<motionPackageDesc_t>	m_motionpacks{ PP_SL };		// motion packages
	Array<materialPathDesc_t>	m_matpathes{ PP_SL };		// material paths
	Array<GenIKChain>			m_ikchains{ PP_SL };		// ik chain list
	Array<GenBone>				m_bones{ PP_SL };			// bone list
	Array<studioTransform_t>	m_transforms{ PP_SL };		// attachment list
	Array<studioBodyGroup_t>	m_bodygroups{ PP_SL };		// body group list
	Array<GenMaterialDesc>		m_materials{ PP_SL };		// materials that referenced by models

	// only participates in write
	Array<GenMaterialDesc*>		m_usedMaterials{ PP_SL };	// materials that used by models referenced by body groups
	Array<GenMaterialGroup*>	m_matGroups{ PP_SL };		// material groups

	// settings
	Vector3D					m_modelScale{ 1.0f };
	Vector3D					m_modelOffset{ 0.0f };
	bool						m_notextures{ false };

	EqString					m_refsPath;
	EqString					m_outputFilename;

	CEGFPhysicsGenerator		m_physModels;
};

struct CEGFGenerator::GenBone
{
	SharedModel::DSBone* refBone{ nullptr };

	Array<GenBone*>	childs{ PP_SL };
	GenBone* parent{ nullptr };
};

struct CEGFGenerator::GenIKLink
{
	Vector3D	mins;
	Vector3D	maxs;

	CEGFGenerator::GenBone* bone;

	float		damping;
};

struct CEGFGenerator::GenIKChain
{
	char		name[44]{ 0 };
	Array<GenIKLink> link_list{ PP_SL };
};

struct CEGFGenerator::GenModel
{
	SharedModel::DSModelPtr		model{ nullptr };
	SharedModel::DSShapeDataPtr	shapeData{ nullptr };
	Matrix4x4	transform{ identity4 };
	EqString	name;
	int			shapeIndex{ -1 };
	int			meshGroupIdx{ -1 };				// model indices are remapped during write as not every modelref is used
	float		simplifyThreshold{ 0.0f };
};

struct CEGFGenerator::GenLODList
{
	FixedArray<int, MAX_MODEL_LODS>	lodmodels;
	EqString	name;
	int			used{ 0 };
	int			writeIdx{ -1 };
};

struct CEGFGenerator::GenMaterialDesc
{
	char		materialname[32]{ 0 };
	int			used{ 0 };
};

struct CEGFGenerator::GenMaterialGroup
{
	Array<GenMaterialDesc> materials{ PP_SL };
};