//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Studio Geometry Form
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IEqParallelJobs.h"
#include "core/ConVar.h"

#include "math/Utility.h"

#include "studiofile/StudioLoader.h"
#include "StudioGeom.h"
#include "StudioCache.h"
#include "StudioGeomInstancer.h"

#include "physics/IStudioShapeCache.h"
#include "render/Decals.h"
#include "materialsystem1/IMaterialSystem.h"

using namespace Threading;

EGFHwVertex::VertexStream s_defaultVertexStreamMappingDesc[] = {
	EGFHwVertex::VERT_POS_UV,
	EGFHwVertex::VERT_TBN,
	EGFHwVertex::VERT_BONEWEIGHT
};
ArrayCRef<EGFHwVertex::VertexStream> g_defaultVertexStreamMapping = { s_defaultVertexStreamMappingDesc ,  elementsOf(s_defaultVertexStreamMappingDesc) };

EGFHwVertex::PositionUV::PositionUV(const studioVertexDesc_t& initFrom)
{
	pos = Vector4D(initFrom.point, 1.0f);
	texcoord = initFrom.texCoord;
}

EGFHwVertex::TBN::TBN(const studioVertexDesc_t& initFrom)
{
	tangent = initFrom.tangent;
	binormal = initFrom.binormal;
	normal = initFrom.normal;
}

EGFHwVertex::BoneWeights::BoneWeights(const studioVertexDesc_t& initFrom)
{
	ASSERT(initFrom.boneweights.numweights <= MAX_MODEL_VERTEX_WEIGHTS);

	memset(boneWeights, 0, sizeof(boneWeights));
	for (int i = 0; i < MAX_MODEL_VERTEX_WEIGHTS; i++)
		boneIndices[i] = -1;

	for (int i = 0; i < min(initFrom.boneweights.numweights, MAX_MODEL_VERTEX_WEIGHTS); i++)
	{
		boneIndices[i] = initFrom.boneweights.bones[i];
		boneWeights[i] = initFrom.boneweights.weight[i];
	}
}

ArrayCRef<VertexFormatDesc_t> EGFHwVertex::PositionUV::GetVertexFormatDesc()
{
	static const VertexFormatDesc_t g_EGFVertexUvFormat[] = {
		{ VERT_POS_UV, 4, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_HALF, "position" },// position
		{ VERT_POS_UV, 2, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "texcoord" },// texcoord 0
	};
	return ArrayCRef(g_EGFVertexUvFormat, elementsOf(g_EGFVertexUvFormat));
}

ArrayCRef<VertexFormatDesc_t> EGFHwVertex::TBN::GetVertexFormatDesc()
{
	static const VertexFormatDesc_t g_EGFTBNFormat[] = {
		{ VERT_TBN, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "tangent" },	// Tangent (TC1)
		{ VERT_TBN, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "binormal" },	// Binormal (TC2)
		{ VERT_TBN, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "normal" },		// Normal (TC3)
	};
	return ArrayCRef(g_EGFTBNFormat, elementsOf(g_EGFTBNFormat));
}

ArrayCRef<VertexFormatDesc_t> EGFHwVertex::BoneWeights::GetVertexFormatDesc()
{
	static const VertexFormatDesc_t g_EGFBoneWeightsFormat[] = {
		{ VERT_BONEWEIGHT, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "boneid" },	// Bone indices (hw skinning), (TC4)
		{ VERT_BONEWEIGHT, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "bonew" }	// Bone weights (hw skinning), (TC5)
	};
	return ArrayCRef(g_EGFBoneWeightsFormat, elementsOf(g_EGFBoneWeightsFormat));
}

DECLARE_CVAR_CLAMP(r_egf_LodTest, "-1", -1.0f, MAX_MODEL_LODS, "Studio LOD test", CV_CHEAT);
DECLARE_CVAR(r_egf_NoTempDecals, "0", "Disables temp decals", CV_CHEAT);
DECLARE_CVAR(r_egf_LodScale, "1.0", "Studio model LOD scale", CV_ARCHIVE);
DECLARE_CVAR_CLAMP(r_egf_LodStart, "0", 0, MAX_MODEL_LODS, "Studio LOD start index", CV_ARCHIVE);
DECLARE_CVAR(r_force_softwareskinning, "0", "Force software skinning", CV_UNREGISTERED);

CEqStudioGeom::CEqStudioGeom()
{
}

CEqStudioGeom::~CEqStudioGeom()
{
	DestroyModel();
}

namespace {

// MUST BE SAME AS IN SHADER

// quaternion with posit
struct bonequaternion_t
{
	Vector4D	quat;
	Vector4D	origin;
};

// multiplies bone
Vector4D quatMul(Vector4D& q1, Vector4D& q2)
{
	Vector3D  im = q1.w * q2.xyz() + q1.xyz() * q2.w + cross(q1.xyz(), q2.xyz());
	Vector4D  dt = q1 * q2;
	float re = dot(dt, Vector4D(-1.0, -1.0, -1.0, 1.0));

	return Vector4D(im, re);
}

// vector rotation
Vector4D quatRotate(Vector3D& p, Vector4D& q)
{
	Quaternion temp = Quaternion(q) * Quaternion(0.0f, p.x, p.y, p.z);
	return (temp * Quaternion(q.w, -q.x, -q.y, -q.z)).asVector4D();
}

// transforms bone
Vector3D boneTransf(bonequaternion_t& bq, Vector3D& pos)
{
	return bq.origin.xyz() + quatRotate(pos, bq.quat).xyz();
}
}

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

		normal += transformVector(Vector3D(tbn.normal), pMatrices[boneIdx]) * weight;
		tangent += transformVector(Vector3D(tbn.tangent), pMatrices[boneIdx]) * weight;
		binormal += transformVector(Vector3D(tbn.binormal), pMatrices[boneIdx]) * weight;

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

static int ComputeQuaternionsForSkinning(const CEqStudioGeom* model, bonequaternion_t* bquats, Matrix4x4* btransforms)
{
	const studioHdr_t& studio = model->GetStudioHdr();
	const int numBones = studio.numBones;

	for (int i = 0; i < numBones; i++)
	{
		// FIXME: kind of slowness
		const Matrix4x4 toAbsTransform = (!model->GetJoint(i).absTrans * btransforms[i]);

		// cast matrices to quaternions
		// note that quaternions uses transposes matrix set.
		bquats[i].quat = Quaternion(transpose(toAbsTransform).getRotationComponent()).asVector4D();
		bquats[i].origin = Vector4D(toAbsTransform.rows[3].xyz(), 1);
	}

	return numBones * 2;
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
			m_vertexBuffer->Update(m_softwareVerts, m_vertexBuffer->GetVertexCount() * sizeof(EGFHwVertex), 0);
			m_skinningDirty = false;
		}

		bonequaternion_t bquats[128];
		const int numRegs = ComputeQuaternionsForSkinning(this, bquats, jointMatrices);
		g_pShaderAPI->SetShaderConstantArrayVector4D("Bones", (Vector4D*)&bquats[0].quat, numRegs);

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
		if (m_vertexBuffer->Lock(0, verticesCount, (void**)&bufferData, false))
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

	g_pShaderAPI->Reset(STATE_RESET_VBO);
	for (int i = 0; i < EGFHwVertex::VERT_COUNT; ++i)
	{
		g_pShaderAPI->DestroyVertexBuffer(m_vertexBuffers[i]);
		m_vertexBuffers[i] = nullptr;
	}
	g_pShaderAPI->DestroyIndexBuffer(m_indexBuffer);
	m_indexBuffer = nullptr;

	m_materials.clear(true);

	if (m_softwareVerts)
	{
		PPFree(m_softwareVerts);
		m_softwareVerts = nullptr;
	}
	m_skinningDirty = false;
	m_forceSoftwareSkinning = false;
	m_materialCount = 0;
	m_materialGroupsCount = 0;

	if (m_studio)
	{
		for (int i = 0; i < m_motionData.numElem(); i++)
		{
			Studio_FreeMotionData(m_motionData[i], m_studio->numBones);
			PPFree(m_motionData[i]);
		}
		m_motionData.clear();

		g_studioShapeCache->DestroyStudioCache(&m_physModel);
		Studio_FreePhysModel(&m_physModel);
		m_physModel = studioPhysData_t();

		for (int i = 0; i < m_studio->numMeshGroups; i++)
			delete[] m_hwGeomRefs[i].meshRefs;

		SAFE_DELETE_ARRAY(m_hwGeomRefs);
		SAFE_DELETE_ARRAY(m_joints);

		Studio_FreeModel(m_studio);
	}
}

void CEqStudioGeom::LoadPhysicsData()
{
	EqString podFileName = m_name.Path_Strip_Ext();
	podFileName.Append(".pod");

	if (Studio_LoadPhysModel(podFileName, &m_physModel))
	{
		DevMsg(DEVMSG_CORE, "Loaded physics object data '%s'\n", podFileName.ToCString());

		ASSERT_MSG(g_studioShapeCache, "studio shape cache is not initialized!\n");
		g_studioShapeCache->InitStudioCache(&m_physModel);
	}
}

static int CopyGroupVertexDataToHWList(EGFHwVertex::PositionUV* hwVertPosList, EGFHwVertex::TBN* hwVertTbnList, EGFHwVertex::BoneWeights* hwVertWeightList, int currentVertexCount, const studioMeshDesc_t* pMesh, BoundingBox& aabb)
{
	for (int32 i = 0; i < pMesh->numVertices; i++)
	{
		const studioVertexDesc_t* pVertex = pMesh->pVertex(i);

		hwVertPosList[currentVertexCount] = EGFHwVertex::PositionUV(*pVertex);
		hwVertTbnList[currentVertexCount] = EGFHwVertex::TBN(*pVertex);
		hwVertWeightList[currentVertexCount] = EGFHwVertex::BoneWeights(*pVertex);

		aabb.AddVertex(pVertex->point);
		++currentVertexCount;
	}

	return pMesh->numVertices;
}

static int CopyGroupIndexDataToHWList(void* indexData, int indexSize, int currentIndexCount, const studioMeshDesc_t* pMesh, int vertex_add_offset)
{
	if (indexSize == sizeof(int))
	{
		for (uint32 i = 0; i < pMesh->numIndices; i++)
		{
			const int index = (*pMesh->pVertexIdx(i)) + vertex_add_offset;
			uint32* indices = (uint32*)indexData;
			indices[currentIndexCount++] = index;
		}
	}
	else if (indexSize == sizeof(short))
	{
		for (uint32 i = 0; i < pMesh->numIndices; i++)
		{
			const int index = (*pMesh->pVertexIdx(i)) + vertex_add_offset;
			uint16* indices = (uint16*)indexData;
			indices[currentIndexCount++] = index;
		}
	}
	else
	{
		ASSERT_FAIL("CopyGroupIndexDataToHWList - Bad index size");
	}

	return pMesh->numIndices;
}

void CEqStudioGeom::LoadModelJob(void* data, int i)
{
	CEqStudioGeom* model = (CEqStudioGeom*)data;

	// multi-threaded version
	if (!model->LoadFromFile())
	{
		model->DestroyModel();
		return;
	}

	model->LoadMaterials();
	model->LoadPhysicsData();

	//g_parallelJobs->AddJob(JOB_TYPE_SPOOL_EGF, LoadPhysicsJob, data, 1, OnLoadingJobComplete);
	g_parallelJobs->AddJob(JOB_TYPE_SPOOL_EGF, LoadVertsJob, data, 1, OnLoadingJobComplete);
	g_parallelJobs->AddJob(JOB_TYPE_SPOOL_EGF, LoadMotionJob, data, 1, OnLoadingJobComplete);

	g_parallelJobs->Submit();
}

void CEqStudioGeom::LoadVertsJob(void* data, int i)
{
	CEqStudioGeom* model = (CEqStudioGeom*)data;
	
	if (model->m_readyState == MODEL_LOAD_ERROR)
		return;

	if (!model->LoadGenerateVertexBuffer())
		model->DestroyModel();
}

void CEqStudioGeom::LoadPhysicsJob(void* data, int i)
{
	CEqStudioGeom* model = (CEqStudioGeom*)data;

	if (model->m_readyState == MODEL_LOAD_ERROR)
		return;

	model->LoadPhysicsData();
}

void CEqStudioGeom::LoadMotionJob(void* data, int i)
{
	CEqStudioGeom* model = (CEqStudioGeom*)data;
	if (model->m_readyState == MODEL_LOAD_ERROR)
		return;

	model->LoadSetupBones();
	model->LoadMotionPackages();
}

void CEqStudioGeom::OnLoadingJobComplete(struct eqParallelJob_t* job)
{
	CEqStudioGeom* model = (CEqStudioGeom*)job->arguments;

	if (model->m_readyState == MODEL_LOAD_ERROR)
		return;
	
	if (Atomic::Decrement(model->m_loading) <= 0)
	{
		Atomic::Exchange(model->m_readyState, MODEL_LOAD_OK);
		//DevMsg(DEVMSG_CORE, "EGF loading completed\n");
	}
}

bool CEqStudioGeom::LoadModel(const char* pszPath, bool useJob)
{
	m_name = pszPath;
	m_name.Path_FixSlashes();

	// first we switch to loading
	Atomic::Exchange(m_readyState, MODEL_LOAD_IN_PROGRESS);

	if (useJob)
	{
		m_loading = 3;
		g_parallelJobs->AddJob(JOB_TYPE_SPOOL_EGF, LoadModelJob, this, 1, OnLoadingJobComplete);

		g_parallelJobs->Submit();
		return true;
	}

	// single-thread version

	if (!LoadFromFile())
	{
		DestroyModel();
		return false;
	}

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
	// detect and set the force software skinning flag
	m_forceSoftwareSkinning = r_force_softwareskinning.GetBool();

	const studioHdr_t* studio = m_studio;

	// use index size
	int numVertices = 0;
	int numIndices = 0;

	// find vert and index count
	for (int i = 0; i < studio->numMeshGroups; i++)
	{
		const studioMeshGroupDesc_t* pMeshGroupDesc = studio->pMeshGroupDesc(i);
		for (int j = 0; j < pMeshGroupDesc->numMeshes; j++)
		{
			const studioMeshDesc_t* pMesh = pMeshGroupDesc->pMesh(j);
			numVertices += pMesh->numVertices;
			numIndices += pMesh->numIndices;
		}
	}

	const int indexSize = numVertices > int(USHRT_MAX) ? sizeof(int) : sizeof(short);

	EGFHwVertex::PositionUV* allPositionUvsList = PPNew EGFHwVertex::PositionUV[numVertices];
	EGFHwVertex::TBN* allTbnList = PPNew EGFHwVertex::TBN[numVertices];
	EGFHwVertex::BoneWeights* allBoneWeightsList = PPNew EGFHwVertex::BoneWeights[numVertices];
	ubyte* allIndices = PPNew ubyte[indexSize * numIndices];

	numVertices = 0;
	numIndices = 0;

	m_hwGeomRefs = PPNew HWGeomRef[studio->numMeshGroups];
	for (int i = 0; i < studio->numMeshGroups; i++)
	{
		const studioMeshGroupDesc_t* pMeshGroupDesc = studio->pMeshGroupDesc(i);
		HWGeomRef::Mesh* meshRefs = PPNew HWGeomRef::Mesh[pMeshGroupDesc->numMeshes];
		m_hwGeomRefs[i].meshRefs = meshRefs;

		for (int j = 0; j < pMeshGroupDesc->numMeshes; j++)
		{
			// add vertices, add indices
			const studioMeshDesc_t* pMeshDesc = pMeshGroupDesc->pMesh(j);
			HWGeomRef::Mesh& meshRef = meshRefs[j];

			meshRef.firstIndex = numIndices;
			meshRef.indexCount = pMeshDesc->numIndices;
			meshRef.primType = pMeshDesc->primitiveType;

			const int newOffset = numVertices;

			numVertices += CopyGroupVertexDataToHWList(
				allPositionUvsList,
				allTbnList, 
				allBoneWeightsList, 
				numVertices, 
				pMeshDesc, m_boundingBox);
			numIndices += CopyGroupIndexDataToHWList(allIndices, indexSize, numIndices, pMeshDesc, newOffset);
		}
	}

	// create hardware buffers
	m_vertexBuffers[EGFHwVertex::VERT_POS_UV] = g_pShaderAPI->CreateVertexBuffer(BUFFER_STATIC, numVertices, sizeof(EGFHwVertex::PositionUV), allPositionUvsList);
	m_vertexBuffers[EGFHwVertex::VERT_TBN] = g_pShaderAPI->CreateVertexBuffer(BUFFER_STATIC, numVertices, sizeof(EGFHwVertex::TBN), allTbnList);
	m_vertexBuffers[EGFHwVertex::VERT_BONEWEIGHT] = g_pShaderAPI->CreateVertexBuffer(BUFFER_STATIC, numVertices, sizeof(EGFHwVertex::BoneWeights), allBoneWeightsList);
	m_indexBuffer = g_pShaderAPI->CreateIndexBuffer(numIndices, indexSize, BUFFER_STATIC, allIndices);

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
	delete[] allIndices;

	return true;
}

void CEqStudioGeom::LoadMotionPackage(const char* filename)
{
	if (m_readyState == MODEL_LOAD_ERROR)
		return;

	if (m_readyState != MODEL_LOAD_OK)
	{
		m_additionalMotionPackages.append(filename);
		return;
	}

	studioMotionData_t* motionData = Studio_LoadMotionData(filename, m_studio->numBones);
	if (motionData)
		m_motionData.append(motionData);
	else
		MsgError("Can't open motion data package '%s'!\n", filename);
}

void CEqStudioGeom::LoadMotionPackages()
{
	const studioHdr_t* studio = m_studio;

	// Try load default motion file
	studioMotionData_t* motionData = Studio_LoadMotionData((m_name.Path_Strip_Ext() + ".mop").GetData(), studio->numBones);
	if (motionData)
		m_motionData.append(motionData);

	// load motion packages that are additionally specified in EGF model
	for (int i = 0; i < studio->numMotionPackages; i++)
	{
		const EqString mopPath(m_name.Path_Strip_Name() + studio->pPackage(i)->packageName + ".mop");
		DevMsg(DEVMSG_CORE, "Loading motion package for '%s'\n", mopPath.ToCString());

		studioMotionData_t* motionData = Studio_LoadMotionData(mopPath.ToCString(), studio->numBones);
		if (motionData)
			m_motionData.append(motionData);
		else
			MsgError("Can't open motion package '%s' specified in EGF\n", studio->pPackage(i)->packageName);
	}

	// load additional external motion packages requested by user
	for (int i = 0; i < m_additionalMotionPackages.numElem(); i++)
	{
		studioMotionData_t* motionData = Studio_LoadMotionData(m_additionalMotionPackages[i].ToCString(), studio->numBones);
		if (motionData)
			m_motionData.append(motionData);
		else
			MsgError("Can't open motion data package '%s'!\n", m_additionalMotionPackages[i].ToCString());
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
			fpath.Path_FixSlashes();

			if (fpath.ToCString()[0] == CORRECT_PATH_SEPARATOR)
				fpath = EqString(fpath.ToCString(), fpath.Length() - 1);

			for (int j = 0; j < studio->numMaterialSearchPaths; j++)
			{
				if (m_materials[i])
					continue;

				EqString spath(studio->pMaterialSearchPath(j)->searchPath);
				spath.Path_FixSlashes();

				if (spath.Length() && spath.ToCString()[spath.Length() - 1] == CORRECT_PATH_SEPARATOR)
					spath = spath.Left(spath.Length() - 1);

				EqString extend_path;
				CombinePath(extend_path, spath.ToCString(), fpath.ToCString());

				if (!materials->IsMaterialExist(extend_path))
					continue;

				IMaterialPtr material = materials->GetMaterial(extend_path.GetData());
				materials->PutMaterialToLoadingQueue(material);

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

			m_materials[i] = g_studioModelCache->GetErrorMaterial();

			const char* materialName = studio->pMaterial(i)->materialname;
			if (*materialName)
				MsgError("Couldn't load model material '%s'\n", materialName, m_name.ToCString());
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
		return materials->GetDefaultMaterial();

	materialGroupIdx = clamp(materialGroupIdx, 0, m_materialGroupsCount - 1);
	return m_materials[m_materialCount * materialGroupIdx + materialIdx];
}

void CEqStudioGeom::LoadSetupBones()
{
	studioHdr_t* studio = m_studio;

	// Initialize HW data joints
	m_joints = PPNew studioJoint_t[studio->numBones];

	// parse bones
	for (int i = 0; i < studio->numBones; ++i)
	{
		// copy all bone data
		const studioBoneDesc_t* bone = studio->pBone(i);

		studioJoint_t& joint = m_joints[i];
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
		studioJoint_t& joint = m_joints[i];
		const int parent_index = joint.parent;

		if (parent_index != -1)
			m_joints[parent_index].childs.append(i);

		if (joint.parent != -1)
			joint.absTrans = joint.localTrans * m_joints[joint.parent].absTrans;
		else
			joint.absTrans = joint.localTrans;
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

void CEqStudioGeom::SetupVBOStream(EGFHwVertex::VertexStream vertStream, int rhiStreamId) const
{
	if (!m_vertexBuffers[vertStream])
		return;

	g_pShaderAPI->SetVertexBuffer(m_vertexBuffers[vertStream], rhiStreamId);
}

void CEqStudioGeom::Draw(const DrawProps& drawProperties) const
{
	if (!drawProperties.bodyGroupFlags)
		return;

	const bool isSkinned = drawProperties.boneTransforms;

	bonequaternion_t bquats[128];
	int numBoneRegisters = 0;
	if (isSkinned)
		 numBoneRegisters = ComputeQuaternionsForSkinning(this, bquats, drawProperties.boneTransforms);

	materials->SetSkinningEnabled(isSkinned);
	materials->SetInstancingEnabled(drawProperties.instanced);

	IVertexFormat* rhiVertFmt = drawProperties.vertexFormat ? drawProperties.vertexFormat : g_studioModelCache->GetEGFVertexFormat(isSkinned);
	g_pShaderAPI->SetVertexFormat(rhiVertFmt);
	// setup vertex buffers
	{
		ArrayCRef<VertexFormatDesc_t> fmtDesc = rhiVertFmt->GetFormatDesc();

		int setVertStreams = 0;
		int numBitsSet = 0;
		for (int i = 0; i < fmtDesc.numElem(); ++i)
		{
			if (numBitsSet == EGFHwVertex::VERT_COUNT)
				break;

			const VertexFormatDesc_t& desc = fmtDesc[i];
			const EGFHwVertex::VertexStream vertStreamId = drawProperties.vertexStreamMapping[desc.streamId];

			if (setVertStreams & (1 << int(vertStreamId)))
				continue;

			g_pShaderAPI->SetVertexBuffer(m_vertexBuffers[vertStreamId], desc.streamId);

			setVertStreams |= (1 << int(vertStreamId));
			++numBitsSet;
		}
	}

	g_pShaderAPI->SetIndexBuffer(m_indexBuffer);
	const int maxVertexCount = m_vertexBuffers[EGFHwVertex::VERT_POS_UV]->GetVertexCount();

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

			const int materialMask = materialFlags & drawProperties.materialFlags;
			if (drawProperties.excludeMaterialFlags && materialMask > 0)
				continue;
			else if (materialFlags && !drawProperties.excludeMaterialFlags && !materialMask)
				continue;

			if (drawProperties.preSetupFunc)
				drawProperties.preSetupFunc(material, i);

			if (!drawProperties.skipMaterials)
				materials->BindMaterial(material, 0);

			if (drawProperties.preDrawFunc)
				drawProperties.preDrawFunc(material, i);

			if (numBoneRegisters)
				g_pShaderAPI->SetShaderConstantArrayVector4D(StringToHashConst("Bones"), (Vector4D*)&bquats[0].quat, numBoneRegisters);

			materials->Apply();

			const HWGeomRef::Mesh& meshRef = m_hwGeomRefs[modelDescId].meshRefs[j];
			g_pShaderAPI->DrawIndexedPrimitives((ER_PrimitiveType)meshRef.primType, meshRef.firstIndex, meshRef.indexCount, 0, maxVertexCount);
		}
	}

	materials->SetSkinningEnabled(false);
	materials->SetInstancingEnabled(false);
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

int	CEqStudioGeom::GetLoadingState() const
{
	g_parallelJobs->CompleteJobCallbacks();
	return m_readyState;
}

const studioHdr_t& CEqStudioGeom::GetStudioHdr() const
{
	while (!m_studio && GetLoadingState() == MODEL_LOAD_IN_PROGRESS) // wait for hwdata
		Platform_Sleep(1);

	return *m_studio; 
}

const studioPhysData_t& CEqStudioGeom::GetPhysData() const 
{
	while (!m_studio && GetLoadingState() == MODEL_LOAD_IN_PROGRESS) // wait for hwdata
		Platform_Sleep(1);

	return m_physModel;
}

const studioMotionData_t& CEqStudioGeom::GetMotionData(int index) const
{
	while (!m_studio && GetLoadingState() == MODEL_LOAD_IN_PROGRESS) // wait for hwdata
		Platform_Sleep(1);

	return *m_motionData[index];
}

const studioJoint_t& CEqStudioGeom::GetJoint(int index) const
{
	return m_joints[index];
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

Matrix4x4 CEqStudioGeom::GetLocalTransformMatrix(int attachmentIdx) const
{
	ASSERT(attachmentIdx >= 0 && attachmentIdx < m_studio->numTransforms);

	const studioTransform_t* attach = m_studio->pTransform(attachmentIdx);
	return attach->transform;
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

			const int numIndices = (pMesh->primitiveType == EGFPRIM_TRI_STRIP) ? pMesh->numIndices - 2 : pMesh->numIndices;
			const int indexStep = (pMesh->primitiveType == EGFPRIM_TRI_STRIP) ? 1 : 3;

			for (int k = 0; k < numIndices; k += indexStep)
			{
				// skip strip degenerates
				if (pIndices[k] == pIndices[k + 1] || pIndices[k] == pIndices[k + 2] || pIndices[k + 1] == pIndices[k + 2])
					continue;

				const int even = k % 2;	// handle flipped triangles on EGFPRIM_TRI_STRIP
				
				int i0, i1, i2;
				if (even && pMesh->primitiveType == EGFPRIM_TRI_STRIP)
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

			const int numIndices = (pMesh->primitiveType == EGFPRIM_TRI_STRIP) ? pMesh->numIndices - 2 : pMesh->numIndices;
			const int indexStep = (pMesh->primitiveType == EGFPRIM_TRI_STRIP) ? 1 : 3;

			for (int k = 0; k < numIndices; k += indexStep)
			{
				// skip strip degenerates
				if (pIndices[k] == pIndices[k+1] || pIndices[k] == pIndices[k+2] || pIndices[k+1] == pIndices[k+2])
					continue;

				const int even = k % 2; // handle flipped triangles on EGFPRIM_TRI_STRIP

				Vector3D v0, v1, v2;
				if (even && pMesh->primitiveType == EGFPRIM_TRI_STRIP)
				{
					v0 = pMesh->pVertex(pIndices[k + 2])->point;
					v1 = pMesh->pVertex(pIndices[k + 1])->point;
					v2 = pMesh->pVertex(pIndices[k])->point;
				}
				else
				{
					v0 = pMesh->pVertex(pIndices[k])->point;
					v1 = pMesh->pVertex(pIndices[k + 1])->point;
					v2 = pMesh->pVertex(pIndices[k + 2])->point;
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