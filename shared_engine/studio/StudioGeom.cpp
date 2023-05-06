//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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

int EGFHwVertex_t::GetVertexFormatDesc(const VertexFormatDesc_t** desc)
{
	static const VertexFormatDesc_t g_EGFHwVertexFormat[] = {
		{ 0, 4, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_HALF, "position" },		// position
		{ 0, 2, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "texcoord" },		// texcoord 0

		// FIXME: split into streams?
		{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "tangent" },		// Tangent (TC1)
		{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "binormal" },		// Binormal (TC2)
		{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "normal" },		// Normal (TC3)

		{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "boneid" },		// Bone indices (hw skinning), (TC4)
		{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "bonew" }			// Bone weights (hw skinning), (TC5)
	};

	*desc = g_EGFHwVertexFormat;
	return elementsOf(g_EGFHwVertexFormat);
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
static bool TransformEGFVertex(EGFHwVertex_t& vert, Matrix4x4* pMatrices)
{
	bool bAffected = false;

	Vector3D vPos = vec3_zero;
	Vector3D vNormal = vec3_zero;
	Vector3D vTangent = vec3_zero;
	Vector3D vBinormal = vec3_zero;

	for (int j = 0; j < 4; j++)
	{
		int index = vert.boneIndices[j];

		if (index == -1)
			continue;

		float weight = vert.boneWeights[j];

		bAffected = true;

		vPos += transformPoint(Vector3D(vert.pos.xyz()), pMatrices[index]) * weight;
		vNormal += transformVector(Vector3D(vert.normal), pMatrices[index]) * weight;
		vTangent += transformVector(Vector3D(vert.tangent), pMatrices[index]) * weight;
		vBinormal += transformVector(Vector3D(vert.binormal), pMatrices[index]) * weight;
	}

	if (bAffected)
	{
		vert.pos = Vector4D(vPos, 1.0f);
		vert.normal = vNormal;
		vert.tangent = vTangent;
		vert.binormal = vBinormal;
	}

	return bAffected;
}

static int ComputeQuaternionsForSkinning(const CEqStudioGeom* model, bonequaternion_t* bquats, Matrix4x4* btransforms)
{
	const studiohdr_t& studio = model->GetStudioHdr();
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
			m_vertexBuffer->Update(m_softwareVerts, m_vertexBuffer->GetVertexCount() * sizeof(EGFHwVertex_t), 0);
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
		EGFHwVertex_t* bufferData = nullptr;
		if (m_vertexBuffer->Lock(0, verticesCount, (void**)&bufferData, false))
		{
			// setup each bone's transformation
			for (int i = 0; i < verticesCount; i++)
			{
				EGFHwVertex_t vert = m_softwareVerts[i];
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

	if (m_instancer != nullptr)
		delete m_instancer;
	m_instancer = nullptr;

	g_pShaderAPI->Reset(STATE_RESET_VBO);
	g_pShaderAPI->DestroyVertexBuffer(m_vertexBuffer);
	g_pShaderAPI->DestroyIndexBuffer(m_indexBuffer);
	m_vertexBuffer = nullptr;
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

		for (int i = 0; i < m_studio->numModels; i++)
			delete[] m_hwGeomRefs[i].groups;

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

static int CopyGroupVertexDataToHWList(EGFHwVertex_t* hwVtxList, int currentVertexCount, const modelgroupdesc_t* pGroup, BoundingBox& aabb)
{
	for (int32 i = 0; i < pGroup->numVertices; i++)
	{
		const studiovertexdesc_t* pVertex = pGroup->pVertex(i);

		hwVtxList[currentVertexCount++] = EGFHwVertex_t(*pVertex);
		aabb.AddVertex(pVertex->point);
	}

	return pGroup->numVertices;
}

static int CopyGroupIndexDataToHWList(void* indexData, int indexSize, int currentIndexCount, const modelgroupdesc_t* pGroup, int vertex_add_offset)
{
	if (indexSize == sizeof(int))
	{
		for (uint32 i = 0; i < pGroup->numIndices; i++)
		{
			const int index = (*pGroup->pVertexIdx(i)) + vertex_add_offset;
			uint32* indices = (uint32*)indexData;
			indices[currentIndexCount++] = index;
		}
	}
	else if (indexSize == sizeof(short))
	{
		for (uint32 i = 0; i < pGroup->numIndices; i++)
		{
			const int index = (*pGroup->pVertexIdx(i)) + vertex_add_offset;
			uint16* indices = (uint16*)indexData;
			indices[currentIndexCount++] = index;
		}
	}
	else
	{
		ASSERT_FAIL("CopyGroupIndexDataToHWList - Bad index size");
	}

	return pGroup->numIndices;
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
	studiohdr_t* pHdr = Studio_LoadModel(m_name.ToCString());

	if (!pHdr)
		return false; // get out, nothing to load

	m_studio = pHdr;

	return true;
}

bool CEqStudioGeom::LoadGenerateVertexBuffer()
{
	// detect and set the force software skinning flag
	m_forceSoftwareSkinning = r_force_softwareskinning.GetBool();

	const studiohdr_t* studio = m_studio;

	// use index size
	int numVertices = 0;
	int numIndices = 0;

	// find vert and index count
	for (int i = 0; i < studio->numModels; i++)
	{
		studiomodeldesc_t* pModelDesc = studio->pModelDesc(i);

		for (int j = 0; j < pModelDesc->numGroups; j++)
		{
			modelgroupdesc_t* pGroup = pModelDesc->pGroup(j);

			numVertices += pGroup->numVertices;
			numIndices += pGroup->numIndices;
		}
	}

	int indexSize = sizeof(short);

	if (numVertices > int(USHRT_MAX))
		indexSize = sizeof(int);

	EGFHwVertex_t* allVerts = PPNew EGFHwVertex_t[numVertices];
	ubyte* allIndices = PPNew ubyte[indexSize * numIndices];

	numVertices = 0;
	numIndices = 0;

	m_hwGeomRefs = PPNew HWGeomRef[studio->numModels];

	for (int i = 0; i < studio->numModels; i++)
	{
		const studiomodeldesc_t* pModelDesc = studio->pModelDesc(i);

		HWGeomRef::Group* groups = PPNew HWGeomRef::Group[pModelDesc->numGroups];
		m_hwGeomRefs[i].groups = groups;

		for (int j = 0; j < pModelDesc->numGroups; j++)
		{
			// add vertices, add indices
			const modelgroupdesc_t* pGroup = pModelDesc->pGroup(j);

			groups[j].firstIndex = numIndices;
			groups[j].indexCount = pGroup->numIndices;
			groups[j].primType = pGroup->primitiveType;

			const int new_offset = numVertices;

			numVertices += CopyGroupVertexDataToHWList(allVerts, numVertices, pGroup, m_boundingBox);
			numIndices += CopyGroupIndexDataToHWList(allIndices, indexSize, numIndices, pGroup, new_offset);
		}
	}

	// create hardware buffers
	m_vertexBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_STATIC, numVertices, sizeof(EGFHwVertex_t), allVerts);
	m_indexBuffer = g_pShaderAPI->CreateIndexBuffer(numIndices, indexSize, BUFFER_STATIC, allIndices);

	// if we using software skinning, we need to create temporary vertices
	if (m_forceSoftwareSkinning)
	{
		m_softwareVerts = PPAllocStructArray(EGFHwVertex_t, numVertices);
		memcpy(m_softwareVerts, allVerts, sizeof(EGFHwVertex_t) * numVertices);
	}

	// done.
	delete[] allVerts;
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
	const studiohdr_t* studio = m_studio;

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
	studiohdr_t* studio = m_studio;

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

			m_materials[i] = materials->GetMaterial(studio->pMaterial(i)->materialname);

			if(m_materials[i]->IsError())
				MsgError("Couldn't load model material '%s'\n", studio->pMaterial(i)->materialname, m_name.ToCString());
			bError = true;
		}
	}

	int maxMaterialIdx = -1;

	for (int i = 0; i < studio->numModels; i++)
	{
		studiomodeldesc_t* pModelDesc = studio->pModelDesc(i);

		for (int j = 0; j < pModelDesc->numGroups; j++)
		{
			// add vertices, add indices
			modelgroupdesc_t* pGroup = pModelDesc->pGroup(j);

			if (pGroup->materialIndex > maxMaterialIdx)
				maxMaterialIdx = pGroup->materialIndex;
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
	studiohdr_t* studio = m_studio;

	// Initialize HW data joints
	m_joints = PPNew studioJoint_t[studio->numBones];

	// parse bones
	for (int i = 0; i < studio->numBones; ++i)
	{
		// copy all bone data
		const bonedesc_t* bone = studio->pBone(i);

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
		const studioikchain_t* ikChain = studio->pIkChain(i);
		for (int j = 0; j < ikChain->numLinks; ++j)
		{
			studioiklink_t* ikLink = ikChain->pLink(j);
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
		if (distance > m_studio->pLodParams(i)->distance * r_egf_LodScale.GetFloat())
			idealLOD = i;
	}

	return idealLOD;
}

void CEqStudioGeom::SetupVBOStream(int nStream) const
{
	if (!m_vertexBuffer)
		return;

	g_pShaderAPI->SetVertexBuffer(m_vertexBuffer, nStream);
}

void CEqStudioGeom::Draw(const DrawProps& drawProperties) const
{
	if (!drawProperties.bodyGroupFlags)
		return;

	bonequaternion_t bquats[128];

	int numBoneRegisters = 0;
	if (drawProperties.boneTransforms)
		 numBoneRegisters = ComputeQuaternionsForSkinning(this, bquats, drawProperties.boneTransforms);

	materials->SetSkinningEnabled(drawProperties.boneTransforms);
	materials->SetInstancingEnabled(drawProperties.instanced);

	g_pShaderAPI->SetVertexFormat(drawProperties.vertexFormat ? drawProperties.vertexFormat : g_studioModelCache->GetEGFVertexFormat());
	g_pShaderAPI->SetVertexBuffer(m_vertexBuffer, drawProperties.mainVertexStream);
	g_pShaderAPI->SetIndexBuffer(m_indexBuffer);

	const studiohdr_t& studio = *m_studio;
	for (int i = 0; i < studio.numBodyGroups; ++i)
	{
		// check bodygroups for rendering
		if (!(drawProperties.bodyGroupFlags & (1 << i)))
			continue;

		const int bodyGroupLodIndex = studio.pBodyGroups(i)->lodModelIndex;
		const studiolodmodel_t* lodModel = studio.pLodModel(bodyGroupLodIndex);

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

		const studiomodeldesc_t* modDesc = studio.pModelDesc(modelDescId);

		// render model groups that in this body group
		for (int j = 0; j < modDesc->numGroups; ++j)
		{
			const int materialIndex = modDesc->pGroup(j)->materialIndex;
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

			const HWGeomRef::Group& groupDesc = m_hwGeomRefs[modelDescId].groups[j];
			g_pShaderAPI->DrawIndexedPrimitives((ER_PrimitiveType)groupDesc.primType, groupDesc.firstIndex, groupDesc.indexCount, 0, m_vertexBuffer->GetVertexCount());
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

const studiohdr_t& CEqStudioGeom::GetStudioHdr() const
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

static void MakeDecalTexCoord(Array<EGFHwVertex_t>& verts, Array<int>& indices, const DecalMakeInfo& info)
{
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
}

// makes dynamic temporary decal
CRefPtr<DecalData> CEqStudioGeom::MakeDecal(const DecalMakeInfo& info, Matrix4x4* jointMatrices, int bodyGroupFlags, int lod) const
{
	if (r_egf_NoTempDecals.GetBool())
		return nullptr;

	const studiohdr_t* studio = m_studio;

	Vector3D decal_origin = info.origin;
	Vector3D decal_normal = info.normal;

	Array<EGFHwVertex_t> verts(PP_SL);
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
		const studiolodmodel_t* lodModel = studio->pLodModel(bodyGroupLodIndex);

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

		const studiomodeldesc_t* modDesc = studio->pModelDesc(modelDescId);

		Array<EGFHwVertex_t> g_verts(PP_SL);
		Array<int> g_indices(PP_SL);
		Array<int> g_orig_indices(PP_SL);

		for (int j = 0; j < modDesc->numGroups; j++)
		{
			const modelgroupdesc_t* pGroup = modDesc->pGroup(j);

			const uint32* pIndices = pGroup->pVertexIdx(0);

			g_verts.clear();
			g_indices.clear();
			g_orig_indices.clear();

			g_verts.reserve(pGroup->numIndices);
			g_indices.reserve(pGroup->numIndices);
			g_orig_indices.reserve(pGroup->numIndices);

			const int numIndices = (pGroup->primitiveType == EGFPRIM_TRI_STRIP) ? pGroup->numIndices - 2 : pGroup->numIndices;
			const int indexStep = (pGroup->primitiveType == EGFPRIM_TRI_STRIP) ? 1 : 3;

			for (uint32 k = 0; k < numIndices; k += indexStep)
			{
				// skip strip degenerates
				if (pIndices[k] == pIndices[k + 1] || pIndices[k] == pIndices[k + 2] || pIndices[k + 1] == pIndices[k + 2])
					continue;

				const int even = k % 2;	// handle flipped triangles on EGFPRIM_TRI_STRIP
				
				int i0, i1, i2;
				if (even && pGroup->primitiveType == EGFPRIM_TRI_STRIP)
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

				EGFHwVertex_t v0(*pGroup->pVertex(i0));
				EGFHwVertex_t v1(*pGroup->pVertex(i1));
				EGFHwVertex_t v2(*pGroup->pVertex(i2));

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

				auto egf_vertex_comp = [](const EGFHwVertex_t& a, const EGFHwVertex_t& b) -> bool
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
				g_verts[g_indices[k]].pos = Vector4D(pGroup->pVertex(g_orig_indices[k])->point, 1.0f);
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
		decal->verts = PPAllocStructArray(EGFHwVertex_t, decal->numVerts);
		decal->indices = PPAllocStructArray(uint16, decal->numIndices);

		// copy geometry
		memcpy(decal->verts, verts.ptr(), sizeof(EGFHwVertex_t) * decal->numVerts);

		for (int i = 0; i < decal->numIndices; i++)
			decal->indices[i] = indices[i];

		return decal;
	}

	return nullptr;
}


float CEqStudioGeom::CheckIntersectionWithRay(const Vector3D& rayStart, const Vector3D& rayDir, int bodyGroupFlags, int lod) const
{
	float f1, f2;
	if(!m_boundingBox.IntersectsRay(rayStart, rayDir, f1, f2))
		return F_INFINITY;

	float best_dist = F_INFINITY;

	const studiohdr_t& studio = *m_studio;

	for(int i = 0; i < studio.numBodyGroups; i++)
	{
		if (!(bodyGroupFlags & (1 << i)))
			continue;

		const int bodyGroupLodIndex = studio.pBodyGroups(i)->lodModelIndex;
		const studiolodmodel_t* lodModel = studio.pLodModel(bodyGroupLodIndex);

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

		const studiomodeldesc_t* modDesc = studio.pModelDesc(modelDescId);

		for(int j = 0; j < modDesc->numGroups; j++)
		{
			const modelgroupdesc_t* pGroup = modDesc->pGroup(j);

			const uint32* pIndices = pGroup->pVertexIdx(0);

			const int numIndices = (pGroup->primitiveType == EGFPRIM_TRI_STRIP) ? pGroup->numIndices - 2 : pGroup->numIndices;
			const int indexStep = (pGroup->primitiveType == EGFPRIM_TRI_STRIP) ? 1 : 3;

			for (int k = 0; k < numIndices; k += indexStep)
			{
				// skip strip degenerates
				if (pIndices[k] == pIndices[k+1] || pIndices[k] == pIndices[k+2] || pIndices[k+1] == pIndices[k+2])
					continue;

				const int even = k % 2; // handle flipped triangles on EGFPRIM_TRI_STRIP

				Vector3D v0, v1, v2;
				if (even && pGroup->primitiveType == EGFPRIM_TRI_STRIP)
				{
					v0 = pGroup->pVertex(pIndices[k + 2])->point;
					v1 = pGroup->pVertex(pIndices[k + 1])->point;
					v2 = pGroup->pVertex(pIndices[k])->point;
				}
				else
				{
					v0 = pGroup->pVertex(pIndices[k])->point;
					v1 = pGroup->pVertex(pIndices[k + 1])->point;
					v2 = pGroup->pVertex(pIndices[k + 2])->point;
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