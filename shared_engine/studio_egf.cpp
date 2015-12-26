//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Generic
//////////////////////////////////////////////////////////////////////////////////

#include "studio_egf.h"
#include "DebugInterface.h"
#include "math/math_util.h"
#include "utils/SmartPtr.h"
#include "utils/GeomTools.h"
#include "physics/IStudioShapeCache.h"
#include "eqParallelJobs.h"

#if !defined(EDITOR) && !defined(NOENGINE) && !defined(NO_GAME)
#include "IEngineGame.h"
#include "IDebugOverlay.h"
#endif

#ifdef NOPHYSICS

static CEmptyStudioShapeCache s_EmptyShapeCache;
IStudioShapeCache* g_pStudioShapeCache = (IStudioShapeCache*)&s_EmptyShapeCache;

#endif

ConVar r_lodtest("r_lodtest", "-1", -1.0f, MAX_MODELLODS, "Studio LOD test", CV_CHEAT);
ConVar r_lodscale("r_lodscale", "1.0","Studio model LOD scale", CV_ARCHIVE);
ConVar r_lodstart("r_lodstart", "0", 0, MAX_MODELLODS, "Studio LOD start index", CV_ARCHIVE);

ConVar r_notempdecals("r_notempdecals", "0", "Disables temp decals\n", CV_CHEAT);
//ConVar r_force_softwareskinning("r_force_softwareskinning", "0", "Forces software skinning", CV_ARCHIVE);

CEngineStudioEGF::CEngineStudioEGF()
{
	m_instancer = NULL;
	m_readyState = EQMODEL_LOAD_ERROR;

	m_bSoftwareSkinned = false;
	m_bSoftwareSkinChanged = false;

	m_pVB = NULL;
	m_pIB = NULL;

	m_pSWVertices = NULL;

	m_numVertices = 0;
	m_numIndices = 0;

	m_nNumGroups = 0;

	m_nNumMaterials = 0;

	m_pHardwareData = NULL;

	m_vBBOX[0] = Vector3D(MAX_COORD_UNITS);
	m_vBBOX[1] = Vector3D(-MAX_COORD_UNITS);
}

CEngineStudioEGF::~CEngineStudioEGF()
{
	DestroyModel();
}

//ConVar r_animtrasnformskipframes("r_animtrasnformskipframes", "10", "Frame skip for animation transforms (huge speedup!)",CV_ARCHIVE);

struct bonequaternion_t
{
	Vector4D	quat;
	Vector4D	origin;
};

// multiplies bone
Vector4D quatMul ( Vector4D &q1, Vector4D &q2 )
{
    Vector3D  im = q1.w * q2.xyz() + q1.xyz() * q2.w + cross ( q1.xyz(), q2.xyz() );
    Vector4D  dt = q1 * q2;
    float re = dot ( dt, Vector4D ( -1.0, -1.0, -1.0, 1.0 ) );

    return Vector4D ( im, re );
}

// vector rotation
Vector4D quatRotate (Vector3D &p, Vector4D &q )
{
    Quaternion temp = Quaternion(q)*Quaternion(0.0f, p.x,p.y,p.z);//quatMul( q, Vector4D ( p, 0.0 ) );

    return (temp * Quaternion( q.w, -q.x, -q.y, -q.z )).asVector4D();//quatMul ( temp, Vector4D ( -q.x, -q.y, -q.z, q.w ) );
}

// transforms bone
Vector3D boneTransf(bonequaternion_t &bq, Vector3D &pos)
{
    return bq.origin.xyz() + quatRotate(pos, bq.quat).xyz();
}

ConVar r_force_softwareskinning("r_force_softwareskinning", "0", "Force software skinning", CV_CHEAT);

// Software vertex transformation, only for compatibility
bool TransformEGFVertex( EGFHwVertex_t& vert, Matrix4x4* pMatrices )
{
	bool bAffected = false;

	Vector3D vPos = vec3_zero;
	Vector3D vNormal = vec3_zero;
	Vector3D vTangent = vec3_zero;
	Vector3D vBinormal = vec3_zero;

	for(int j = 0; j < 4; j++)
	{
		int index = vert.boneIndices[j];

		if(index == -1)
			continue;

		float weight = vert.boneWeights[j];

		bAffected = true;

		vPos += transform4( vert.pos, pMatrices[index] ) * weight;
		vNormal += transform3( Vector3D(vert.normal), pMatrices[index] ) * weight;
		vTangent += transform3( Vector3D(vert.tangent), pMatrices[index] ) * weight;
		vBinormal += transform3( Vector3D(vert.binormal), pMatrices[index] ) * weight;
	}

	if(bAffected)
	{
		vert.pos = vPos;
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
	if(!m_pHardwareData || (m_pHardwareData && !m_pHardwareData->pStudioHdr))
		return false;

	if(m_pHardwareData->pStudioHdr->numbones == 0)
		return false; // model is not animatable, skip

	if(!jointMatrices)
		return false;

	// TODO: SOFTWARE SKINNING FOR BAD SLOWEST FOR SLOW PC's, and we've never fix it, HAHA!

	/*if(g_pShaderAPI->IsSupportsHardwareSkinning() && !r_force_softwareskinning.GetBool())*/
	if(!r_force_softwareskinning.GetBool())
	{
		if(m_bSoftwareSkinChanged)
		{
			EGFHwVertex_t* bufferData = NULL;

			if(m_pVB->Lock( 0, m_numVertices, (void**)&bufferData, false))
			{
				memcpy( bufferData, m_pSWVertices, m_numVertices*sizeof(EGFHwVertex_t));
				m_pVB->Unlock();
			}

			m_bSoftwareSkinChanged = false;
		}

		bonequaternion_t bquats[128];

		// Send all matrices as 4x3
		for(int i = 0; i < m_pHardwareData->pStudioHdr->numbones; i++)
		{
			// FIXME: kind of slowness
			Matrix4x4 toAbsTransform = (!m_pHardwareData->joints[i].absTrans * jointMatrices[i]);

			// cast matrices to quaternions
			// note that quaternions uses transposes matrix set.
			bquats[i].quat = Quaternion(transpose(toAbsTransform).getRotationComponent()).asVector4D();
			bquats[i].origin = Vector4D(toAbsTransform.rows[3].xyz(), 1);
		}

		//g_pShaderAPI->SetVertexShaderConstantVector4DArray(100, (Vector4D*)&bquats[0].quat, m_pHardwareData->pStudioHdr->numbones*2);
		g_pShaderAPI->SetShaderConstantArrayVector4D("Bones", (Vector4D*)&bquats[0].quat, m_pHardwareData->pStudioHdr->numbones*2);

		return true;
	}
	else if(m_bSoftwareSkinned && r_force_softwareskinning.GetBool())
	{
		m_bSoftwareSkinChanged = true;

		Matrix4x4 tempMatrixArray[128];// = PPAllocStructArray(Matrix4x4, m_pHardwareData->pStudioHdr->numbones);

		// Send all matrices as 4x3
		for(int i = 0; i < m_pHardwareData->pStudioHdr->numbones; i++)
		{
			// FIXME: kind of slowness
			tempMatrixArray[i] = (!m_pHardwareData->joints[i].absTrans * jointMatrices[i]);
		}

		EGFHwVertex_t* bufferData = NULL;

		if(m_pVB->Lock( 0, m_numVertices, (void**)&bufferData, false))
		{
			// setup each bone's transformation
			for(register int i = 0; i < m_numVertices; i++)
			{
				EGFHwVertex_t vert = m_pSWVertices[i];

				TransformEGFVertex( vert, tempMatrixArray );

				bufferData[i] = vert;

			}

			//PPFree( tempMatrixArray );

			m_pVB->Unlock();

			return false;
		}
	}

	return false;
}

void CEngineStudioEGF::DestroyModel()
{
	DevMsg(2, "CEngineStudioEGF::DestroyModel()\n");

	// instancer is removed here if set
	if(m_instancer != NULL)
		delete m_instancer;

	m_instancer = NULL;

	g_pShaderAPI->Reset(STATE_RESET_VBO);
	g_pShaderAPI->ApplyBuffers();

	g_pShaderAPI->DestroyVertexBuffer(m_pVB);
	g_pShaderAPI->DestroyIndexBuffer(m_pIB);

	for(int i = 0; i < m_nNumMaterials;i++)
		materials->FreeMaterial(m_pMaterials[i]);

	memset(m_pMaterials, 0, sizeof(m_pMaterials));

	if(m_pHardwareData)
	{
		for(int i = 0; i < 8; i++)
		{
			if(m_pHardwareData->motiondata[i])
				Studio_FreeMotionData(m_pHardwareData->motiondata[i], m_pHardwareData->pStudioHdr->numbones);
		}

		// destroy this model
		g_pStudioShapeCache->DestroyStudioCache( &m_pHardwareData->m_physmodel );

		Studio_FreePhysModel(&m_pHardwareData->m_physmodel);

		hwmodelref_t* pLodModels = m_pHardwareData->modelrefs;

		for(int i = 0; i < m_pHardwareData->pStudioHdr->nummodels; i++)
			PPFree(pLodModels[i].groupdescs);

		PPFree(m_pHardwareData->modelrefs);
		PPFree(m_pHardwareData->joints);
		PPFree(m_pHardwareData->pStudioHdr);

		PPFree(m_pHardwareData);
	}

	m_pHardwareData = NULL;

	if( m_bSoftwareSkinned )
	{
		PPFree(m_pSWVertices);
		m_pSWVertices = NULL;
	}

	m_nNumGroups = 0;
	m_numIndices = 0;
	m_numVertices = 0;
	m_nNumMaterials = 0;

	m_readyState = EQMODEL_LOAD_ERROR;
}

void CEngineStudioEGF::LoadPhysicsData()
{
	EqString podFileName = m_szPath.Path_Strip_Ext();
	podFileName.Append(".pod");

	if( Studio_LoadPhysModel(podFileName.GetData(), &m_pHardwareData->m_physmodel) )
		g_pStudioShapeCache->InitStudioCache( &m_pHardwareData->m_physmodel );
}

extern ConVar r_detaillevel;

void CopyGroupVertexDataToHWList(DkList<EGFHwVertex_t>& hwVtxList, modelgroupdesc_t* pGroup, Vector3D &bboxMin, Vector3D &bboxMax)
{
	// huge speedup
	hwVtxList.resize(hwVtxList.numElem() + pGroup->numvertices);

	for(int32 i = 0; i < pGroup->numvertices; i++)
	{
		studiovertexdesc_t* pVertex = pGroup->pVertex(i);

		EGFHwVertex_t pNewVertex(pVertex);

		hwVtxList.append(pNewVertex);

		POINT_TO_BBOX(pVertex->point, bboxMin, bboxMax);
	}
}

int CopyGroupIndexDataToHWList(DkList<uint16>& hwIdxList, DkList<uint32>& hwIdxList32, modelgroupdesc_t* pGroup, int vertex_add_offset)
{
	// basicly
	int index_size = INDEX_SIZE_SHORT;

	// huge speedup
	hwIdxList.resize(hwIdxList.numElem() + pGroup->numindices);
	hwIdxList32.resize(hwIdxList32.numElem() + pGroup->numindices);

	for(uint32 i = 0; i < pGroup->numindices; i++)
	{
		int index = (*pGroup->pVertexIdx(i));

		index += vertex_add_offset; // always add offset to index (usually it's a current loadedvertices size)

		// check index size
		if( index > SHRT_MAX )
			index_size = INDEX_SIZE_INT;

		if( index_size == INDEX_SIZE_SHORT )
			hwIdxList.append( index );

		hwIdxList32.append( index );
	}

	return index_size;
}

void CEngineStudioEGF::ModelLoaderJob( void* data )
{
	CEngineStudioEGF* model = (CEngineStudioEGF*)data;

	// single-thread version

	if(!model->LoadFromFile())
	{
		model->m_readyState = EQMODEL_LOAD_ERROR;
		model->DestroyModel();
		return;
	}

	if( !model->LoadGenerateVertexBuffer())
	{
		model->m_readyState = EQMODEL_LOAD_ERROR;
		model->DestroyModel();
		return;
	}

	model->LoadMotionPackages();
	model->LoadPhysicsData();
	model->LoadMaterials();

	materials->Wait();

	model->m_readyState = EQMODEL_LOAD_OK;
}

bool CEngineStudioEGF::LoadModel( const char* pszPath, bool useJob )
{
	m_szPath = pszPath;
	m_szPath.Path_FixSlashes();

	// first we switch to loading
	m_readyState = EQMODEL_LOAD_IN_PROGRESS;

	if(useJob)
	{
		g_parallelJobs->AddJob( ModelLoaderJob, this );
		g_parallelJobs->Submit();
		return true;
	}

	// single-thread version

	if(!LoadFromFile())
	{
		m_readyState = EQMODEL_LOAD_ERROR;
		DestroyModel();
		return false;
	}

	if( !LoadGenerateVertexBuffer())
	{
		m_readyState = EQMODEL_LOAD_ERROR;
		DestroyModel();
		return false;
	}

	LoadMotionPackages();
	LoadPhysicsData();
	LoadMaterials();

	materials->Wait();

	m_readyState = EQMODEL_LOAD_OK;

	return true;
}

bool CEngineStudioEGF::LoadFromFile()
{
	studiohdr_t* pHdr = Studio_LoadModel( m_szPath.c_str() );

	if(!pHdr)
		return false; // get out, nothing to load

	// allocate hardware data
	m_pHardwareData = (studiohwdata_t*)PPAlloc(sizeof(studiohwdata_t));
	memset(m_pHardwareData, 0, sizeof(studiohwdata_t));

	m_pHardwareData->pStudioHdr = pHdr;

	return true;
}

bool CEngineStudioEGF::LoadGenerateVertexBuffer()
{
	m_bSoftwareSkinned = r_force_softwareskinning.GetBool();
	m_nNumGroups = 0;

	studiohdr_t* pHdr = m_pHardwareData->pStudioHdr;

	m_pHardwareData->modelrefs = (hwmodelref_t*)PPAlloc(sizeof(hwmodelref_t)*pHdr->nummodels);
	hwmodelref_t* pLodModels = m_pHardwareData->modelrefs;

	{
		// load all group vertices and indices
		DkList<EGFHwVertex_t>	loadedvertices;

		DkList<uint32>			loadedindices;
		DkList<uint16>			loadedindices_short;

		// use index size
		int nIndexSize = INDEX_SIZE_SHORT;

		for(int i = 0; i < pHdr->nummodels; i++)
		{
			studiomodeldesc_t* pModelDesc = pHdr->pModelDesc(i);

			pLodModels[i].groupdescs = (hwgroup_desc_t*)PPAlloc( sizeof(hwgroup_desc_t) * pModelDesc->numgroups );

			for(int j = 0; j < pModelDesc->numgroups; j++)
			{
				// add vertices, add indices
				modelgroupdesc_t* pGroup = pModelDesc->pGroup(j);

				m_nNumGroups++;

				// set lod index
				int lod_first_index = loadedindices.numElem();
				pLodModels[i].groupdescs[j].firstindex = lod_first_index;

				int new_offset = loadedvertices.numElem();

				// copy vertices to new buffer first
				CopyGroupVertexDataToHWList(loadedvertices, pGroup, m_vBBOX[0], m_vBBOX[1]);

				// then using new offset copy indices to buffer
				int nGenIndexSize = CopyGroupIndexDataToHWList(loadedindices_short, loadedindices, pGroup, new_offset);

				// if size is INT, use it
				if(nGenIndexSize > INDEX_SIZE_SHORT)
					nIndexSize = nGenIndexSize;

				// set index count for lod group
				pLodModels[i].groupdescs[j].indexcount = pGroup->numindices;
			}
		}

		// set for rendering
		m_numVertices = loadedvertices.numElem();
		m_numIndices = loadedindices.numElem();

		// Initialize HW data joints
		m_pHardwareData->joints = (joint_t*)PPAlloc(sizeof(joint_t)*pHdr->numbones);

		// parse bones
		for(int i = 0; i < pHdr->numbones; i++)
		{
			// copy all bone data
			bonedesc_t* bone = pHdr->pBone(i);
			m_pHardwareData->joints[i].position = bone->position;
			m_pHardwareData->joints[i].rotation = bone->rotation;
			m_pHardwareData->joints[i].parentbone = bone->parent;

			// initialize array because we don't want bugs
			memset(&m_pHardwareData->joints[i].childs, 0, sizeof(DkList<int>));
			m_pHardwareData->joints[i].bone_id = i;
			m_pHardwareData->joints[i].link_id = -1;
			m_pHardwareData->joints[i].chain_id = -1;

			// copy name
			strcpy(m_pHardwareData->joints[i].name, bone->name);
		}

		// link joints
		for(int i = 0; i < pHdr->numbones; i++)
		{
			int parent_index = m_pHardwareData->joints[i].parentbone;

			if(parent_index != -1)
				m_pHardwareData->joints[parent_index].childs.append(i);
		}

		// setup matrices for bones
		SetupBones();

		// choose the right index size
		void* pIndexData = (nIndexSize == INDEX_SIZE_SHORT) ? loadedindices_short.ptr() : (void*)loadedindices.ptr();
		int nIndexCount = (nIndexSize == INDEX_SIZE_SHORT) ? loadedindices_short.numElem() : loadedindices.numElem();

		// create hardware buffers
		m_pVB = g_pShaderAPI->CreateVertexBuffer( BUFFER_STATIC, loadedvertices.numElem(), sizeof(EGFHwVertex_t), loadedvertices.ptr() );
		m_pIB = g_pShaderAPI->CreateIndexBuffer( nIndexCount, nIndexSize, BUFFER_STATIC, pIndexData );

		// if we using software skinning, we need to create temporary vertices
		if(m_bSoftwareSkinned)
		{
			m_pSWVertices = PPAllocStructArray(EGFHwVertex_t, m_numVertices);

			memcpy(m_pSWVertices, loadedvertices.ptr(), sizeof(EGFHwVertex_t)*m_numVertices);
		}
	}

	// try to load materials
	m_nNumMaterials = pHdr->nummaterials;

	// init materials
	memset(m_pMaterials, 0, sizeof(m_pMaterials));

	// try load materials properly
	// this is a source engine - like material loading using material paths
	for(int i = 0; i < m_nNumMaterials; i++)
	{
        EqString fpath(pHdr->pMaterial(i)->materialname);
		fpath.Path_FixSlashes();

        if(fpath.c_str()[0] == CORRECT_PATH_SEPARATOR)
            fpath = EqString(fpath.c_str(),fpath.GetLength()-1);

		for(int j = 0; j < pHdr->numsearchpathdescs; j++)
		{
			if(!m_pMaterials[i])
			{
                EqString spath(pHdr->pMaterialSearchPath(j)->m_szSearchPathString);
				spath.Path_FixSlashes();

                if(spath.c_str()[spath.GetLength()-1] == CORRECT_PATH_SEPARATOR)
                    spath = spath.Left(spath.GetLength()-1);

				EqString extend_path = spath + CORRECT_PATH_SEPARATOR + fpath;

				if(materials->IsMaterialExist( extend_path.GetData() ))
				{
					m_pMaterials[i] = materials->FindMaterial(extend_path.GetData(), true);

					if(!m_pMaterials[i]->IsError() && !(m_pMaterials[i]->GetFlags() & MATERIAL_FLAG_SKINNED))
						MsgWarning("Warning! Material '%s' shader '%s' for model '%s' is invalid\n", m_pMaterials[i]->GetName(), m_pMaterials[i]->GetShaderName(), m_szPath.c_str());

					m_pMaterials[i]->Ref_Grab();
				}
			}
		}
	}

	bool bError = false;

	// false-initialization of non-loaded materials
	for(int i = 0; i < m_nNumMaterials; i++)
	{
		if(!m_pMaterials[i])
		{
			MsgError( "Couldn't load model material '%s'\n", pHdr->pMaterial(i)->materialname, m_szPath.c_str() );
			bError = true;

			m_pMaterials[i] = materials->FindMaterial("error");
		}
	}

	if(bError)
	{
		MsgError("  In following search paths:");

		for(int i = 0; i < pHdr->numsearchpathdescs; i++)
		{
			MsgError( "   '%s'\n", pHdr->pMaterialSearchPath(i)->m_szSearchPathString );
		}
	}

	return true;
}

void CEngineStudioEGF::LoadMotionPackages()
{
	studiohdr_t* pHdr = m_pHardwareData->pStudioHdr;

	DevMsg(2, "Loading animations for '%s'\n", m_szPath.c_str());

	// Try load default motion file
	m_pHardwareData->motiondata[0] = Studio_LoadMotionData(( m_szPath.Path_Strip_Ext() + ".mop").GetData(), pHdr->numbones);

	if(m_pHardwareData->motiondata[0])
		m_pHardwareData->numMotionPackages++;
	else
		DevMsg(2, "Can't open motion data package, skipped\n");

	// load external motion packages if available
	for(int i = 0; i < pHdr->nummotionpackages; i++)
	{
		int nPackages = m_pHardwareData->numMotionPackages;

		m_pHardwareData->motiondata[nPackages] = Studio_LoadMotionData(pHdr->pPackage(i)->packagename, pHdr->numbones);

		if(m_pHardwareData->motiondata[nPackages])
			m_pHardwareData->numMotionPackages++;
		else
			MsgError("Can't open motion package '%s'\n", pHdr->pPackage(i)->packagename);
	}
}

// loads materials for studio
void CEngineStudioEGF::LoadMaterials()
{
	for(int i = 0; i < m_nNumMaterials; i++)
		materials->PutMaterialToLoadingQueue( m_pMaterials[i] );
}

IMaterial* CEngineStudioEGF::GetMaterial(int nModel, int nTexGroup)
{
	int material_index = m_pHardwareData->pStudioHdr->pModelDesc(nModel)->pGroup(nTexGroup)->materialIndex;

	if(material_index != -1)
		return m_pMaterials[material_index];

	return NULL;
}

void CEngineStudioEGF::SetupBones()
{
	// setup each bone's transformation
	for(int8 i = 0; i < m_pHardwareData->pStudioHdr->numbones; i++)
	{
		joint_t* bone = &m_pHardwareData->joints[i];

		// setup transformation
		bone->localTrans = identity4();

		bone->localTrans.setRotation(bone->rotation);
		bone->localTrans.setTranslation(bone->position);

		if(bone->parentbone != -1)
			bone->absTrans = bone->localTrans*m_pHardwareData->joints[bone->parentbone].absTrans;
		else
			bone->absTrans = bone->localTrans;
	}
}

void CEngineStudioEGF::DrawFull()
{
	if(m_nNumMaterials > 0)
		materials->BindMaterial(m_pMaterials[0]);

	g_pShaderAPI->SetVertexFormat(g_pModelCache->GetEGFVertexFormat());

	g_pShaderAPI->SetVertexBuffer(m_pVB,0);
	g_pShaderAPI->SetIndexBuffer(m_pIB);

	materials->Apply();

	g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, 0, m_numIndices, 0, m_numVertices);
}

int CEngineStudioEGF::SelectLod(float dist_to_camera)
{
	if(r_lodtest.GetInt() != -1)
		return r_lodtest.GetInt();

	if( m_pHardwareData->pStudioHdr->numlodparams < 2)
		return 0;

	int idealLOD = 0;

	for(int i = 1; i < m_pHardwareData->pStudioHdr->numlodparams; i++)
	{
		if(dist_to_camera > m_pHardwareData->pStudioHdr->pLodParams(i)->distance * r_lodscale.GetFloat())
			idealLOD = i;
	}

	return idealLOD;
}

void CEngineStudioEGF::SetupVBOStream( int nStream )
{
	if(m_numVertices == 0)
		return;

	g_pShaderAPI->SetVertexBuffer( m_pVB, nStream);
}

void CEngineStudioEGF::DrawGroup(int nModel, int nGroup, bool preSetVBO)
{
	if(m_numVertices == 0)
		return;

	if(preSetVBO)
	{
		g_pShaderAPI->SetVertexFormat( g_pModelCache->GetEGFVertexFormat() );
		g_pShaderAPI->SetVertexBuffer( m_pVB, 0);
	}

	g_pShaderAPI->SetIndexBuffer( m_pIB );

	materials->Apply();

	int nFirstIndex = m_pHardwareData->modelrefs[nModel].groupdescs[nGroup].firstindex;
	int nIndexCount = m_pHardwareData->modelrefs[nModel].groupdescs[nGroup].indexcount;

	// get primitive type
	int8 nPrimType = m_pHardwareData->pStudioHdr->pModelDesc( nModel )->pGroup( nGroup )->primitiveType;

	g_pShaderAPI->DrawIndexedPrimitives( (PrimitiveType_e)nPrimType, nFirstIndex, nIndexCount, 0, m_numVertices );
}

ConVar r_debugmodels("r_debugmodels","0","Draw debug information for each model",CV_CHEAT);

void CEngineStudioEGF::DrawDebug()
{
	if(!r_debugmodels.GetBool())
		return;

	//g_pShaderAPI->ChangeDepthStateEx(false,false);

	//g_pShaderAPI->Reset(STATE_RESET_SHADER);
	/*
	g_pShaderAPI->DrawSetColor(ColorRGBA(1,0,0,1));
	g_pShaderAPI->DrawLine3D(Vector3D(0),Vector3D(10,0,0));

	g_pShaderAPI->DrawSetColor(ColorRGBA(0,1,0,1));
	g_pShaderAPI->DrawLine3D(Vector3D(0),Vector3D(0,10,0));

	g_pShaderAPI->DrawSetColor(ColorRGBA(0,0,1,1));
	g_pShaderAPI->DrawLine3D(Vector3D(0),Vector3D(0,0,10));
	*/
}

const Vector3D& CEngineStudioEGF::GetBBoxMins() const
{
	return m_vBBOX[0];
}

const Vector3D& CEngineStudioEGF::GetBBoxMaxs() const
{
	return m_vBBOX[1];
}

const char* CEngineStudioEGF::GetName() const
{
	return m_szPath.GetData();//m_pHardwareData->pStudioHdr->modelname;
}

const char* CEngineStudioEGF::GetPath() const
{
	return m_szPath.GetData();
}

studiohwdata_t* CEngineStudioEGF::GetHWData() const
{
	// wait for loading end
	while(m_readyState == EQMODEL_LOAD_IN_PROGRESS)
		Platform_Sleep(1);

	return m_pHardwareData;
}

// instancing
void CEngineStudioEGF::SetInstancer( IEqModelInstancer* instancer )
{
	m_instancer = instancer;

	if(m_instancer)
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

void MakeDecalTexCoord(DkList<EGFHwVertex_t>& verts, DkList<int>& indices, const decalmakeinfo_t &info)
{
	Vector3D fSize = info.size * 2.0f;

	if(info.flags & DECALFLAG_TEXCOORD_BYNORMAL)
	{
		int texSizeW = 1;
		int texSizeH = 1;

		ITexture* pTex = info.pMaterial->GetBaseTexture();

		if(pTex)
		{
			texSizeW = pTex->GetWidth();
			texSizeH = pTex->GetHeight();
		}

		Vector3D uaxis;
		Vector3D vaxis;

		Vector3D axisAngles = VectorAngles(info.normal);
		AngleVectors(axisAngles, NULL, &uaxis, &vaxis);

		vaxis *= -1;

		//VectorVectors(m_surftex.Plane.normal, m_surftex.UAxis.normal, m_surftex.VAxis.normal);

		Matrix3x3 texMatrix(uaxis, vaxis, info.normal);

		Matrix3x3 rotationMat = rotateZXY3( 0.0f, 0.0f, DEG2RAD(info.texRotation));
		texMatrix = rotationMat*texMatrix;

		uaxis = texMatrix.rows[0];
		vaxis = texMatrix.rows[1];

		//debugoverlay->Line3D(info.origin, info.origin + info.normal*10, ColorRGBA(0,0,1,1), ColorRGBA(0,1,0,1), 100500);

		for(int i = 0; i < verts.numElem(); i++)
		{
			float one_over_w = 1.0f / fabs(dot(fSize,uaxis*uaxis) * info.texScale.x);
			float one_over_h = 1.0f / fabs(dot(fSize,vaxis*vaxis) * info.texScale.y);

			float U, V;

			U = dot(uaxis, info.origin-verts[i].pos) * one_over_w+0.5f;
			V = dot(vaxis, info.origin-verts[i].pos) * one_over_h-0.5f;

			verts[i].texcoord.x = U;
			verts[i].texcoord.y = -V;
		}
	}
	else
	{
		// generate new texture coordinates
		for(int i = 0; i < verts.numElem(); i++)
		{
			verts[i].tangent = fastNormalize(verts[i].tangent);
			verts[i].binormal = fastNormalize(verts[i].binormal);
			verts[i].normal = fastNormalize(verts[i].normal);

			float one_over_w = 1.0f / fabs(dot(fSize,Vector3D(verts[i].tangent)));
			float one_over_h = 1.0f / fabs(dot(fSize,Vector3D(verts[i].binormal)));

			verts[i].texcoord.x = fabs(dot(info.origin-verts[i].pos,Vector3D(verts[i].tangent)*sign(verts[i].tangent))*one_over_w+0.5f);
			verts[i].texcoord.y = fabs(dot(info.origin-verts[i].pos,Vector3D(verts[i].binormal)*sign(verts[i].binormal))*one_over_h+0.5f);
		}
	}

	// it needs TBN refreshing
	for(int i = 0; i < indices.numElem(); i+=3)
	{
		int idxs[3] = {indices[i], indices[i+1], indices[i+2]};

		Vector3D t,b,n;

		ComputeTriangleTBN(	verts[idxs[0]].pos,
							verts[idxs[1]].pos,
							verts[idxs[2]].pos,
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
studiotempdecal_t* CEngineStudioEGF::MakeTempDecal( const decalmakeinfo_t& info, Matrix4x4* jointMatrices)
{
	if(r_notempdecals.GetBool())
		return NULL;

	Vector3D decal_origin = info.origin;
	Vector3D decal_normal = info.normal;

	// pick a geometry
	studiohdr_t* pHdr = m_pHardwareData->pStudioHdr;
	int nLod = 0;	// do from LOD 0

	DkList<EGFHwVertex_t>	verts;
	DkList<int>				indices;

	Matrix4x4 tempMatrixArray[128];// = PPAllocStructArray(Matrix4x4, m_pHardwareData->pStudioHdr->numbones);

	if(jointMatrices)
	{
		// Send all matrices as 4x3
		for(int i = 0; i < m_pHardwareData->pStudioHdr->numbones; i++)
		{
			// FIXME: kind of slowness
			tempMatrixArray[i] = (!m_pHardwareData->joints[i].absTrans * jointMatrices[i]);
		}
	}

	BoundingBox bbox(decal_origin - info.size, decal_origin + info.size);

	// we should transform every vertex and find an intersecting ones
	for(int i = 0; i < pHdr->numbodygroups; i++)
	{
		int nLodableModelIndex = pHdr->pBodyGroups(i)->lodmodel_index;
		int nModDescId = pHdr->pLodModel(nLodableModelIndex)->lodmodels[nLod];

		while(nLod > 0 && nModDescId != -1)
		{
			nLod--;
			nModDescId = pHdr->pLodModel(nLodableModelIndex)->lodmodels[nLod];
		}

		if(nModDescId == -1)
			continue;

		for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numgroups; j++)
		{
			modelgroupdesc_t* pGroup = pHdr->pModelDesc(nModDescId)->pGroup(j);

			uint32 *pIndices = pGroup->pVertexIdx(0);

			DkList<EGFHwVertex_t>	g_verts;
			DkList<int>				g_indices;
			DkList<int>				g_orig_indices;

			g_verts.resize(pGroup->numindices);
			g_indices.resize(pGroup->numindices);

			for(uint32 k = 0; k < pGroup->numindices; k+=3)
			{
				EGFHwVertex_t v0(pGroup->pVertex(pIndices[k]));
				EGFHwVertex_t v1(pGroup->pVertex(pIndices[k+1]));
				EGFHwVertex_t v2(pGroup->pVertex(pIndices[k+2]));

				if(jointMatrices)
				{
					TransformEGFVertex(v0, tempMatrixArray);
					TransformEGFVertex(v1, tempMatrixArray);
					TransformEGFVertex(v2, tempMatrixArray);
				}

				BoundingBox vbox;
				vbox.AddVertex(v0.pos);
				vbox.AddVertex(v1.pos);
				vbox.AddVertex(v2.pos);

				// make and check surface normal
				Vector3D normal = (v0.normal + v1.normal + v2.normal) / 3.0f;

				if( bbox.Intersects(vbox) && dot(normal, decal_normal) < 0 )
				{
					g_indices.append( g_verts.addUnique(v0, egf_vertex_comp) );
					g_indices.append( g_verts.addUnique(v1, egf_vertex_comp) );
					g_indices.append( g_verts.addUnique(v2, egf_vertex_comp) );

					g_orig_indices.append(pIndices[k]);
					g_orig_indices.append(pIndices[k+1]);
					g_orig_indices.append(pIndices[k+2]);
				}
			}

			MakeDecalTexCoord( g_verts, g_indices, (decalmakeinfo_t)info );

			int nStart = verts.numElem();

			verts.resize(g_indices.numElem());
			indices.resize(g_indices.numElem());

			// restore
			for(int k = 0; k < g_indices.numElem(); k++)
			{
				g_verts[g_indices[k]].pos = pGroup->pVertex(g_orig_indices[k])->point;
				indices.append( nStart + g_indices[k] );
			}

			verts.append( g_verts );
		}
	}

	if(verts.numElem() && indices.numElem())
	{
		studiotempdecal_t* pDecal = new studiotempdecal_t;

		pDecal->pMaterial = info.pMaterial;

		pDecal->flags = DECAL_FLAG_STUDIODECAL;

		pDecal->numVerts = verts.numElem();
		pDecal->numIndices = indices.numElem();

		pDecal->pVerts = PPAllocStructArray(EGFHwVertex_t, pDecal->numVerts);
		pDecal->pIndices = PPAllocStructArray(uint16, pDecal->numIndices);

		// copy geometry
		memcpy(pDecal->pVerts, verts.ptr(), sizeof(EGFHwVertex_t)*pDecal->numVerts);

		for(int i = 0; i < pDecal->numIndices; i++)
			pDecal->pIndices[i] = indices[i];

		return pDecal;
	}

	return NULL;
}

//-------------------------------------------------------
//
// CModelCache - model cache
//
//-------------------------------------------------------

static CModelCache s_ModelCache;
IModelCache* g_pModelCache = &s_ModelCache;

CModelCache::CModelCache()
{
	m_egfFormat = NULL;
}

void CModelCache::ReloadModels()
{
	g_pShaderAPI->Reset();
	g_pShaderAPI->Apply();

	/*
	for(int i = 0; i < m_pModels.numElem(); i++)
	{
		if(m_pModels[i]->GetHWData()->pStudioHdr)
		{
			m_pModels[i]->DestroyModel();
			((CEngineStudioEGF*)m_pModels[i])->LoadModel();
		}
	}
	*/
}

ConVar job_modelLoader("job_modelLoader", "1", "Load models in parallel threads", CV_ARCHIVE);

// caches model and returns it's index
int CModelCache::PrecacheModel( const char* modelName )
{
	if(strlen(modelName) <= 0)
		return CACHE_INVALID_MODEL;

	if(m_egfFormat == NULL)
	{
		VertexFormatDesc_t pFormat[] = {
			{ 0, 3, VERTEXTYPE_VERTEX, ATTRIBUTEFORMAT_FLOAT },	  // position
			{ 0, 2, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // texcoord 0

			{ 0, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // Tangent (TC1)
			{ 0, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // Binormal (TC2)
			{ 0, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // Normal (TC3)

			{ 0, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // Bone indices (hw skinning), (TC4)
			{ 0, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_HALF }  // Bone weights (hw skinning), (TC5)
		};

		m_egfFormat = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));
	}

	int idx = GetModelIndex( modelName );

	if(idx == CACHE_INVALID_MODEL)
	{
#if !defined(EDITOR) && !defined(NOENGINE) && !defined(NO_GAME)
		IEqModel* pModel = engine->LoadModel(modelName, false);
#else
		CEngineStudioEGF* pModel = new CEngineStudioEGF;
		if(!pModel->LoadModel( modelName, job_modelLoader.GetBool() ))
		{
			delete pModel;
			pModel = NULL;
		}

#endif

		if(!pModel)
			return 0; // return error model index

#if !defined(EDITOR) && !defined(NOENGINE) && !defined(NO_GAME)
		if(gpGlobals && gpGlobals->curtime > 0)
		{
			MsgWarning("WARNING! Late precache of model '%s'\n", modelName );
		}
		else
			pModel->LoadMaterials();
#endif

		return m_pModels.append(pModel);
	}

	return idx;
}

// returns count of cached models
int	CModelCache::GetCachedModelCount() const
{
	return m_pModels.numElem();
}

IEqModel* CModelCache::GetModel(int index) const
{
	IEqModel* model = NULL;

	if(index == -1)
		model = m_pModels[0];

	model = m_pModels[index];

	if(model->GetLoadingState() != EQMODEL_LOAD_ERROR)
		return model;
	else
		return NULL;
}

const char* CModelCache::GetModelFilename(IEqModel* pModel) const
{
	return pModel->GetName();
}

int CModelCache::GetModelIndex(const char* modelName) const
{
	S_MALLOC(str, strlen(modelName)+10);

	strcpy(str, modelName);
	FixSlashes(str);

	for(int i = 0; i < m_pModels.numElem(); i++)
	{
		if(!stricmp(m_pModels[i]->GetName(), str))
			return i;
	}

	return CACHE_INVALID_MODEL;
}

int CModelCache::GetModelIndex(IEqModel* pModel) const
{
	for(int i = 0; i < m_pModels.numElem(); i++)
	{
		if(m_pModels[i] == pModel)
			return i;
	}

	return CACHE_INVALID_MODEL;
}

void CModelCache::ReleaseCache()
{
	for(int i = 0; i < m_pModels.numElem(); i++)
	{
		if(m_pModels[i])
		{
			delete m_pModels[i];
		}
	}

	m_pModels.clear();

	g_pShaderAPI->DestroyVertexFormat(m_egfFormat);
	m_egfFormat = NULL;
}

IVertexFormat* CModelCache::GetEGFVertexFormat() const
{
	return m_egfFormat;
}

// prints loaded models to console
void CModelCache::PrintLoadedModels() const
{
	Msg("---MODELS---\n");
	for(int i = 0; i < m_pModels.numElem(); i++)
	{
		if(m_pModels[i])
			Msg("%s\n", m_pModels[i]->GetName());
	}
	Msg("---END MODELS---\n");
}
