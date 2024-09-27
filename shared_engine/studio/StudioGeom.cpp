//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Studio Geometry Form
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IFileSystem.h"
#include "core/platform/eqjobmanager.h"

#include "math/Utility.h"

#include "studiofile/StudioLoader.h"
#include "StudioGeom.h"
#include "StudioCache.h"
#include "StudioGeomInstancer.h"

#include "physics/IStudioShapeCache.h"
#include "render/Decals.h"
#include "render/StudioRenderDefs.h"
#include "materialsystem1/IMaterialSystem.h"

using namespace Threading;

DECLARE_CVAR_CLAMP(r_egf_LodTest, "-1", -1.0f, MAX_MODEL_LODS, "Studio LOD test", CV_CHEAT);
DECLARE_CVAR(r_egf_NoTempDecals, "0", "Disables temp decals", CV_CHEAT);
DECLARE_CVAR(r_egf_LodScale, "1.0", "Studio model LOD scale", CV_ARCHIVE);
DECLARE_CVAR_CLAMP(r_egf_LodStart, "0", 0, MAX_MODEL_LODS, "Studio LOD start index", CV_ARCHIVE);
DECLARE_CVAR(r_force_softwareskinning, "0", "Force software skinning", CV_UNREGISTERED);


// Software vertex transformation, only for compatibility
static bool TransformEGFVertex(EGFHwVertex::PositionUV& vertPos, EGFHwVertex::TBN& tbn, EGFHwVertex::BoneWeights& vertWeight, Matrix4x4* pMatrices)
{
	bool bAffected = false;

	Vector3D pos = vec3_zero;
	Vector3D normal = vec3_zero;
	Vector3D tangent = vec3_zero;
	Vector3D binormal = vec3_zero;

	for (int i = 0; i < MAX_MODEL_VERTEX_WEIGHTS; ++i)
	{
		const int boneIdx = vertWeight.boneIndices[i];
		if (boneIdx == -1)
			continue;

		const float weight = vertWeight.boneWeights[i];
		pos += transformPoint(Vector3D(vertPos.pos.xyz()), pMatrices[boneIdx]) * weight;

		normal += rotateVector(Vector3D(tbn.normal), pMatrices[boneIdx]) * weight;
		tangent += rotateVector(Vector3D(tbn.tangent), pMatrices[boneIdx]) * weight;
		binormal += rotateVector(Vector3D(tbn.binormal), pMatrices[boneIdx]) * weight;

		bAffected = true;
	}

	if (bAffected)
	{
		vertPos.pos = Vector4D(pos, 1.0f);

		tbn.normal = normal;
		tbn.tangent = tangent;
		tbn.binormal = binormal;
	}

	return bAffected;
}

static int s_studioInstanceFormatId = 0;
void CEqStudioGeom::SetInstanceFormatId(int instanceFormatId)
{
	s_studioInstanceFormatId = instanceFormatId;
}

CEqStudioGeom::CEqStudioGeom()
	: m_cacheIdx(STUDIOCACHE_INVALID_IDX)
{
}

CEqStudioGeom::~CEqStudioGeom()
{
	DestroyModel();
}

void CEqStudioGeom::Ref_DeleteObject()
{
	g_studioCache->FreeModel(m_cacheIdx);
	RefCountedObject::Ref_DeleteObject();
}

int CEqStudioGeom::ConvertBoneMatricesToQuaternions(const Matrix4x4* boneMatrices, RenderBoneTransform* bquats) const
{
	const studioHdr_t& studio = GetStudioHdr();
	const int numBones = studio.numBones;

	ASSERT(!fisNan(boneMatrices[0].rows[0].x));

	for (int i = 0; i < numBones; i++)
	{
		const Matrix4x4 transform = m_joints[i].invAbsTrans * boneMatrices[i];

		// cast matrices to quaternions
		// note that quaternions uses transposes matrix set.
		bquats[i].quat = Quaternion(transform.getRotationComponentTransposed());
		bquats[i].origin = Vector4D(transform.getTranslationComponent(), 1);
	}

	return numBones;
}

#if 0
//------------------------------------------
// Software skinning of model. Very slow, but recomputes bounding box for all model
// Mobile GPUs could benefit from that at some point
//------------------------------------------
bool CEqStudioGeom::PrepareForSkinning(Matrix4x4* jointMatrices) const
{
	const studiohdr_t* studio = m_studio;

	if (!jointMatrices)
		return false;

	if (!studio)
		return false;

	if (studio->numBones == 0)
		return false; // model is not animatable, skip

	if (!r_force_softwareskinning.GetBool())
	{

		if (m_skinningDirty)
		{
			m_vertexBuffer->Update(m_softwareVerts, m_vertexBuffer->GetVertexCount() * sizeof(EGFHwVertex));
			m_skinningDirty = false;
		}

		RenderBoneTransform rendBoneTransforms[128];
		const int numBones = ComputeQuaternionsForSkinning(this, drawProperties.boneTransforms, rendBoneTransforms);
		g_matSystem->SetSkinningBones(ArrayCRef(rendBoneTransforms, numBones));

		return true;
	}
	else if (m_forceSoftwareSkinning && r_force_softwareskinning.GetBool())
	{
		m_skinningDirty = true;

		// multiply since jointMatrices are relative to bones
		Matrix4x4 tempMatrixArray[128];
		for (int i = 0; i < studio->numBones; i++)
			tempMatrixArray[i] = (!m_joints[i].absTrans * jointMatrices[i]);

		const int verticesCount = m_vertexBuffer->GetVertexCount();
		EGFHwVertex* bufferData = nullptr;
		if (m_vertexBuffer->Lock(0, verticesCount, (void**)&bufferData, BUFFERFLAG_WRITE))
		{
			// setup each bone's transformation
			for (int i = 0; i < verticesCount; i++)
			{
				EGFHwVertex vert = m_softwareVerts[i];
				TransformEGFVertex(vert, tempMatrixArray);

				bufferData[i] = vert;
			}

			m_vertexBuffer->Unlock();
			return false;
		}
	}

	return false;
}
#endif

void CEqStudioGeom::DestroyModel()
{
	DevMsg(DEVMSG_CORE, "DestroyModel: '%s'\n", m_name.ToCString());

	Atomic::Exchange(m_readyState, MODEL_LOAD_ERROR);

	SAFE_DELETE(m_instancer);

	for (int i = 0; i < EGFHwVertex::VERT_COUNT; ++i)
		m_vertexBuffers[i] = nullptr;
	m_indexBuffer = nullptr;

	m_motionData.clear();
	m_materials.clear(true);
	m_materialCount = 0;
	m_materialGroupsCount = 0;
	m_boundingBox.Reset();

	g_studioShapeCache->DestroyStudioCache(m_physModel);
	Studio_FreePhysModel(m_physModel);
	m_physModel = {};

	if (m_studio)
	{
		for (int i = 0; i < m_hwGeomRefs.numElem(); i++)
			PPDeleteArrayRef(m_hwGeomRefs[i].meshRefs);

		Studio_FreeModel(m_studio);
	}

	PPDeleteArrayRef(m_hwGeomRefs);
	SAFE_DELETE_ARRAY(m_joints);
}

void CEqStudioGeom::LoadPhysicsData()
{
	const EqString physDataFilename = fnmPathApplyExt(m_name, s_egfPhysicsObjectExt);

	if (!Studio_LoadPhysModel(physDataFilename, m_physModel))
		return;

	DevMsg(DEVMSG_CORE, "Loaded physics object data '%s'\n", physDataFilename.ToCString());
	g_studioShapeCache->InitStudioCache(m_physModel);
}

static int CopyGroupVertexDataToHWList(EGFHwVertex::PositionUV* hwVertPosList, EGFHwVertex::TBN* hwVertTbnList, EGFHwVertex::BoneWeights* hwVertWeightList, EGFHwVertex::Color* hwVertColorList, int currentVertexCount, const studioMeshDesc_t* pMesh, BoundingBox& aabb)
{
	for (int32 i = 0; i < pMesh->numVertices; i++)
	{
		if(hwVertPosList)
		{
			if (pMesh->vertexType & STUDIO_VERTFLAG_POS_UV)
			{
				const studioVertexPosUv_t* vertPosUv = pMesh->pPosUvs(i);
				hwVertPosList[currentVertexCount] = EGFHwVertex::PositionUV(*vertPosUv);
				aabb.AddVertex(vertPosUv->point);
			}
		}
		if(hwVertTbnList)
		{
			if (pMesh->vertexType & STUDIO_VERTFLAG_TBN)
				hwVertTbnList[currentVertexCount] = EGFHwVertex::TBN(*pMesh->pTBNs(i));
		}
		if(hwVertWeightList)
		{
			if (pMesh->vertexType & STUDIO_VERTFLAG_BONEWEIGHT)
				hwVertWeightList[currentVertexCount] = EGFHwVertex::BoneWeights(*pMesh->pBoneWeight(i));
			else
				hwVertWeightList[currentVertexCount].boneIndices[0] = -1;
		}
		if (hwVertColorList)
		{
			if (pMesh->vertexType & STUDIO_VERTFLAG_COLOR)
				hwVertColorList[currentVertexCount] = EGFHwVertex::Color(*pMesh->pColor(i));
			else
				hwVertColorList[currentVertexCount].color = color_white.pack();
		}
		++currentVertexCount;
	}

	return pMesh->numVertices;
}

static int CopyGroupIndexDataToHWList(void* indexData, int indexSize, int currentIndexCount, const studioMeshDesc_t* pMesh, int vertex_add_offset)
{
	if (indexSize == sizeof(int))
	{
		uint32* indices = (uint32*)indexData;
		for (uint32 i = 0; i < pMesh->numIndices; i++)
		{
			const uint32 index = (*pMesh->pVertexIdx(i)) + vertex_add_offset;
			indices[currentIndexCount++] = index;
		}
	}
	else if (indexSize == sizeof(short))
	{
		uint16* indices = (uint16*)indexData;
		for (uint32 i = 0; i < pMesh->numIndices; i++)
		{
			const uint32 index = (*pMesh->pVertexIdx(i)) + vertex_add_offset;
			indices[currentIndexCount++] = index & 0xffff;
		}
	}
	else
	{
		ASSERT_FAIL("CopyGroupIndexDataToHWList - Bad index size");
	}

	return pMesh->numIndices;
}

bool CEqStudioGeom::LoadModel(const char* pszPath, bool useJob)
{
	m_name = pszPath;
	fnmPathFixSeparators(m_name);

	// first we switch to loading
	Atomic::Exchange(m_readyState, MODEL_LOAD_IN_PROGRESS);

	if (!LoadFromFile())
	{
		DestroyModel();
		return false;
	}

	if (useJob)
	{
		Promise<bool> loadingPromise;
		m_loadingFuture = loadingPromise.CreateFuture();

		FunctionJob* loadModelJob = PPNew FunctionJob("LoadEGF", [this](void*, int) {
			LoadMaterials();
			LoadPhysicsData();
			LoadSetupBones();
		});
		loadModelJob->DeleteOnFinish();
		loadModelJob->InitJob();

		CEqJobManager* jobMng = g_studioCache->GetJobMng();

		const int numPackages = m_studio->numMotionPackages
			+ g_fileSystem->FileExist(fnmPathApplyExt(m_name, s_egfMotionPackageExt), SP_MOD);

		FunctionJob* loadGeomJob = PPNew FunctionJob("LoadEGFHWGeom", [this](void*, int) {
			DevMsg(DEVMSG_CORE, "Loading HW geom for %s, state: %d\n", GetName());

			if (!LoadGenerateVertexBuffer())
			{
				DestroyModel();
				return;
			}
		});
		loadGeomJob->DeleteOnFinish();
		loadGeomJob->InitJob();

		FunctionJob* finishJob = PPNew FunctionJob("FinishSyncJob", [this, loadingPromise](void*, int) {
			if (m_readyState != MODEL_LOAD_ERROR)
				Atomic::Exchange(m_readyState, MODEL_LOAD_OK);

			DevMsg(DEVMSG_CORE, "Finished loading %s, state: %d\n", GetName(), m_readyState);
			loadingPromise.SetResult(m_readyState == MODEL_LOAD_OK);
			m_loadingFuture = nullptr;
		});
		finishJob->DeleteOnFinish();
		finishJob->InitJob();
		finishJob->AddWait(loadModelJob);
		finishJob->AddWait(loadGeomJob);

		if (numPackages)
		{
			FunctionJob* loadMotionsJob = PPNew FunctionJob("LoadStudioMotion", [this](void*, int) {
				if (m_readyState == MODEL_LOAD_ERROR)
					return;
				DevMsg(DEVMSG_CORE, "Loading motions for %s, state: %d\n", GetName());

				LoadMotionPackages();
			});
			loadMotionsJob->DeleteOnFinish();
			loadMotionsJob->InitJob();
			loadMotionsJob->AddWait(loadModelJob);
			finishJob->AddWait(loadMotionsJob);

			jobMng->StartJob(loadMotionsJob);
		}

		jobMng->StartJob(loadModelJob);
		jobMng->StartJob(loadGeomJob);
		jobMng->StartJob(finishJob);

		return true;
	}

	// single-thread version
	LoadMaterials();
	
	if (!LoadGenerateVertexBuffer())
	{
		DestroyModel();
		return false;
	}

	LoadSetupBones();
	LoadMotionPackages();
	LoadPhysicsData();
	
	Atomic::Exchange(m_readyState, MODEL_LOAD_OK);

	return true;
}

bool CEqStudioGeom::LoadFromFile()
{
	studioHdr_t* pHdr = Studio_LoadModel(m_name.ToCString());

	if (!pHdr)
		return false; // get out, nothing to load

	m_studio = pHdr;

	return true;
}

bool CEqStudioGeom::LoadGenerateVertexBuffer()
{
	const studioHdr_t* studio = m_studio;

	// use index size
	int numVertices = 0;
	int numIndices = 0;

	int allVertexFeatureFlags = 0;

	// find vert and index count
	for (int i = 0; i < studio->numMeshGroups; i++)
	{
		const studioMeshGroupDesc_t* pMeshGroupDesc = studio->pMeshGroupDesc(i);
		for (int j = 0; j < pMeshGroupDesc->numMeshes; j++)
		{
			const studioMeshDesc_t* pMesh = pMeshGroupDesc->pMesh(j);
			numVertices += pMesh->numVertices;
			numIndices += pMesh->numIndices;

			allVertexFeatureFlags |= pMesh->vertexType;
		}
	}

	const int indexSize = numVertices > int(USHRT_MAX) ? sizeof(int) : sizeof(short);

	// FIXME: separate per each mesh since each mesh has different features?
	EGFHwVertex::PositionUV* allPositionUvsList = nullptr;
	EGFHwVertex::TBN* allTbnList = nullptr;
	EGFHwVertex::BoneWeights* allBoneWeightsList = nullptr;
	EGFHwVertex::Color* allColorList = nullptr;

	if(allVertexFeatureFlags & STUDIO_VERTFLAG_POS_UV)
		allPositionUvsList = PPNew EGFHwVertex::PositionUV[numVertices];

	if (allVertexFeatureFlags & STUDIO_VERTFLAG_TBN)
		allTbnList = PPNew EGFHwVertex::TBN[numVertices];

	if (allVertexFeatureFlags & STUDIO_VERTFLAG_BONEWEIGHT)
		allBoneWeightsList = PPNew EGFHwVertex::BoneWeights[numVertices];

	if (allVertexFeatureFlags & STUDIO_VERTFLAG_COLOR)
		allColorList = PPNew EGFHwVertex::Color[numVertices];

	ubyte* allIndices = PPNew ubyte[indexSize * numIndices + 4];

	numVertices = 0;
	numIndices = 0;

	static const EPrimTopology s_egfPrimTypeMap[] = {
		PRIM_TRIANGLES, 
		PRIM_TRIANGLE_STRIP, // was fan, now placeholder
		PRIM_TRIANGLE_STRIP,
	};

	m_hwGeomRefs = PPNewArrayRef(HWGeomRef, studio->numMeshGroups);
	for (int i = 0; i < studio->numMeshGroups; i++)
	{
		const studioMeshGroupDesc_t* pMeshGroupDesc = studio->pMeshGroupDesc(i);
		ArrayRef<HWGeomRef::MeshRef> meshRefs = PPNewArrayRef(HWGeomRef::MeshRef, pMeshGroupDesc->numMeshes);
		
		for (int j = 0; j < pMeshGroupDesc->numMeshes; j++)
		{
			// add vertices, add indices
			const studioMeshDesc_t* pMeshDesc = pMeshGroupDesc->pMesh(j);
			HWGeomRef::MeshRef& meshRef = meshRefs[j];

			meshRef.firstIndex = numIndices;
			meshRef.indexCount = pMeshDesc->numIndices;
			meshRef.primType = s_egfPrimTypeMap[pMeshDesc->primitiveType];
			meshRef.materialIdx = pMeshDesc->materialIndex;
			meshRef.supportsSkinning = (pMeshDesc->vertexType & STUDIO_VERTFLAG_BONEWEIGHT);

			const int newOffset = numVertices;

			numVertices += CopyGroupVertexDataToHWList(
				allPositionUvsList,
				allTbnList, 
				allBoneWeightsList,
				allColorList,
				numVertices, 
				pMeshDesc, m_boundingBox);
			numIndices += CopyGroupIndexDataToHWList(allIndices, indexSize, numIndices, pMeshDesc, newOffset);
		}

		m_hwGeomRefs[i].meshRefs = meshRefs;
	}

	// create hardware buffers
	if(allPositionUvsList)
		m_vertexBuffers[EGFHwVertex::VERT_POS_UV] = g_renderAPI->CreateBuffer({ allPositionUvsList, numVertices }, BUFFERUSAGE_VERTEX, "EGFPosUVBuf");
	if(allTbnList)
		m_vertexBuffers[EGFHwVertex::VERT_TBN] = g_renderAPI->CreateBuffer({ allTbnList, numVertices }, BUFFERUSAGE_VERTEX, "EGFTBNBuf");
	if(allBoneWeightsList)
		m_vertexBuffers[EGFHwVertex::VERT_BONEWEIGHT] = g_renderAPI->CreateBuffer({ allBoneWeightsList, numVertices }, BUFFERUSAGE_VERTEX, "EGFBoneWBuf");
	if(allColorList)
		m_vertexBuffers[EGFHwVertex::VERT_COLOR] = g_renderAPI->CreateBuffer({ allColorList, numVertices }, BUFFERUSAGE_VERTEX, "EGFColBuf");

	m_indexBuffer = g_renderAPI->CreateBuffer(BufferInfo(allIndices, indexSize, numIndices), BUFFERUSAGE_INDEX, "EGFIdxBuffer");
	m_indexFmt = (indexSize == 2) ? INDEXFMT_UINT16 : INDEXFMT_UINT32;

	// if we using software skinning, we need to create temporary vertices
#if 0
	if (m_forceSoftwareSkinning)
	{
		m_softwareVerts = PPAllocStructArray(EGFHwVertex, numVertices);
		memcpy(m_softwareVerts, allVerts, sizeof(EGFHwVertex) * numVertices);
	}
#endif

	// done.
	delete[] allPositionUvsList;
	delete[] allTbnList;
	delete[] allBoneWeightsList;
	delete[] allColorList;

	delete[] allIndices;

	return true;
}

void CEqStudioGeom::AddMotionPackage(const char* filename)
{
	if (m_readyState == MODEL_LOAD_ERROR)
		return;

	if (m_readyState != MODEL_LOAD_OK)
	{
		m_additionalMotionPackages.append(filename);
		return;
	}

	const int motionDataIdx = g_studioCache->PrecacheMotionData(filename);
	if (motionDataIdx == STUDIOCACHE_INVALID_IDX)
		return;

	m_motionData.append(motionDataIdx);
}

void CEqStudioGeom::LoadMotionPackages()
{
	// Try load default motion file
	{
		const int motionDataIdx = g_studioCache->PrecacheMotionData(fnmPathApplyExt(m_name, s_egfMotionPackageExt));
		if (motionDataIdx != STUDIOCACHE_INVALID_IDX)
			m_motionData.append(motionDataIdx);
	}

	const studioHdr_t* studio = m_studio;

	// load motion packages that were specified in EGF file
	for (int i = 0; i < studio->numMotionPackages; i++)
	{
		EqString mopPath;
		fnmPathCombine(mopPath, fnmPathStripName(m_name), fnmPathApplyExt(studio->pPackage(i)->packageName, s_egfMotionPackageExt));

		const int motionDataIdx = g_studioCache->PrecacheMotionData(mopPath, m_name);
		if (motionDataIdx != STUDIOCACHE_INVALID_IDX)
			m_motionData.append(motionDataIdx);
	}

	// load additional external motion packages requested by user
	for (const EqString& extraMotionPackName : m_additionalMotionPackages)
	{
		const int motionDataIdx = g_studioCache->PrecacheMotionData(extraMotionPackName, m_name);
		if (motionDataIdx != STUDIOCACHE_INVALID_IDX)
			m_motionData.append(motionDataIdx);
	}
	m_additionalMotionPackages.clear(true);
}

// loads materials for studio
void CEqStudioGeom::LoadMaterials()
{
	studioHdr_t* studio = m_studio;

	bool bError = false;
	{
		// init materials
		const int numMaterials = studio->numMaterials;
		m_materials.setNum(numMaterials);

		// try load materials properly
		// this is a source engine - like material loading using material paths
		for (int i = 0; i < numMaterials; i++)
		{
			EqString fpath(studio->pMaterial(i)->materialname);
			fnmPathFixSeparators(fpath);

			for (int j = 0; j < studio->numMaterialSearchPaths; j++)
			{
				if (m_materials[i])
					continue;

				EqString spath(studio->pMaterialSearchPath(j)->searchPath);
				fnmPathFixSeparators(spath);

				if (spath.Length() && spath[spath.Length() - 1] == CORRECT_PATH_SEPARATOR)
					spath = spath.Left(spath.Length() - 1);

				EqString extend_path;
				fnmPathCombine(extend_path, spath.ToCString(), fpath.ToCString());

				if (!g_matSystem->IsMaterialExist(extend_path))
					continue;

				IMaterialPtr material = g_matSystem->GetMaterial(extend_path, s_studioInstanceFormatId);
				g_matSystem->QueueLoading(material);

				if (!material->IsError() && !(material->GetFlags() & MATERIAL_FLAG_SKINNED))
					MsgWarning("Warning! Material '%s' shader '%s' for model '%s' is invalid\n", material->GetName(), material->GetShaderName(), m_name.ToCString());

				m_materials[i] = material;
			}
		}

		// false-initialization of non-loaded materials
		for (int i = 0; i < numMaterials; i++)
		{
			if (m_materials[i])
				continue;

			m_materials[i] = g_studioCache->GetErrorMaterial();

			const char* materialName = studio->pMaterial(i)->materialname;
			if (*materialName)
				MsgError("Couldn't load model material '%s' for '%s'\n", materialName, m_name.ToCString());
			else
				MsgError("Model %s has empty material name\n", m_name.ToCString());

			bError = true;
		}
	}

	int maxMaterialIdx = -1;

	for (int i = 0; i < studio->numMeshGroups; i++)
	{
		studioMeshGroupDesc_t* pMeshGroupDesc = studio->pMeshGroupDesc(i);

		for (int j = 0; j < pMeshGroupDesc->numMeshes; j++)
		{
			// add vertices, add indices
			studioMeshDesc_t* pMesh = pMeshGroupDesc->pMesh(j);

			if (pMesh->materialIndex > maxMaterialIdx)
				maxMaterialIdx = pMesh->materialIndex;
		}
	}
	
	const int numUsedMaterials = maxMaterialIdx + 1;

	m_materialCount = numUsedMaterials;
	m_materialGroupsCount = numUsedMaterials ? m_materials.numElem() / numUsedMaterials : 0;

	if (bError)
	{
		MsgError("  In following search paths:");

		for (int i = 0; i < studio->numMaterialSearchPaths; i++)
			MsgError("   '%s'\n", studio->pMaterialSearchPath(i)->searchPath);
	}
}

const IMaterialPtr& CEqStudioGeom::GetMaterial(int materialIdx, int materialGroupIdx) const
{
	if (materialIdx == -1)
		return g_matSystem->GetDefaultMaterial();

	materialGroupIdx = clamp(materialGroupIdx, 0, m_materialGroupsCount - 1);
	return m_materials[m_materialCount * materialGroupIdx + materialIdx];
}

ArrayCRef<IMaterialPtr>	CEqStudioGeom::GetMaterials(int materialGroupIdx) const
{
	materialGroupIdx = clamp(materialGroupIdx, 0, m_materialGroupsCount - 1);
	return ArrayCRef(&m_materials[m_materialCount * materialGroupIdx], m_materialCount);
}

void CEqStudioGeom::LoadSetupBones()
{
	studioHdr_t* studio = m_studio;

	// Initialize HW data joints
	m_joints = PPNew StudioJoint[studio->numBones];

	// parse bones
	for (int i = 0; i < studio->numBones; ++i)
	{
		// copy all bone data
		const studioBoneDesc_t* bone = studio->pBone(i);

		StudioJoint& joint = m_joints[i];
		joint.bone = bone;

		// setup transformation
		joint.localTrans = identity4;
		joint.localTrans.setRotation(bone->rotation);
		joint.localTrans.setTranslation(bone->position);

		joint.parent = bone->parent;
		joint.boneId = i;
	}

	// link joints and setup transforms
	for (int i = 0; i < studio->numBones; ++i)
	{
		StudioJoint& joint = m_joints[i];
		const int parent_index = joint.parent;

		if (parent_index != -1)
			m_joints[parent_index].childs.append(i);

		if (joint.parent != -1)
			joint.absTrans = joint.localTrans * m_joints[joint.parent].absTrans;
		else
			joint.absTrans = joint.localTrans;

		joint.invAbsTrans = !joint.absTrans;
	}

	// init ik chain links
	for (int i = 0; i < studio->numIKChains; ++i)
	{
		const studioIkChain_t* ikChain = studio->pIkChain(i);
		for (int j = 0; j < ikChain->numLinks; ++j)
		{
			studioIkLink_t* ikLink = ikChain->pLink(j);
			m_joints[ikLink->bone].ikChainId = i;
			m_joints[ikLink->bone].ikLinkId = j;
		}
	}
}

int CEqStudioGeom::SelectLod(float distance) const
{
	if (r_egf_LodTest.GetInt() != -1)
		return r_egf_LodTest.GetInt();

	const int numLods = m_studio->numLodParams;

	if (numLods < 2)
		return 0;

	int idealLOD = 0;

	for (int i = r_egf_LodStart.GetInt(); i < numLods; i++)
	{
		const studioLodParams_t* lodParam = m_studio->pLodParams(i);
		if (lodParam->flags & STUDIO_LOD_FLAG_MANUAL)
			continue;

		if (distance > lodParam->distance * r_egf_LodScale.GetFloat())
			idealLOD = i;
	}

	return idealLOD;
}

int CEqStudioGeom::FindManualLod(float value) const
{
	const int numLods = m_studio->numLodParams;

	for (int i = 0; i < numLods; i++)
	{
		const studioLodParams_t* lodParam = m_studio->pLodParams(i);
		if (!(lodParam->flags & STUDIO_LOD_FLAG_MANUAL))
			continue;

		if (fsimilar(lodParam->distance, value))
		{
			return i;
			break;
		}
	}

	return -1;
}

IGPUBufferPtr CEqStudioGeom::GetVertexBuffer(EGFHwVertex::VertexStreamId vertStream) const
{
	return m_vertexBuffers[vertStream];
}

void CEqStudioGeom::Draw(const DrawProps& drawProperties, const MeshInstanceData& instData, const RenderPassContext& passContext) const
{
	if (!drawProperties.bodyGroupFlags)
		return;

	RenderDrawCmd drawCmd;
	drawCmd.SetInstanceFormat(drawProperties.vertexFormat ? drawProperties.vertexFormat : g_studioCache->GetGeomVertexFormat())
		.SetInstanceData(instData)
		.SetIndexBuffer(m_indexBuffer, static_cast<EIndexFormat>(m_indexFmt));

	const bool isSkinned = [&]() -> bool{
		if (!m_vertexBuffers[EGFHwVertex::VERT_BONEWEIGHT])
			return false;

		if (!drawProperties.boneTransforms)
			return false;

		const studioHdr_t& studio = GetStudioHdr();
		const int numBones = studio.numBones;
		ASSERT_MSG(drawProperties.boneTransforms.size >= numBones * sizeof(RenderBoneTransform), "Bones buffer size %d while required %d", drawProperties.boneTransforms.size, numBones * sizeof(RenderBoneTransform));

		return true;
	}();

	MeshInstanceFormatRef& meshInstFormat = drawCmd.instanceInfo.instFormat;

	if (isSkinned)
	{
		// TODO: remove - use instance storage instead
		drawCmd.AddUniformBufferView(RenderBoneTransformID, drawProperties.boneTransforms);

		// HACK: This is a temporary hack until we get proper identification
		// or maybe hardware skinning using Compute shaders
		meshInstFormat.formatId = StringToHash(EqString(meshInstFormat.name) + "Skinned");
	}

	// setup vertex buffers
	{
		uint setVertStreams = 0;
		int numBitsSet = 0;
		
		ArrayCRef<VertexLayoutDesc> layoutDescList = meshInstFormat.layout;
		for (int i = 0; i < layoutDescList.numElem(); ++i)
		{
			const VertexLayoutDesc& streamLayout = layoutDescList[i];
			if (numBitsSet == EGFHwVertex::VERT_COUNT)
				break;

			if (streamLayout.stepMode == VERTEX_STEPMODE_INSTANCE && drawCmd.instanceInfo.instData.buffer)
			{
				setVertStreams |= (1 << i);
				continue;
			}

			if (streamLayout.userId & EGFHwVertex::EGF_FLAG)
			{
				const EGFHwVertex::VertexStreamId vertStreamId = static_cast<EGFHwVertex::VertexStreamId>(streamLayout.userId & EGFHwVertex::EGF_MASK);
				if (!m_vertexBuffers[vertStreamId])
					continue;

				if (setVertStreams & (1 << vertStreamId))
					continue;

				drawCmd.SetVertexBuffer(i, m_vertexBuffers[vertStreamId]);
				setVertStreams |= (1 << int(vertStreamId));
				++numBitsSet;
			}
		}

		meshInstFormat.usedLayoutBits &= ~7;
		meshInstFormat.usedLayoutBits |= setVertStreams;
	}

	if (drawProperties.setupDrawCmd)
		drawProperties.setupDrawCmd(drawCmd);

	const studioHdr_t& studio = *m_studio;
	for (int i = 0; i < studio.numBodyGroups; ++i)
	{
		// check bodygroups for rendering
		if (!(drawProperties.bodyGroupFlags & (1 << i)))
			continue;

		const int bodyGroupLodIndex = studio.pBodyGroups(i)->lodModelIndex;
		const studioLodModel_t* lodModel = studio.pLodModel(bodyGroupLodIndex);

		// get the right LOD model number
		int bodyGroupLOD = drawProperties.lod;
		uint8 modelDescId = EGF_INVALID_IDX;
		do
		{
			modelDescId = lodModel->modelsIndexes[bodyGroupLOD];
			bodyGroupLOD--;
		} while (modelDescId == EGF_INVALID_IDX && bodyGroupLOD >= 0);

		if (modelDescId == EGF_INVALID_IDX)
			continue;

		const studioMeshGroupDesc_t* modDesc = studio.pMeshGroupDesc(modelDescId);

		// render model groups that in this body group
		for (int j = 0; j < modDesc->numMeshes; ++j)
		{
			const int materialIndex = modDesc->pMesh(j)->materialIndex;

			IMaterial* material = GetMaterial(materialIndex, drawProperties.materialGroup);
			const int materialFlags = material->GetFlags();

			const int materialFlagsMask = materialFlags & drawProperties.materialFlags;
			if (drawProperties.excludeMaterialFlags && materialFlagsMask > 0)
				continue;
			else if (materialFlags && !drawProperties.excludeMaterialFlags && !materialFlagsMask)
				continue;

			if (drawProperties.setupBodyGroup)
				drawProperties.setupBodyGroup(drawCmd, material, i, j);

			if (!drawProperties.skipMaterials)
				drawCmd.SetMaterial(material);

			const HWGeomRef::MeshRef& meshRef = m_hwGeomRefs[modelDescId].meshRefs[j];
			drawCmd.SetDrawIndexed(static_cast<EPrimTopology>(meshRef.primType), meshRef.indexCount, meshRef.firstIndex);

			g_matSystem->SetupDrawCommand(drawCmd, passContext);
		}
	}
}

const BoundingBox& CEqStudioGeom::GetBoundingBox() const
{
	while (!m_studio && GetLoadingState() == MODEL_LOAD_IN_PROGRESS) // wait for hwdata
		Platform_Sleep(1);

	return m_boundingBox;
}

const char* CEqStudioGeom::GetName() const
{
	return m_name.ToCString();
}

EModelLoadingState CEqStudioGeom::GetLoadingState() const
{
	return static_cast<EModelLoadingState>(m_readyState);
}

Future<bool> CEqStudioGeom::GetLoadingFuture() const
{
	if (m_readyState != MODEL_LOAD_IN_PROGRESS)
		return Future<bool>::Succeed(m_readyState == MODEL_LOAD_OK);

	return m_loadingFuture;
}

const studioHdr_t& CEqStudioGeom::GetStudioHdr() const
{
	while (!m_studio && GetLoadingState() == MODEL_LOAD_IN_PROGRESS) // wait for hwdata
		Platform_Sleep(1);

	return *m_studio; 
}

const StudioPhysData& CEqStudioGeom::GetPhysData() const 
{
	while (!m_studio && GetLoadingState() == MODEL_LOAD_IN_PROGRESS) // wait for hwdata
		Platform_Sleep(1);

	return m_physModel;
}

ArrayCRef<int> CEqStudioGeom::GetMotionDataIdxs() const
{
	while (!m_studio && GetLoadingState() == MODEL_LOAD_IN_PROGRESS) // wait for hwdata
		Platform_Sleep(1);

	return m_motionData;
}

ArrayCRef<StudioJoint> CEqStudioGeom::GetJoints() const
{
	return ArrayCRef(m_joints, m_studio->numBones);
}

// instancing
void CEqStudioGeom::SetInstancer(CBaseEqGeomInstancer* instancer)
{
	m_instancer = instancer;

	if (m_instancer)
		m_instancer->ValidateAssert();
}

CBaseEqGeomInstancer* CEqStudioGeom::GetInstancer() const
{
	return m_instancer;
}

static void MakeDecalTexCoord(Array<EGFHwVertex>& verts, Array<int>& indices, const DecalMakeInfo& info)
{
	ASSERT_FAIL("Not implemented");
#if 0
	const Vector3D decalSize = info.size * 2.0f;

	if (info.flags & DECAL_MAKE_FLAG_TEX_NORMAL)
	{
		int texSizeW = 1;
		int texSizeH = 1;

		ITexturePtr pTex = info.material->GetBaseTexture();

		if (pTex)
		{
			texSizeW = pTex->GetWidth();
			texSizeH = pTex->GetHeight();
		}

		Vector3D uaxis;
		Vector3D vaxis;

		Vector3D axisAngles = VectorAngles(info.normal);
		AngleVectors(axisAngles, nullptr, &uaxis, &vaxis);

		vaxis *= -1;

		//VectorVectors(m_surftex.Plane.normal, m_surftex.UAxis.normal, m_surftex.VAxis.normal);

		Matrix3x3 texMatrix(uaxis, vaxis, info.normal);

		const Matrix3x3 rotationMat = rotateZXY3(0.0f, 0.0f, DEG2RAD(info.texRotation));
		texMatrix = rotationMat * texMatrix;

		uaxis = texMatrix.rows[0];
		vaxis = texMatrix.rows[1];

		for (int i = 0; i < verts.numElem(); i++)
		{
			const float one_over_w = 1.0f / fabs(dot(decalSize, uaxis * uaxis) * info.texScale.x);
			const float one_over_h = 1.0f / fabs(dot(decalSize, vaxis * vaxis) * info.texScale.y);

			const float U = dot(uaxis, info.origin - verts[i].pos.xyz()) * one_over_w + 0.5f;
			const float V = dot(vaxis, info.origin - verts[i].pos.xyz()) * one_over_h - 0.5f;

			verts[i].texcoord.x = U;
			verts[i].texcoord.y = -V;
		}
	}
	else
	{
		// generate new texture coordinates
		for (int i = 0; i < verts.numElem(); i++)
		{
			verts[i].tangent = fastNormalize(verts[i].tangent);
			verts[i].binormal = fastNormalize(verts[i].binormal);
			verts[i].normal = fastNormalize(verts[i].normal);

			const float one_over_w = 1.0f / fabs(dot(decalSize, Vector3D(verts[i].tangent)));
			const float one_over_h = 1.0f / fabs(dot(decalSize, Vector3D(verts[i].binormal)));

			const Vector3D t(verts[i].tangent);
			const Vector3D b(verts[i].binormal);

			verts[i].texcoord.x = fabs(dot(info.origin - verts[i].pos.xyz(), t * sign(t)) * one_over_w + 0.5f);
			verts[i].texcoord.y = fabs(dot(info.origin - verts[i].pos.xyz(), b * sign(b)) * one_over_h + 0.5f);
		}
	}

	// it needs TBN refreshing
	for (int i = 0; i < indices.numElem(); i += 3)
	{
		const int idxs[3] = { indices[i], indices[i + 1], indices[i + 2] };

		Vector3D t, b, n;

		ComputeTriangleTBN(
			Vector3D(verts[idxs[0]].pos.xyz()),
			Vector3D(verts[idxs[1]].pos.xyz()),
			Vector3D(verts[idxs[2]].pos.xyz()),
			verts[idxs[0]].texcoord,
			verts[idxs[1]].texcoord,
			verts[idxs[2]].texcoord,
			n,
			t,
			b);

		verts[idxs[0]].tangent = t;
		verts[idxs[0]].binormal = b;
		verts[idxs[1]].tangent = t;
		verts[idxs[1]].binormal = b;
		verts[idxs[2]].tangent = t;
		verts[idxs[2]].binormal = b;
	}
#endif
}

// makes dynamic temporary decal
CRefPtr<DecalData> CEqStudioGeom::MakeDecal(const DecalMakeInfo& info, Matrix4x4* jointMatrices, int bodyGroupFlags, int lod) const
{
	ASSERT_FAIL("Not implemented");
#if 0
	if (r_egf_NoTempDecals.GetBool())
		return nullptr;

	const studioHdr_t* studio = m_studio;

	Vector3D decal_origin = info.origin;
	Vector3D decal_normal = info.normal;

	Array<EGFHwVertex> verts(PP_SL);
	Array<int> indices(PP_SL);

	Matrix4x4 tempMatrixArray[128];

	if (jointMatrices)
	{
		// Send all matrices as 4x3
		for (int i = 0; i < studio->numBones; i++)
		{
			// FIXME: kind of slowness
			tempMatrixArray[i] = (!m_joints[i].absTrans * jointMatrices[i]);
		}
	}

	BoundingBox bbox(decal_origin - info.size, decal_origin + info.size);

	// we should transform every vertex and find an intersecting ones
	for (int i = 0; i < studio->numBodyGroups; i++)
	{
		if (!(bodyGroupFlags & (1 << i)))
			continue;

		const int bodyGroupLodIndex = studio->pBodyGroups(i)->lodModelIndex;
		const studioLodModel_t* lodModel = studio->pLodModel(bodyGroupLodIndex);

		// get the right LOD model number
		int bodyGroupLOD = lod;
		uint8 modelDescId = EGF_INVALID_IDX;
		do
		{
			modelDescId = lodModel->modelsIndexes[bodyGroupLOD];
			bodyGroupLOD--;
		} while (modelDescId == EGF_INVALID_IDX && bodyGroupLOD >= 0);

		if (modelDescId == EGF_INVALID_IDX)
			continue;

		const studioMeshGroupDesc_t* modDesc = studio->pMeshGroupDesc(modelDescId);

		Array<EGFHwVertex> g_verts(PP_SL);
		Array<int> g_indices(PP_SL);
		Array<int> g_orig_indices(PP_SL);

		for (int j = 0; j < modDesc->numMeshes; j++)
		{
			const studioMeshDesc_t* pMesh = modDesc->pMesh(j);

			const uint32* pIndices = pMesh->pVertexIdx(0);

			g_verts.clear();
			g_indices.clear();
			g_orig_indices.clear();

			g_verts.reserve(pMesh->numIndices);
			g_indices.reserve(pMesh->numIndices);
			g_orig_indices.reserve(pMesh->numIndices);

			const int numIndices = (pMesh->primitiveType == STUDIO_PRIM_TRI_STRIP) ? pMesh->numIndices - 2 : pMesh->numIndices;
			const int indexStep = (pMesh->primitiveType == STUDIO_PRIM_TRI_STRIP) ? 1 : 3;

			for (int k = 0; k < numIndices; k += indexStep)
			{
				// skip strip degenerates
				if (pIndices[k] == pIndices[k + 1] || pIndices[k] == pIndices[k + 2] || pIndices[k + 1] == pIndices[k + 2])
					continue;

				const int even = k % 2;	// handle flipped triangles on STUDIO_PRIM_TRI_STRIP
				
				int i0, i1, i2;
				if (even && pMesh->primitiveType == STUDIO_PRIM_TRI_STRIP)
				{
					i0 = pIndices[k + 2];
					i1 = pIndices[k + 1];
					i2 = pIndices[k];
				}
				else
				{
					i0 = pIndices[k];
					i1 = pIndices[k + 1];
					i2 = pIndices[k + 2];
				}

				EGFHwVertex v0(*pMesh->pVertex(i0));
				EGFHwVertex v1(*pMesh->pVertex(i1));
				EGFHwVertex v2(*pMesh->pVertex(i2));

				// for skinning we transform vertices but we add raw verts to decal later
				if (jointMatrices)
				{
					TransformEGFVertex(v0, tempMatrixArray);
					TransformEGFVertex(v1, tempMatrixArray);
					TransformEGFVertex(v2, tempMatrixArray);
				}

				BoundingBox vbox;
				vbox.AddVertex(v0.pos.xyz());
				vbox.AddVertex(v1.pos.xyz());
				vbox.AddVertex(v2.pos.xyz());

				// make and check surface normal
				const Vector3D normal = (v0.normal + v1.normal + v2.normal) / 3.0f;

				auto egf_vertex_comp = [](const EGFHwVertex& a, const EGFHwVertex& b) -> bool
				{
					return (a.pos == b.pos) && (a.texcoord == b.texcoord);
				};

				if (bbox.Intersects(vbox) && dot(normal, decal_normal) < 0)
				{
					// TODO: optimize using Map and HashVector3D
					g_indices.append(g_verts.addUnique(v0, egf_vertex_comp));
					g_indices.append(g_verts.addUnique(v1, egf_vertex_comp));
					g_indices.append(g_verts.addUnique(v2, egf_vertex_comp));

					g_orig_indices.append(pIndices[k]);
					g_orig_indices.append(pIndices[k + 1]);
					g_orig_indices.append(pIndices[k + 2]);
				}
			}

			MakeDecalTexCoord(g_verts, g_indices, info);

			const int start = verts.numElem();

			verts.resize(g_indices.numElem());
			indices.resize(g_indices.numElem());

			for (int k = 0; k < g_indices.numElem(); k++)
			{
				indices.append(start + g_indices[k]);

				// restore vertex position in case of skinning
				g_verts[g_indices[k]].pos = Vector4D(pMesh->pVertex(g_orig_indices[k])->point, 1.0f);
			}

			verts.append(g_verts);
		}
	}

	if (verts.numElem() && indices.numElem())
	{
		CRefPtr<DecalData> decal = CRefPtr_new(DecalData);

		decal->material = info.material;
		decal->flags = DECAL_FLAG_STUDIODECAL;
		decal->numVerts = verts.numElem();
		decal->numIndices = indices.numElem();
		decal->verts = PPAllocStructArray(EGFHwVertex, decal->numVerts);
		decal->indices = PPAllocStructArray(uint16, decal->numIndices);

		// copy geometry
		memcpy(decal->verts, verts.ptr(), sizeof(EGFHwVertex) * decal->numVerts);

		for (int i = 0; i < decal->numIndices; i++)
			decal->indices[i] = indices[i];

		return decal;
	}

#endif
	return nullptr;
}


float CEqStudioGeom::CheckIntersectionWithRay(const Vector3D& rayStart, const Vector3D& rayDir, int bodyGroupFlags, int lod) const
{
	float f1, f2;
	if(!m_boundingBox.IntersectsRay(rayStart, rayDir, f1, f2))
		return F_INFINITY;

	float best_dist = F_INFINITY;

	const studioHdr_t& studio = *m_studio;

	for(int i = 0; i < studio.numBodyGroups; i++)
	{
		if (!(bodyGroupFlags & (1 << i)))
			continue;

		const int bodyGroupLodIndex = studio.pBodyGroups(i)->lodModelIndex;
		const studioLodModel_t* lodModel = studio.pLodModel(bodyGroupLodIndex);

		// get the right LOD model number
		int bodyGroupLOD = lod;
		uint8 modelDescId = EGF_INVALID_IDX;
		do
		{
			modelDescId = lodModel->modelsIndexes[bodyGroupLOD];
			bodyGroupLOD--;
		} while (modelDescId == EGF_INVALID_IDX && bodyGroupLOD >= 0);

		if (modelDescId == EGF_INVALID_IDX)
			continue;

		const studioMeshGroupDesc_t* modDesc = studio.pMeshGroupDesc(modelDescId);

		for(int j = 0; j < modDesc->numMeshes; j++)
		{
			const studioMeshDesc_t* pMesh = modDesc->pMesh(j);

			const uint32* pIndices = pMesh->pVertexIdx(0);

			const int numIndices = (pMesh->primitiveType == STUDIO_PRIM_TRI_STRIP) ? pMesh->numIndices - 2 : pMesh->numIndices;
			const int indexStep = (pMesh->primitiveType == STUDIO_PRIM_TRI_STRIP) ? 1 : 3;

			for (int k = 0; k < numIndices; k += indexStep)
			{
				// skip strip degenerates
				if (pIndices[k] == pIndices[k+1] || pIndices[k] == pIndices[k+2] || pIndices[k+1] == pIndices[k+2])
					continue;

				const int even = k % 2; // handle flipped triangles on STUDIO_PRIM_TRI_STRIP

				Vector3D v0, v1, v2;
				if (even && pMesh->primitiveType == STUDIO_PRIM_TRI_STRIP)
				{
					v0 = pMesh->pPosUvs(pIndices[k + 2])->point;
					v1 = pMesh->pPosUvs(pIndices[k + 1])->point;
					v2 = pMesh->pPosUvs(pIndices[k])->point;
				}
				else
				{
					v0 = pMesh->pPosUvs(pIndices[k])->point;
					v1 = pMesh->pPosUvs(pIndices[k + 1])->point;
					v2 = pMesh->pPosUvs(pIndices[k + 2])->point;
				}

				float dist = F_INFINITY;
				if(IsRayIntersectsTriangle(v0,v1,v2, rayStart, rayDir, dist, true))
				{
					if(dist < best_dist && dist > 0)
						best_dist = dist;
				}
			}
		}
	}

	return best_dist;
}