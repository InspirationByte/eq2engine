//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Generic
//////////////////////////////////////////////////////////////////////////////////

#include "studio_egf.h"

#include "core/DebugInterface.h"
#include "core/IEqParallelJobs.h"
#include "core/ConVar.h"

#include "ds/SmartPtr.h"
#include "utils/strtools.h"

#include "physics/IStudioShapeCache.h"
#include "math/Utility.h"
#include "materialsystem1/IMaterialSystem.h"


ConVar r_lodtest("r_lodtest", "-1", -1.0f, MAX_MODELLODS, "Studio LOD test", CV_CHEAT);
ConVar r_lodscale("r_lodscale", "1.0", "Studio model LOD scale", CV_ARCHIVE);
ConVar r_lodstart("r_lodstart", "0", 0, MAX_MODELLODS, "Studio LOD start index", CV_ARCHIVE);

ConVar r_notempdecals("r_notempdecals", "0", "Disables temp decals", CV_CHEAT);
//ConVar r_force_softwareskinning("r_force_softwareskinning", "0", "Forces software skinning", CV_ARCHIVE);

CEngineStudioEGF::CEngineStudioEGF()
{
	m_instancer = nullptr;
	m_readyState = MODEL_LOAD_ERROR;

	m_forceSoftwareSkinning = false;
	m_skinningDirty = false;

	m_pVB = nullptr;
	m_pIB = nullptr;

	m_softwareVerts = nullptr;

	m_numVertices = 0;
	m_numIndices = 0;

	memset(m_materials, 0, sizeof(m_materials));
	m_numMaterials = 0;

	m_hwdata = nullptr;

	m_cacheIdx = -1;
}

CEngineStudioEGF::~CEngineStudioEGF()
{
	DestroyModel();
}

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

ConVar r_force_softwareskinning("r_force_softwareskinning", "0", "Force software skinning", CV_CHEAT);

// Software vertex transformation, only for compatibility
bool TransformEGFVertex(EGFHwVertex_t& vert, Matrix4x4* pMatrices)
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

		vPos += transform4(Vector3D(vert.pos.xyz()), pMatrices[index]) * weight;
		vNormal += transform3(Vector3D(vert.normal), pMatrices[index]) * weight;
		vTangent += transform3(Vector3D(vert.tangent), pMatrices[index]) * weight;
		vBinormal += transform3(Vector3D(vert.binormal), pMatrices[index]) * weight;
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

//------------------------------------------
// Software skinning of model. Very slow, but recomputes bounding box for all model
//------------------------------------------
bool CEngineStudioEGF::PrepareForSkinning(Matrix4x4* jointMatrices)
{
	if (!m_hwdata || (m_hwdata && !m_hwdata->studio))
		return false;

	if (m_hwdata->studio->numBones == 0)
		return false; // model is not animatable, skip

	if (!jointMatrices)
		return false;

	// TODO: SOFTWARE SKINNING FOR BAD SLOWEST FOR SLOW PC's, and we've never fix it, HAHA!

	/*if(g_pShaderAPI->IsSupportsHardwareSkinning() && !r_force_softwareskinning.GetBool())*/
	if (!r_force_softwareskinning.GetBool())
	{
		if (m_skinningDirty)
		{
			EGFHwVertex_t* bufferData = nullptr;

			if (m_pVB->Lock(0, m_numVertices, (void**)&bufferData, false))
			{
				memcpy(bufferData, m_softwareVerts, m_numVertices * sizeof(EGFHwVertex_t));
				m_pVB->Unlock();
			}

			m_skinningDirty = false;
		}

		bonequaternion_t bquats[128];

		int numBones = m_hwdata->studio->numBones;

		for (int i = 0; i < numBones; i++)
		{
			// FIXME: kind of slowness
			Matrix4x4 toAbsTransform = (!m_hwdata->joints[i].absTrans * jointMatrices[i]);

			// cast matrices to quaternions
			// note that quaternions uses transposes matrix set.
			bquats[i].quat = Quaternion(transpose(toAbsTransform).getRotationComponent()).asVector4D();
			bquats[i].origin = Vector4D(toAbsTransform.rows[3].xyz(), 1);
		}

		//g_pShaderAPI->SetVertexShaderConstantVector4DArray(100, (Vector4D*)&bquats[0].quat, m_hwdata->studio->numBones*2);
		g_pShaderAPI->SetShaderConstantArrayVector4D("Bones", (Vector4D*)&bquats[0].quat, numBones * 2);

		return true;
	}
	else if (m_forceSoftwareSkinning && r_force_softwareskinning.GetBool())
	{
		m_skinningDirty = true;

		Matrix4x4 tempMatrixArray[128];

		// Send all matrices as 4x3
		for (int i = 0; i < m_hwdata->studio->numBones; i++)
		{
			// FIXME: kind of slowness
			tempMatrixArray[i] = (!m_hwdata->joints[i].absTrans * jointMatrices[i]);
		}

		EGFHwVertex_t* bufferData = nullptr;

		if (m_pVB->Lock(0, m_numVertices, (void**)&bufferData, false))
		{
			// setup each bone's transformation
			for (int i = 0; i < m_numVertices; i++)
			{
				EGFHwVertex_t vert = m_softwareVerts[i];

				TransformEGFVertex(vert, tempMatrixArray);

				bufferData[i] = vert;

			}

			m_pVB->Unlock();

			return false;
		}
	}

	return false;
}

void CEngineStudioEGF::DestroyModel()
{
	DevMsg(DEVMSG_CORE, "DestroyModel: '%s'\n", m_szPath.ToCString());

	m_readyState = MODEL_LOAD_ERROR;

	// instancer is removed here if set
	if (m_instancer != nullptr)
		delete m_instancer;

	m_instancer = nullptr;

	g_pShaderAPI->Reset(STATE_RESET_VBO);
	g_pShaderAPI->ApplyBuffers();

	g_pShaderAPI->DestroyVertexBuffer(m_pVB);
	g_pShaderAPI->DestroyIndexBuffer(m_pIB);

	for (int i = 0; i < m_numMaterials; i++)
		materials->FreeMaterial(m_materials[i]);

	memset(m_materials, 0, sizeof(m_materials));

	m_numMaterials = 0;

	if (m_forceSoftwareSkinning)
	{
		PPFree(m_softwareVerts);
		m_softwareVerts = nullptr;
	}

	m_numIndices = 0;
	m_numVertices = 0;
	m_numMaterials = 0;

	if (m_hwdata)
	{
		if (m_hwdata->studio)
		{
			for (int i = 0; i < MAX_MOTIONPACKAGES; i++)
			{
				if (!m_hwdata->motiondata[i])
					continue;

				Studio_FreeMotionData(m_hwdata->motiondata[i], m_hwdata->studio->numBones);
				PPFree(m_hwdata->motiondata[i]);
			}

			g_studioShapeCache->DestroyStudioCache(&m_hwdata->physModel);
			Studio_FreePhysModel(&m_hwdata->physModel);

			auto lodRefs = m_hwdata->modelrefs;

			for (int i = 0; i < m_hwdata->studio->numModels; i++)
				delete[] lodRefs[i].groupDescs;

			delete[] m_hwdata->modelrefs;
			delete[] m_hwdata->joints;

			delete m_hwdata->studio;
		}

		delete m_hwdata;
		m_hwdata = nullptr;
	}
}

void CEngineStudioEGF::LoadPhysicsData()
{
	EqString podFileName = m_szPath.Path_Strip_Ext();
	podFileName.Append(".pod");

	if (Studio_LoadPhysModel(podFileName.GetData(), &m_hwdata->physModel))
	{
		ASSERT_MSG(g_studioShapeCache, "studio shape cache is not initialized!\n");
		g_studioShapeCache->InitStudioCache(&m_hwdata->physModel);
	}
}

extern ConVar r_detaillevel;

int CopyGroupVertexDataToHWList(EGFHwVertex_t* hwVtxList, int currentVertexCount, modelgroupdesc_t* pGroup, BoundingBox& aabb)
{
	for (int32 i = 0; i < pGroup->numVertices; i++)
	{
		studiovertexdesc_t* pVertex = pGroup->pVertex(i);

		hwVtxList[currentVertexCount] = EGFHwVertex_t(*pVertex);
		aabb.AddVertex(hwVtxList[currentVertexCount++].pos.xyz());
	}

	return pGroup->numVertices;
}

int CopyGroupIndexDataToHWList(void* indexData, int indexSize, int currentIndexCount, modelgroupdesc_t* pGroup, int vertex_add_offset)
{
	for (uint32 i = 0; i < pGroup->numIndices; i++)
	{
		// always add offset to index (usually it's a current loadedvertices size)
		int index = (*pGroup->pVertexIdx(i)) + vertex_add_offset;

		if (indexSize == INDEX_SIZE_INT)
		{
			uint32* indices = (uint32*)indexData;
			indices[currentIndexCount++] = index;
		}
		else if (indexSize == INDEX_SIZE_SHORT)
		{
			uint16* indices = (uint16*)indexData;
			indices[currentIndexCount++] = index;
		}
	}

	return pGroup->numIndices;
}

void CEngineStudioEGF::LoadModelJob(void* data, int i)
{
	CEngineStudioEGF* model = (CEngineStudioEGF*)data;

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

void CEngineStudioEGF::LoadVertsJob(void* data, int i)
{
	CEngineStudioEGF* model = (CEngineStudioEGF*)data;
	
	if (model->m_readyState == MODEL_LOAD_ERROR)
		return;

	if (!model->LoadGenerateVertexBuffer())
		model->DestroyModel();
}

void CEngineStudioEGF::LoadPhysicsJob(void* data, int i)
{
	CEngineStudioEGF* model = (CEngineStudioEGF*)data;

	if (model->m_readyState == MODEL_LOAD_ERROR)
		return;

	model->LoadPhysicsData();
}

void CEngineStudioEGF::LoadMotionJob(void* data, int i)
{
	CEngineStudioEGF* model = (CEngineStudioEGF*)data;
	if (model->m_readyState == MODEL_LOAD_ERROR)
		return;

	model->LoadSetupBones();
	model->LoadMotionPackages();
}

void CEngineStudioEGF::OnLoadingJobComplete(struct eqParallelJob_t* job)
{
	CEngineStudioEGF* model = (CEngineStudioEGF*)job->arguments;

	if (model->m_readyState == MODEL_LOAD_ERROR)
		return;
	
	if(model->m_loading.Decrement() <= 0)
		model->m_readyState = MODEL_LOAD_OK;
}

bool CEngineStudioEGF::LoadModel(const char* pszPath, bool useJob)
{
	m_szPath = pszPath;
	m_szPath.Path_FixSlashes();

	// first we switch to loading
	m_readyState = MODEL_LOAD_IN_PROGRESS;

	// allocate hardware data
	m_hwdata = PPNew studioHwData_t;
	memset(m_hwdata, 0, sizeof(studioHwData_t));

	if (useJob)
	{
		m_loading.SetValue(3);
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
	
	m_readyState = MODEL_LOAD_OK;

	return true;
}

bool CEngineStudioEGF::LoadFromFile()
{
	studiohdr_t* pHdr = Studio_LoadModel(m_szPath.ToCString());

	if (!pHdr)
		return false; // get out, nothing to load

	m_hwdata->studio = pHdr;

	return true;
}

bool CEngineStudioEGF::LoadGenerateVertexBuffer()
{
	// detect and set the force software skinning flag
	m_forceSoftwareSkinning = r_force_softwareskinning.GetBool();

	studiohdr_t* pHdr = m_hwdata->studio;

	auto lodModels = PPNew studioModelRef_t[pHdr->numModels];
	m_hwdata->modelrefs = lodModels;

	int maxMaterialIdx = -1;

	// TODO: this should be optimized by the compiler
	{
		// load all group vertices and indices
		//Array<EGFHwVertex_t>	loadedvertices{ PP_SL };

		//Array<uint32>			loadedindices{ PP_SL };
		//Array<uint16>			loadedindices_short{ PP_SL };

		// use index size
		int nIndexSize = INDEX_SIZE_SHORT;
		int numVertices = 0;
		int numIndices = 0;

		// determine index size
		for (int i = 0; i < pHdr->numModels; i++)
		{
			studiomodeldesc_t* pModelDesc = pHdr->pModelDesc(i);

			for (int j = 0; j < pModelDesc->numGroups; j++)
			{
				modelgroupdesc_t* pGroup = pModelDesc->pGroup(j);

				numVertices += pGroup->numVertices;
				numIndices += pGroup->numIndices;
			}
		}

		if (numVertices > int(USHRT_MAX))
			nIndexSize = INDEX_SIZE_INT;

		EGFHwVertex_t* allVerts = PPNew EGFHwVertex_t[numVertices];
		ubyte* allIndices = PPNew ubyte[nIndexSize * numIndices];

		numVertices = 0;
		numIndices = 0;

		for (int i = 0; i < pHdr->numModels; i++)
		{
			studiomodeldesc_t* pModelDesc = pHdr->pModelDesc(i);
			studioModelRefGroupDesc_t* groupDescs = PPNew studioModelRefGroupDesc_t[pModelDesc->numGroups];
			lodModels[i].groupDescs = groupDescs;

			for (int j = 0; j < pModelDesc->numGroups; j++)
			{
				// add vertices, add indices
				modelgroupdesc_t* pGroup = pModelDesc->pGroup(j);

				// set lod index
				int lod_first_index = numIndices;
				groupDescs[j].firstindex = lod_first_index;

				int new_offset = numVertices;

				// copy vertices to new buffer first
				numVertices += CopyGroupVertexDataToHWList(allVerts, numVertices, pGroup, m_aabb);

				// then using new offset copy indices to buffer
				numIndices += CopyGroupIndexDataToHWList(allIndices, nIndexSize, numIndices, pGroup, new_offset);

				// set index count for lod group
				groupDescs[j].indexcount = pGroup->numIndices;

				if (pGroup->materialIndex > maxMaterialIdx)
					maxMaterialIdx = pGroup->materialIndex;
			}
		}

		// set for rendering
		m_numVertices = numVertices;
		m_numIndices = numIndices;

		// create hardware buffers
		m_pVB = g_pShaderAPI->CreateVertexBuffer(BUFFER_STATIC, numVertices, sizeof(EGFHwVertex_t), allVerts);
		m_pIB = g_pShaderAPI->CreateIndexBuffer(m_numIndices, nIndexSize, BUFFER_STATIC, allIndices);

		// if we using software skinning, we need to create temporary vertices
		if (m_forceSoftwareSkinning)
		{
			m_softwareVerts = PPAllocStructArray(EGFHwVertex_t, m_numVertices);

			memcpy(m_softwareVerts, allVerts, sizeof(EGFHwVertex_t) * m_numVertices);
		}

		// done.
		delete[] allVerts;
		delete[] allIndices;
	}

	return true;
}

void CEngineStudioEGF::LoadMotionPackage(const char* filename)
{
	if (m_readyState == MODEL_LOAD_ERROR)
		return;

	if (m_readyState != MODEL_LOAD_OK)
	{
		m_additionalMotionPackages.append(filename);
		return;
	}

	studiohdr_t* pHdr = m_hwdata->studio;

	// Try load default motion file
	m_hwdata->motiondata[m_hwdata->numMotionPackages] = Studio_LoadMotionData(filename, pHdr->numBones);

	if (m_hwdata->motiondata[m_hwdata->numMotionPackages])
		m_hwdata->numMotionPackages++;
	else
		MsgError("Can't open motion data package '%s'!\n", filename);
}

void CEngineStudioEGF::LoadMotionPackages()
{
	studiohdr_t* pHdr = m_hwdata->studio;

	DevMsg(DEVMSG_CORE, "Loading animations for '%s'\n", m_szPath.ToCString());

	// Try load default motion file
	m_hwdata->motiondata[m_hwdata->numMotionPackages] = Studio_LoadMotionData((m_szPath.Path_Strip_Ext() + ".mop").GetData(), pHdr->numBones);

	if (m_hwdata->motiondata[m_hwdata->numMotionPackages])
		m_hwdata->numMotionPackages++;
	else
		DevMsg(DEVMSG_CORE, "Can't open motion data package, skipped\n");

	// load external motion packages if available
	for (int i = 0; i < pHdr->numMotionPackages; i++)
	{
		int nPackages = m_hwdata->numMotionPackages;

		EqString mopPath(m_szPath.Path_Strip_Name() + pHdr->pPackage(i)->packageName + ".mop");

		m_hwdata->motiondata[nPackages] = Studio_LoadMotionData(mopPath.ToCString(), pHdr->numBones);

		if (m_hwdata->motiondata[nPackages])
			m_hwdata->numMotionPackages++;
		else
			MsgError("Can't open motion package '%s'\n", pHdr->pPackage(i)->packageName);
	}

	// load additional external motion packages requested by user
	for (int i = 0; i < m_additionalMotionPackages.numElem(); i++)
	{
		// Try load default motion file
		m_hwdata->motiondata[m_hwdata->numMotionPackages] = Studio_LoadMotionData(m_additionalMotionPackages[i].ToCString(), pHdr->numBones);

		if (m_hwdata->motiondata[m_hwdata->numMotionPackages])
			m_hwdata->numMotionPackages++;
		else
			MsgError("Can't open motion data package '%s'!\n", m_additionalMotionPackages[i].ToCString());
	}
	m_additionalMotionPackages.clear();
}

// loads materials for studio
void CEngineStudioEGF::LoadMaterials()
{
	studiohdr_t* pHdr = m_hwdata->studio;

	bool bError = false;
	{
		// init materials
		m_numMaterials = pHdr->numMaterials;

		memset(m_materials, 0, sizeof(m_materials));

		// try load materials properly
		// this is a source engine - like material loading using material paths
		for (int i = 0; i < m_numMaterials; i++)
		{
			EqString fpath(pHdr->pMaterial(i)->materialname);
			fpath.Path_FixSlashes();

			if (fpath.ToCString()[0] == CORRECT_PATH_SEPARATOR)
				fpath = EqString(fpath.ToCString(), fpath.Length() - 1);

			for (int j = 0; j < pHdr->numMaterialSearchPaths; j++)
			{
				if (!m_materials[i])
				{
					EqString spath(pHdr->pMaterialSearchPath(j)->searchPath);
					spath.Path_FixSlashes();

					if (spath.ToCString()[spath.Length() - 1] == CORRECT_PATH_SEPARATOR)
						spath = spath.Left(spath.Length() - 1);

					EqString extend_path;
					CombinePath(extend_path, 2, spath.ToCString(), fpath.ToCString());

					if (materials->IsMaterialExist(extend_path))
					{
						m_materials[i] = materials->GetMaterial(extend_path.GetData());
						m_materials[i]->Ref_Grab();

						materials->PutMaterialToLoadingQueue(m_materials[i]);

						if (!m_materials[i]->IsError() && !(m_materials[i]->GetFlags() & MATERIAL_FLAG_SKINNED))
							MsgWarning("Warning! Material '%s' shader '%s' for model '%s' is invalid\n", m_materials[i]->GetName(), m_materials[i]->GetShaderName(), m_szPath.ToCString());
					}
				}
			}
		}

		// false-initialization of non-loaded materials
		for (int i = 0; i < m_numMaterials; i++)
		{
			if (!m_materials[i])
			{
				MsgError("Couldn't load model material '%s'\n", pHdr->pMaterial(i)->materialname, m_szPath.ToCString());
				bError = true;

				m_materials[i] = materials->GetMaterial("error");
			}
		}
	}

	int maxMaterialIdx = -1;

	for (int i = 0; i < pHdr->numModels; i++)
	{
		studiomodeldesc_t* pModelDesc = pHdr->pModelDesc(i);

		for (int j = 0; j < pModelDesc->numGroups; j++)
		{
			// add vertices, add indices
			modelgroupdesc_t* pGroup = pModelDesc->pGroup(j);

			if (pGroup->materialIndex > maxMaterialIdx)
				maxMaterialIdx = pGroup->materialIndex;
		}
	}
	
	const int numUsedMaterials = maxMaterialIdx + 1;

	m_hwdata->numUsedMaterials = numUsedMaterials;
	m_hwdata->numMaterialGroups = numUsedMaterials ? m_numMaterials / numUsedMaterials : 0;

	if (bError)
	{
		MsgError("  In following search paths:");

		for (int i = 0; i < pHdr->numMaterialSearchPaths; i++)
			MsgError("   '%s'\n", pHdr->pMaterialSearchPath(i)->searchPath);
	}
}

IMaterial* CEngineStudioEGF::GetMaterial(int materialIdx, int materialGroupIdx)
{
	if (materialIdx == -1)
		return materials->GetDefaultMaterial();

	return m_materials[m_hwdata->numUsedMaterials * materialGroupIdx + materialIdx];
}

void CEngineStudioEGF::LoadSetupBones()
{
	studiohdr_t* pHdr = m_hwdata->studio;

	// Initialize HW data joints
	m_hwdata->joints = PPNew studioJoint_t[pHdr->numBones];

	// parse bones
	for (int i = 0; i < pHdr->numBones; i++)
	{
		// copy all bone data
		bonedesc_t* bone = pHdr->pBone(i);
		auto& joint = m_hwdata->joints[i];

		joint.position = bone->position;
		joint.rotation = bone->rotation;
		joint.parentbone = bone->parent;

		// initialize array because we don't want bugs
		joint.bone_id = i;
		joint.link_id = -1;
		joint.chain_id = -1;

		// copy name
		strcpy(joint.name, bone->name);
	}

	// link joints
	for (int i = 0; i < pHdr->numBones; i++)
	{
		int parent_index = m_hwdata->joints[i].parentbone;

		if (parent_index != -1)
			m_hwdata->joints[parent_index].childs.append(i);
	}

	// setup each bone's transformation
	for (int i = 0; i < m_hwdata->studio->numBones; i++)
	{
		studioJoint_t* bone = &m_hwdata->joints[i];

		// setup transformation
		bone->localTrans = identity4();

		bone->localTrans.setRotation(bone->rotation);
		bone->localTrans.setTranslation(bone->position);

		if (bone->parentbone != -1)
			bone->absTrans = bone->localTrans * m_hwdata->joints[bone->parentbone].absTrans;
		else
			bone->absTrans = bone->localTrans;
	}
}

void CEngineStudioEGF::DrawFull()
{
	if (m_numMaterials > 0)
		materials->BindMaterial(m_materials[0]);

	g_pShaderAPI->SetVertexFormat(g_studioModelCache->GetEGFVertexFormat());

	g_pShaderAPI->SetVertexBuffer(m_pVB, 0);
	g_pShaderAPI->SetIndexBuffer(m_pIB);

	materials->Apply();

	g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, 0, m_numIndices, 0, m_numVertices);
}

int CEngineStudioEGF::SelectLod(float dist_to_camera)
{
	if (r_lodtest.GetInt() != -1)
		return r_lodtest.GetInt();

	int numLods = m_hwdata->studio->numLodParams;

	if (numLods < 2)
		return 0;

	int idealLOD = 0;

	for (int i = r_lodstart.GetInt(); i < numLods; i++)
	{
		if (dist_to_camera > m_hwdata->studio->pLodParams(i)->distance * r_lodscale.GetFloat())
			idealLOD = i;
	}

	return idealLOD;
}

void CEngineStudioEGF::SetupVBOStream(int nStream)
{
	if (m_numVertices == 0)
		return;

	g_pShaderAPI->SetVertexBuffer(m_pVB, nStream);
}

void CEngineStudioEGF::DrawGroup(int nModel, int nGroup, bool preSetVBO)
{
	if (m_numVertices == 0)
		return;

	if (preSetVBO)
	{
		g_pShaderAPI->SetVertexFormat(g_studioModelCache->GetEGFVertexFormat());
		g_pShaderAPI->SetVertexBuffer(m_pVB, 0);
	}

	g_pShaderAPI->SetIndexBuffer(m_pIB);

	materials->Apply();

	auto& groupDesc = m_hwdata->modelrefs[nModel].groupDescs[nGroup];

	int nFirstIndex = groupDesc.firstindex;
	int nIndexCount = groupDesc.indexcount;

	// get primitive type
	int8 nPrimType = m_hwdata->studio->pModelDesc(nModel)->pGroup(nGroup)->primitiveType;

	g_pShaderAPI->DrawIndexedPrimitives((ER_PrimitiveType)nPrimType, nFirstIndex, nIndexCount, 0, m_numVertices);
}

const BoundingBox& CEngineStudioEGF::GetAABB() const
{
	return m_aabb;
}

const char* CEngineStudioEGF::GetName() const
{
	return m_szPath.ToCString();
}

int	CEngineStudioEGF::GetLoadingState() const
{
	g_parallelJobs->CompleteJobCallbacks();
	return m_readyState;
}

studioHwData_t* CEngineStudioEGF::GetHWData() const
{
	while ((!m_hwdata || m_hwdata && !m_hwdata->studio) && GetLoadingState() == MODEL_LOAD_IN_PROGRESS) // wait for hwdata
	{
		Platform_Sleep(1);
	}

	return m_hwdata;
}

// instancing
void CEngineStudioEGF::SetInstancer(IEqModelInstancer* instancer)
{
	m_instancer = instancer;

	if (m_instancer)
		m_instancer->ValidateAssert();
}

IEqModelInstancer* CEngineStudioEGF::GetInstancer() const
{
	return m_instancer;
}

bool egf_vertex_comp(const EGFHwVertex_t& a, const EGFHwVertex_t& b)
{
	return (a.pos == b.pos) && (a.texcoord == b.texcoord);
}

void MakeDecalTexCoord(Array<EGFHwVertex_t>& verts, Array<int>& indices, const decalmakeinfo_t& info)
{
	Vector3D fSize = info.size * 2.0f;

	if (info.flags & MAKEDECAL_FLAG_TEX_NORMAL)
	{
		int texSizeW = 1;
		int texSizeH = 1;

		ITexture* pTex = info.material->GetBaseTexture();

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

		Matrix3x3 rotationMat = rotateZXY3(0.0f, 0.0f, DEG2RAD(info.texRotation));
		texMatrix = rotationMat * texMatrix;

		uaxis = texMatrix.rows[0];
		vaxis = texMatrix.rows[1];

		for (int i = 0; i < verts.numElem(); i++)
		{
			float one_over_w = 1.0f / fabs(dot(fSize, uaxis * uaxis) * info.texScale.x);
			float one_over_h = 1.0f / fabs(dot(fSize, vaxis * vaxis) * info.texScale.y);

			float U, V;

			U = dot(uaxis, info.origin - verts[i].pos.xyz()) * one_over_w + 0.5f;
			V = dot(vaxis, info.origin - verts[i].pos.xyz()) * one_over_h - 0.5f;

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

			float one_over_w = 1.0f / fabs(dot(fSize, Vector3D(verts[i].tangent)));
			float one_over_h = 1.0f / fabs(dot(fSize, Vector3D(verts[i].binormal)));

			Vector3D t, b;
			t = Vector3D(verts[i].tangent);
			b = Vector3D(verts[i].binormal);

			verts[i].texcoord.x = fabs(dot(info.origin - verts[i].pos.xyz(), t * sign(t)) * one_over_w + 0.5f);
			verts[i].texcoord.y = fabs(dot(info.origin - verts[i].pos.xyz(), b * sign(b)) * one_over_h + 0.5f);
		}
	}

	// it needs TBN refreshing
	for (int i = 0; i < indices.numElem(); i += 3)
	{
		int idxs[3] = { indices[i], indices[i + 1], indices[i + 2] };

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
tempdecal_t* CEngineStudioEGF::MakeTempDecal(const decalmakeinfo_t& info, Matrix4x4* jointMatrices)
{
	if (r_notempdecals.GetBool())
		return nullptr;

	Vector3D decal_origin = info.origin;
	Vector3D decal_normal = info.normal;

	// pick a geometry
	studiohdr_t* pHdr = m_hwdata->studio;
	int nLod = 0;	// do from LOD 0

	Array<EGFHwVertex_t>	verts{ PP_SL };
	Array<int>				indices{ PP_SL };

	Matrix4x4* tempMatrixArray = PPAllocStructArray(Matrix4x4, m_hwdata->studio->numBones);

	if (jointMatrices)
	{
		// Send all matrices as 4x3
		for (int i = 0; i < m_hwdata->studio->numBones; i++)
		{
			// FIXME: kind of slowness
			tempMatrixArray[i] = (!m_hwdata->joints[i].absTrans * jointMatrices[i]);
		}
	}

	BoundingBox bbox(decal_origin - info.size, decal_origin + info.size);

	// we should transform every vertex and find an intersecting ones
	for (int i = 0; i < pHdr->numBodyGroups; i++)
	{
		int nLodableModelIndex = pHdr->pBodyGroups(i)->lodModelIndex;
		int nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];

		while (nLod > 0 && nModDescId != -1)
		{
			nLod--;
			nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];
		}

		if (nModDescId == -1)
			continue;

		for (int j = 0; j < pHdr->pModelDesc(nModDescId)->numGroups; j++)
		{
			modelgroupdesc_t* pGroup = pHdr->pModelDesc(nModDescId)->pGroup(j);

			uint32* pIndices = pGroup->pVertexIdx(0);

			Array<EGFHwVertex_t>	g_verts{ PP_SL };
			Array<int>				g_indices{ PP_SL };
			Array<int>				g_orig_indices{ PP_SL };

			g_verts.resize(pGroup->numIndices);
			g_indices.resize(pGroup->numIndices);

			uint numIndices = (pGroup->primitiveType == EGFPRIM_TRI_STRIP) ? pGroup->numIndices - 2 : pGroup->numIndices;
			uint indexStep = (pGroup->primitiveType == EGFPRIM_TRI_STRIP) ? 1 : 3;

			for (uint32 k = 0; k < numIndices; k += indexStep)
			{
				// skip strip degenerates
				if (pIndices[k] == pIndices[k + 1] || pIndices[k] == pIndices[k + 2] || pIndices[k + 1] == pIndices[k + 2])
					continue;

				int i0, i1, i2;

				int even = k % 2;
				// handle flipped triangles on EGFPRIM_TRI_STRIP
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
				Vector3D normal = (v0.normal + v1.normal + v2.normal) / 3.0f;

				if (bbox.Intersects(vbox) && dot(normal, decal_normal) < 0)
				{
					g_indices.append(g_verts.addUnique(v0, egf_vertex_comp));
					g_indices.append(g_verts.addUnique(v1, egf_vertex_comp));
					g_indices.append(g_verts.addUnique(v2, egf_vertex_comp));

					g_orig_indices.append(pIndices[k]);
					g_orig_indices.append(pIndices[k + 1]);
					g_orig_indices.append(pIndices[k + 2]);
				}
			}

			MakeDecalTexCoord(g_verts, g_indices, info);

			int nStart = verts.numElem();

			verts.resize(g_indices.numElem());
			indices.resize(g_indices.numElem());

			// restore
			for (int k = 0; k < g_indices.numElem(); k++)
			{
				g_verts[g_indices[k]].pos = Vector4D(pGroup->pVertex(g_orig_indices[k])->point, 1.0f);
				indices.append(nStart + g_indices[k]);
			}

			verts.append(g_verts);
		}
	}

	PPFree(tempMatrixArray);

	if (verts.numElem() && indices.numElem())
	{
		tempdecal_t* pDecal = PPNew tempdecal_t;

		pDecal->material = info.material;

		pDecal->flags = DECAL_FLAG_STUDIODECAL;

		pDecal->numVerts = verts.numElem();
		pDecal->numIndices = indices.numElem();

		pDecal->verts = PPAllocStructArray(EGFHwVertex_t, pDecal->numVerts);
		pDecal->indices = PPAllocStructArray(uint16, pDecal->numIndices);

		// copy geometry
		memcpy(pDecal->verts, verts.ptr(), sizeof(EGFHwVertex_t) * pDecal->numVerts);

		for (int i = 0; i < pDecal->numIndices; i++)
			pDecal->indices[i] = indices[i];

		return pDecal;
	}

	return nullptr;
}

//-------------------------------------------------------
//
// CModelCache - model cache
//
//-------------------------------------------------------

static CStudioModelCache s_ModelCache;
IStudioModelCache* g_studioModelCache = &s_ModelCache;

CStudioModelCache::CStudioModelCache()
{
	m_egfFormat = nullptr;
}

void CStudioModelCache::ReloadModels()
{

}

ConVar job_modelLoader("job_modelLoader", "0", "Load models in parallel threads", CV_ARCHIVE);

// caches model and returns it's index
int CStudioModelCache::PrecacheModel(const char* modelName)
{
	if (strlen(modelName) <= 0)
		return CACHE_INVALID_MODEL;

	if (m_egfFormat == nullptr)
		m_egfFormat = g_pShaderAPI->CreateVertexFormat("EGFVertex", g_EGFHwVertexFormat, elementsOf(g_EGFHwVertexFormat));

	int idx = GetModelIndex(modelName);

	if (idx == CACHE_INVALID_MODEL)
	{
		EqString str(modelName);
		str.Path_FixSlashes();
		const int nameHash = StringToHash(str, true);
		
		DevMsg(DEVMSG_CORE, "Loading model '%s'\n", str.ToCString());

		const int cacheIdx = m_cachedList.numElem();

		CEngineStudioEGF* pModel = PPNew CEngineStudioEGF();
		if (pModel->LoadModel(str, job_modelLoader.GetBool()))
		{
			int newIdx = m_cachedList.append(pModel);
			ASSERT(newIdx == cacheIdx);

			pModel->m_cacheIdx = cacheIdx;
			m_cacheIndex.insert(nameHash, cacheIdx);
		}
		else
		{
			delete pModel;
			pModel = nullptr;
		}

		return pModel ? cacheIdx : 0;
	}

	return idx;
}

// returns count of cached models
int	CStudioModelCache::GetCachedModelCount() const
{
	return m_cachedList.numElem();
}

IEqModel* CStudioModelCache::GetModel(int index) const
{
	IEqModel* model = nullptr;

	if (index <= CACHE_INVALID_MODEL)
		model = m_cachedList[0];
	else
		model = m_cachedList[index];

	if (model && model->GetLoadingState() != MODEL_LOAD_ERROR)
		return model;
	else
		return m_cachedList[0];
}

const char* CStudioModelCache::GetModelFilename(IEqModel* pModel) const
{
	return pModel->GetName();
}

int CStudioModelCache::GetModelIndex(const char* modelName) const
{
	EqString str(modelName);
	str.Path_FixSlashes();

	const int hash = StringToHash(str, true);
	auto found = m_cacheIndex.find(hash);
	if (found != m_cacheIndex.end())
	{
		return *found;
	}

	return CACHE_INVALID_MODEL;
}

int CStudioModelCache::GetModelIndex(IEqModel* pModel) const
{
	for (int i = 0; i < m_cachedList.numElem(); i++)
	{
		if (m_cachedList[i] == pModel)
			return i;
	}

	return CACHE_INVALID_MODEL;
}

// decrements reference count and deletes if it's zero
void CStudioModelCache::FreeCachedModel(IEqModel* pModel)
{

}

void CStudioModelCache::ReleaseCache()
{
	for (int i = 0; i < m_cachedList.numElem(); i++)
	{
		if (m_cachedList[i])
		{
			// wait for loading completion
			m_cachedList[i]->GetHWData();
			delete m_cachedList[i];
		}
	}

	m_cachedList.clear();
	m_cacheIndex.clear();

	g_pShaderAPI->DestroyVertexFormat(m_egfFormat);
	m_egfFormat = nullptr;
}

IVertexFormat* CStudioModelCache::GetEGFVertexFormat() const
{
	return m_egfFormat;
}

// prints loaded models to console
void CStudioModelCache::PrintLoadedModels() const
{
	Msg("---MODELS---\n");
	for (int i = 0; i < m_cachedList.numElem(); i++)
	{
		if (m_cachedList[i])
			Msg("%s\n", m_cachedList[i]->GetName());
	}
	Msg("---END MODELS---\n");
}
