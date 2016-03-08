//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: Level object
//////////////////////////////////////////////////////////////////////////////////

#include "world.h"
#include "utils/GeomTools.h"
#include "eqPhysics/eqBulletIndexedMesh.h"

extern ConVar r_enableLevelInstancing;

//------------------------------------------------------------------------------------

CLevObjectDef::CLevObjectDef()
{
	m_model = NULL;
	m_instData = NULL;
	m_defModel = NULL;

	memset(&m_info, 0, sizeof(levObjectDefInfo_t));
}

CLevObjectDef::~CLevObjectDef()
{
	if(m_info.type == LOBJ_TYPE_INTERNAL_STATIC && m_model)
	{
		m_model->Ref_Drop();

		if(m_model->Ref_Count() <= 0)
			delete m_model;
	}

	if(m_instData)
		delete m_instData;
}

#ifdef EDITOR

#define PREVIEW_BOX_SIZE 256

void CLevObjectDef::RefreshPreview()
{
	if(!m_dirtyPreview)
		return;

	m_dirtyPreview = false;

	if(!CreatePreview( PREVIEW_BOX_SIZE ))
		return;

	static ITexture* pTempRendertarget = g_pShaderAPI->CreateRenderTarget(PREVIEW_BOX_SIZE, PREVIEW_BOX_SIZE, FORMAT_RGBA8);

	Vector3D	camera_rotation(35,225,0);
	Vector3D	camera_target(0);
	float		camDistance = 0.0f;
	
	BoundingBox bbox;

	if(m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
		bbox = m_model->m_bbox;

		//Msg("RENDER preview for STATIC model\n");
		camDistance = length(m_model->m_bbox.GetSize())+0.5f;
		camera_target = m_model->m_bbox.GetCenter();
	}
	else
	{
		//Msg("RENDER preview for DEF model\n");

		bbox = BoundingBox(m_defModel->GetBBoxMins(),m_defModel->GetBBoxMaxs());

		camDistance = length(bbox.GetSize())+0.5f;
		camera_target = bbox.GetCenter();
	}

	Vector3D forward, right;
	AngleVectors(camera_rotation, &forward, &right);

	Vector3D cam_pos = camera_target - forward*camDistance;

	// setup perspective
	Matrix4x4 mProjMat = perspectiveMatrixY(DEG2RAD(60), PREVIEW_BOX_SIZE, PREVIEW_BOX_SIZE, 0.05f, 512.0f);

	Matrix4x4 mViewMat = rotateZXY4(DEG2RAD(-camera_rotation.x),DEG2RAD(-camera_rotation.y),DEG2RAD(-camera_rotation.z));
	mViewMat.translate(-cam_pos);

	materials->SetMatrix(MATRIXMODE_PROJECTION, mProjMat);
	materials->SetMatrix(MATRIXMODE_VIEW, mViewMat);

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	// setup render
	g_pShaderAPI->ChangeRenderTarget( pTempRendertarget );

	g_pShaderAPI->Clear(true, true, false, ColorRGBA(0.25,0.25,0.25,1.0f));

	Render(0.0f, bbox, true, 0);

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();

	// copy rendertarget
	UTIL_CopyRendertargetToTexture(pTempRendertarget, m_preview);
}
#endif // EDITOR

void CLevObjectDef::Render( float lodDistance, const BoundingBox& bbox, bool preloadMaterials, int nRenderFlags)
{
	if(m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
		if(preloadMaterials)
		{
			m_model->PreloadTextures();
			materials->Wait();
		}

		m_model->Render( nRenderFlags, bbox );
	}
	else
	{
		if( !m_defModel )
			return;

		materials->SetCullMode((nRenderFlags & RFLAG_FLIP_VIEWPORT_X) ? CULL_FRONT : CULL_BACK);

		studiohdr_t* pHdr = m_defModel->GetHWData()->pStudioHdr;

		int nLOD = m_defModel->SelectLod( lodDistance ); // lod distance check

		for(int i = 0; i < pHdr->numbodygroups; i++)
		{
			int bodyGroupLOD = nLOD;

			// TODO: check bodygroups for rendering

			int nLodModelIdx = pHdr->pBodyGroups(i)->lodmodel_index;
			int nModDescId = pHdr->pLodModel(nLodModelIdx)->lodmodels[ bodyGroupLOD ];

			// get the right LOD model number
			while(nModDescId == -1 && bodyGroupLOD > 0)
			{
				bodyGroupLOD--;
				nModDescId = pHdr->pLodModel(nLodModelIdx)->lodmodels[ bodyGroupLOD ];
			}

			if(nModDescId == -1)
				continue;

			// render model groups that in this body group
			for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numgroups; j++)
			{
				//materials->SetSkinningEnabled(true);
				IMaterial* pMaterial = m_defModel->GetMaterial(nModDescId, j);

				if(preloadMaterials)
				{
					materials->PutMaterialToLoadingQueue(pMaterial);
					materials->Wait();
				}

				materials->BindMaterial(pMaterial, false);

				g_pGameWorld->ApplyLighting(bbox);

				//m_pModel->PrepareForSkinning( m_BoneMatrixList );
				m_defModel->DrawGroup( nModDescId, j );
			}
		}
	}
}

//------------------------------------------------------------------------------------

#define LMODEL_VERSION		(2)

struct lmodelbatch_file_s
{
	lmodelbatch_file_s()
	{
		startVertex = 0;
		numVerts = 0;
		startIndex = 0;
		numIndices = 0;
	}

	lmodelbatch_file_s(const lmodel_batch_t& batch)
	{
		startVertex = batch.startVertex;
		numVerts = batch.numVerts;
		startIndex = batch.startIndex;
		numIndices = batch.numIndices;

		memset(materialname, 0, sizeof(materialname));
		strcpy(materialname, batch.pMaterial->GetName());
	}

	int			startVertex;
	int			numVerts;
	int			startIndex;
	int			numIndices;

	char		materialname[260];
};

ALIGNED_TYPE(lmodelbatch_file_s, 1) lmodelbatch_file_t;

// TODO: alignment
struct lmodelhdr_s
{
	int				version;	// LMODEL_VERSION

	int				numVerts;
	int				numIndices;

	int				numBatches;

	BoundingBox		bbox;

	int				unused;	// placement level index (or type)
};

ALIGNED_TYPE(lmodelhdr_s, 1) lmodelhdr_t;

//------------------------------------------------------------------------------------

CLevelModel::CLevelModel() :
	m_batches (NULL),
	m_numBatches (0),
	m_verts (NULL),
	m_indices (NULL),
	m_numVerts (0),
	m_numIndices (0),
	m_vertexBuffer(NULL),
	m_indexBuffer(NULL),
	m_format(NULL)
{
	m_bbox.Reset();
}


CLevelModel::~CLevelModel()
{
	Cleanup();
}

void CLevelModel::Cleanup()
{
	delete [] m_verts;

	m_verts = NULL;

	delete [] m_indices;

	m_indices = NULL;

	m_numVerts = 0;
	m_numIndices = 0;

	for(int i = 0; i < m_numBatches; i++)
		materials->FreeMaterial(m_batches[i].pMaterial);

	delete [] m_batches;

	m_batches = NULL;
	m_numBatches = 0;

	if(m_indexBuffer)
		g_pShaderAPI->DestroyIndexBuffer(m_indexBuffer);

	if(m_vertexBuffer)
		g_pShaderAPI->DestroyVertexBuffer(m_vertexBuffer);

	if(m_format)
		g_pShaderAPI->DestroyVertexFormat(m_format);

	m_indexBuffer = NULL;
	m_vertexBuffer = NULL;
	m_format = NULL;

	m_bbox.Reset();

	for(int i = 0; i < m_batchMeshes.numElem(); i++)
		delete m_batchMeshes[i];

	m_batchMeshes.clear();
}

CEqBulletIndexedMesh* CLevelModel::CreateBulletTriangleMeshFromModelBatch(lmodel_batch_t* batch)
{
	CEqBulletIndexedMesh* pTriMesh = new CEqBulletIndexedMesh(	((ubyte*)m_verts)+offsetOf(lmodeldrawvertex_t, position), sizeof(lmodeldrawvertex_t),
																(ubyte*)m_indices, sizeof(uint16), m_numVerts, m_numIndices);

	return pTriMesh;
}

bool physModelVertexComparator(const lmodeldrawvertex_t &a, const lmodeldrawvertex_t &b)
{
	return compare_epsilon(a.position, b.position, 0.001f);
}

void CLevelModel::GeneratePhysicsData(bool isGround)
{
#ifndef EDITOR

	if(!m_verts || !m_indices)
		return;

	lmodel_batch_t* tempBatches = new lmodel_batch_t[m_numBatches];

	// regenerate mesh
	DkList<lmodeldrawvertex_t>	verts;
	DkList<uint16>				indices;

	for(int i = 0; i < m_numBatches; i++)
	{
		tempBatches[i].startVertex = m_batches[i].startVertex;
		tempBatches[i].startIndex = m_batches[i].startIndex;
		tempBatches[i].numVerts = m_batches[i].numVerts;
		tempBatches[i].numIndices = m_batches[i].numIndices;

		DkList<lmodeldrawvertex_t>	batchverts;
		DkList<uint32>				batchindices;

		batchverts.resize(m_batches[i].numVerts);
		batchindices.resize(m_batches[i].numIndices);

		for(uint32 j = 0; j < m_batches[i].numIndices; j += 3)
		{
			uint16 vtxId0 = m_indices[m_batches[i].startIndex+j];
			uint16 vtxId1 = m_indices[m_batches[i].startIndex+j+1];
			uint16 vtxId2 = m_indices[m_batches[i].startIndex+j+2];

			if(isGround)
			{
				Vector3D normal;
				ComputeTriangleNormal(m_verts[vtxId0].position, m_verts[vtxId1].position, m_verts[vtxId2].position, normal);

				if(dot(normal, vec3_up) < 0.25)
					continue;
			}

			uint16 idx0 = batchverts.addUnique( m_verts[vtxId0], physModelVertexComparator );
			uint16 idx1 = batchverts.addUnique( m_verts[vtxId1], physModelVertexComparator );
			uint16 idx2 = batchverts.addUnique( m_verts[vtxId2], physModelVertexComparator );
			batchindices.append(idx0);
			batchindices.append(idx1);
			batchindices.append(idx2);
		}

		tempBatches[i].startVertex = verts.numElem();
		tempBatches[i].startIndex = indices.numElem();

		tempBatches[i].numVerts = batchverts.numElem();
		tempBatches[i].numIndices = batchindices.numElem();

		verts.append(batchverts);

		for(int j = 0; j < batchindices.numElem(); j++)
			indices.append(tempBatches[i].startVertex + batchindices[j]);
	}

	delete [] m_verts;

	delete [] m_indices;

	int numOldVerts = m_numVerts;

	m_numVerts = verts.numElem();
	m_numIndices = indices.numElem();

	m_verts = new lmodeldrawvertex_t[m_numVerts];
	m_indices = new uint16[m_numIndices];

	memcpy(m_verts, verts.ptr(), sizeof(lmodeldrawvertex_t)*m_numVerts);
	memcpy(m_indices, indices.ptr(), sizeof(uint16)*m_numIndices);

	verts.clear();
	indices.clear();

	CEqBulletIndexedMesh* mesh = new CEqBulletIndexedMesh(	((ubyte*)m_verts)+offsetOf(lmodeldrawvertex_t, position), sizeof(lmodeldrawvertex_t),
															(ubyte*)m_indices, sizeof(uint16), m_numVerts, m_numIndices);

	m_batchMeshes.append(mesh);

	for(int i = 0; i < m_numBatches; i++)
	{
		IMatVar* pVar = m_batches[i].pMaterial->GetMaterialVar("surfaceprops", "default");

		eqPhysSurfParam_t* param = g_pPhysics->FindSurfaceParam( pVar->GetString() );

		if(!param)
		{
			MsgError("FindSurfaceParam: invalid material '%s' in material '%s'\n", pVar->GetString(), m_batches[i].pMaterial->GetName());
			param = g_pPhysics->FindSurfaceParam( "default" );
		}

		int surfParamId = 0;

		if(param)
			surfParamId = param->id;

		mesh->AddSubpart(tempBatches[i].startIndex, tempBatches[i].numIndices, tempBatches[i].startVertex, tempBatches[i].numVerts, surfParamId);
	}

	m_numVerts = numOldVerts;

	delete [] tempBatches;
#endif // EDITOR
}

void CLevelModel::CreateCollisionObjects( CLevelRegion* reg, regionObject_t* ref )
{
	Matrix4x4 mat = GetModelRefRenderMatrix(reg, ref);
	mat = transpose(mat);

	for(int i = 0; i < m_batchMeshes.numElem(); i++)
	{
		CEqCollisionObject* collObj = new CEqCollisionObject();
		collObj->Initialize(m_batchMeshes[i]);

		collObj->SetOrientation( Quaternion(mat.getRotationComponent()) );
		collObj->SetPosition( mat.getTranslationComponent() );

		// make as ground
		if(ref->def->m_info.modelflags & LMODEL_FLAG_ISGROUND)
		{
			collObj->SetContents( OBJECTCONTENTS_SOLID_GROUND );
			collObj->SetRestitution(0.0f);
			collObj->SetFriction(1.0f);
		}
		else
		{
			collObj->SetContents( OBJECTCONTENTS_SOLID_OBJECTS );
			collObj->SetFriction(0.0f);
			collObj->SetRestitution(1.0f);
		}

		collObj->SetCollideMask(0);

		// don't add them to world
		ref->collisionObjects.append(collObj);
	}

}

bool CLevelModel::GenereateRenderData()
{
	if(!m_indices || !m_verts)
		return false;

	if(!m_vertexBuffer || !m_indexBuffer || !m_format)
	{
		const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

		if(caps.isInstancingSupported && r_enableLevelInstancing.GetBool())
		{
			VertexFormatDesc_t pFormat[] = {
				{ 0, 3, VERTEXTYPE_VERTEX, ATTRIBUTEFORMAT_FLOAT },	  // position
				{ 0, 2, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // texcoord 0

				{ 0, 3, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Normal (TC3)

				{ 2, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // quaternion
				{ 2, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // position
			};

			if (!m_format)
				m_format = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));
		}
		else
		{
			VertexFormatDesc_t pFormat[] = {
				{ 0, 3, VERTEXTYPE_VERTEX, ATTRIBUTEFORMAT_FLOAT },	  // position
				{ 0, 2, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // texcoord 0

				{ 0, 3, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Normal (TC3)
			};

			if (!m_format)
				m_format = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));
		}

		BufferAccessType_e bufferType = BUFFER_STATIC;
		int vb_lock_size = m_numVerts;
		int ib_lock_size = m_numIndices;

		DevMsg(DEVMSG_CORE, "Creating model buffer, %d verts %d indices in %d batches\n", m_numVerts, m_numIndices, m_numBatches);

		m_vertexBuffer = g_pShaderAPI->CreateVertexBuffer(bufferType, vb_lock_size, sizeof(lmodeldrawvertex_t), m_verts);
		m_indexBuffer = g_pShaderAPI->CreateIndexBuffer(ib_lock_size, sizeof(uint16), bufferType, m_indices);
	}

	return true;
}

void CLevelModel::PreloadTextures()
{
	for(int i = 0; i < m_numBatches; i++)
	{
		materials->PutMaterialToLoadingQueue(m_batches[i].pMaterial);
	}
}

ConVar r_drawLevelModels("r_drawLevelModels", "1", "Draw level models (region parts)", CV_CHEAT);

void CLevelModel::Render(int nDrawFlags, const BoundingBox& aabb)
{
	if(!r_drawLevelModels.GetBool())
		return;

	if(!m_vertexBuffer || !m_indexBuffer || !m_format)
		return;

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	if(!caps.isInstancingSupported && r_enableLevelInstancing.GetBool())
		g_pShaderAPI->Reset(STATE_RESET_VBO);

	g_pShaderAPI->SetVertexFormat(m_format);
	g_pShaderAPI->SetVertexBuffer(m_vertexBuffer, 0);
	g_pShaderAPI->SetVertexBuffer(NULL, 1);
	g_pShaderAPI->SetIndexBuffer(m_indexBuffer);

	for(int i = 0; i < m_numBatches; i++)
	{
		materials->SetCullMode((nDrawFlags & RFLAG_FLIP_VIEWPORT_X) ? CULL_FRONT : CULL_BACK);

		materials->BindMaterial(m_batches[i].pMaterial, false);

		if (!(caps.isInstancingSupported && r_enableLevelInstancing.GetBool()))
			g_pGameWorld->ApplyLighting( aabb );

		materials->Apply();

		g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, m_batches[i].startIndex, m_batches[i].numIndices, 0, m_numVerts);
	}
}

// releases data but keeps batchs
void CLevelModel::ReleaseData()
{
	if(m_verts)
		delete [] m_verts;

	m_verts = NULL;

	if(m_indices)
		delete [] m_indices;

	m_indices = NULL;

	//m_numVerts = 0;
	//m_numIndices = 0;
}

bool CLevelModel::CreateFrom(dsmmodel_t* pModel)
{
#ifdef EDITOR
	Cleanup();

	DkList<lmodeldrawvertex_t>	vertexdata;
	DkList<uint16>				indexdata;

	m_numBatches = pModel->groups.numElem();
	m_batches = new lmodel_batch_t[m_numBatches];

	m_bbox.Reset();

	for(int i = 0; i < m_numBatches; i++)
	{
		dsmgroup_t* pGroup = pModel->groups[i];

		int nNumVerts = pGroup->verts.numElem();
		int nNumTris = nNumVerts/3;

		m_batches[i].startVertex = vertexdata.numElem();
		m_batches[i].startIndex = indexdata.numElem();

		vertexdata.resize(vertexdata.numElem() + nNumVerts);
		indexdata.resize(indexdata.numElem() + nNumVerts);

		int j;
		for(int jj = 0; jj < nNumTris; jj++)
		{
			j = jj*3;

			// deny creation of bad triangles
			if(	j >= pGroup->verts.numElem() ||
				j+1 >= pGroup->verts.numElem() ||
				j+2 >= pGroup->verts.numElem())
				break;

			// little parralelism
			lmodeldrawvertex_t vtx0;
			vtx0.position = pGroup->verts[j].position;
			vtx0.texcoord = pGroup->verts[j].texcoord;
			vtx0.normal = pGroup->verts[j].normal;

			lmodeldrawvertex_t vtx1;
			vtx1.position = pGroup->verts[j+1].position;
			vtx1.texcoord = pGroup->verts[j+1].texcoord;
			vtx1.normal = pGroup->verts[j+1].normal;

			lmodeldrawvertex_t vtx2;
			vtx2.position = pGroup->verts[j+2].position;
			vtx2.texcoord = pGroup->verts[j+2].texcoord;
			vtx2.normal = pGroup->verts[j+2].normal;

			m_bbox.AddVertex(vtx0.position);
			m_bbox.AddVertex(vtx1.position);
			m_bbox.AddVertex(vtx2.position);

			int idx[3] = {-1,-1,-1};

			// find any vertex
			for(int k = 0; k < vertexdata.numElem(); k++)
			{
				if(idx[0] == -1 && (vertexdata[k] == vtx0))
					idx[0] = k;
				else if(idx[1] == -1 && (vertexdata[k] == vtx1))
					idx[1] = k;
				else if(idx[2] == -1 && (vertexdata[k] == vtx2))
					idx[2] = k;

				if(idx[0] != -1 && idx[1] != -1 && idx[2] != -1)
					break;
			}

			if(idx[0] == -1)
				idx[0] = vertexdata.append(vtx0);

			if(idx[1] == -1)
				idx[1] = vertexdata.append(vtx1);

			if(idx[2] == -1)
				idx[2] = vertexdata.append(vtx2);

			indexdata.append(idx[0]);
			indexdata.append(idx[1]);
			indexdata.append(idx[2]);
		}

		m_batches[i].numIndices = indexdata.numElem() - m_batches[i].startIndex;
		m_batches[i].numVerts = vertexdata.numElem() - m_batches[i].startVertex;

		EqString path(pModel->groups[i]->texture);

		path = path.Path_Strip_Ext();

		FixSlashes((char*)path.GetData());
		char* sub = strstr((char*)path.GetData(), materials->GetMaterialPath());

		char* pszPath = (char*)path.GetData();
		if(sub)
		{
			int diff = (sub - pszPath);

			pszPath += diff + strlen(materials->GetMaterialPath());
		}

		m_batches[i].pMaterial = materials->FindMaterial(pszPath, true);
		m_batches[i].pMaterial->Ref_Grab();
	}

	m_numVerts = vertexdata.numElem();
	m_numIndices = indexdata.numElem();

	m_verts = new lmodeldrawvertex_t[m_numVerts];
	m_indices = new uint16[m_numIndices];

	memcpy(m_verts, vertexdata.ptr(), sizeof(lmodeldrawvertex_t)*m_numVerts);
	memcpy(m_indices, indexdata.ptr(), sizeof(uint16)*m_numIndices);

	return GenereateRenderData();
#else
	return false;
#endif
}

void CLevelModel::Load(IVirtualStream* stream)
{
	lmodelhdr_t hdr;

	stream->Read(&hdr, 1, sizeof(hdr));

	m_bbox = hdr.bbox;
	m_numVerts = hdr.numVerts;
	m_numIndices = hdr.numIndices;
	m_numBatches = hdr.numBatches;

	m_batches = new lmodel_batch_t[m_numBatches];

	// write batchs
	for(int i = 0; i < m_numBatches; i++)
	{
		lmodelbatch_file_t batch;
		stream->Read(&batch, 1, sizeof(batch));

		m_batches[i].numVerts = batch.numVerts;
		m_batches[i].numIndices = batch.numIndices;

		m_batches[i].startVertex = batch.startVertex;
		m_batches[i].startIndex = batch.startIndex;

		m_batches[i].pMaterial = materials->FindMaterial(batch.materialname, true);
		m_batches[i].pMaterial->Ref_Grab();
	}

	m_verts = new lmodeldrawvertex_t[m_numVerts];
	m_indices = new uint16[m_numIndices];

	// write data
	stream->Read(m_verts, 1, sizeof(lmodeldrawvertex_t)*m_numVerts);
	stream->Read(m_indices, 1, sizeof(uint16)*m_numIndices);

	GenereateRenderData();

}

void CLevelModel::Save(IVirtualStream* stream) const
{
	lmodelhdr_t hdr;

	hdr.version = LMODEL_VERSION;
	hdr.bbox = m_bbox;
	hdr.numVerts = m_numVerts;
	hdr.numIndices = m_numIndices;
	hdr.numBatches = m_numBatches;

	stream->Write(&hdr, 1, sizeof(hdr));

	// write batchs
	for(int i = 0; i < m_numBatches; i++)
	{
		lmodelbatch_file_t batch( m_batches[i] );
		stream->Write(&batch, 1, sizeof(lmodelbatch_file_t));
	}

	// write data
	stream->Write(m_verts, 1, sizeof(lmodeldrawvertex_t)*m_numVerts);
	stream->Write(m_indices, 1, sizeof(uint16)*m_numIndices);
}

#ifdef EDITOR
float CLevelModel::Ed_TraceRayDist(const Vector3D& start, const Vector3D& dir)
{
	float f1,f2;

	if(!m_bbox.IntersectsRay(start, dir, f1,f2))
		return MAX_COORD_UNITS;

	float best_dist = MAX_COORD_UNITS;

	bool bDoTwoSided = false;

	for(int i = 0; i < m_numIndices; i+=3)
	{
		if(	i+1 > m_numIndices ||
			i+2 > m_numIndices)
			break;

		Vector3D v0,v1,v2;

		if(	m_indices[i] >= m_numVerts ||
			m_indices[i+1] >= m_numVerts ||
			m_indices[i+2] >= m_numVerts)
			continue;

		v0 = m_verts[m_indices[i]].position;
		v1 = m_verts[m_indices[i+1]].position;
		v2 = m_verts[m_indices[i+2]].position;

		float dist = MAX_COORD_UNITS+1;

		if(IsRayIntersectsTriangle(v0,v1,v2, start, dir, dist, bDoTwoSided))
		{
			if(dist < best_dist && dist > 0)
			{
				best_dist = dist;
			}
		}
	}

	return best_dist;
}

#endif // EDITOR