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

ConVar r_drawStaticRegionModels("r_drawStaticRegionModels", "1", NULL, CV_CHEAT);

//------------------------------------------------------------------------------------

CLevObjectDef::CLevObjectDef() :
	m_model(NULL),
	m_instData(NULL),
	m_defModel(NULL),
	m_defKeyvalues(NULL)
{
	memset(&m_info, 0, sizeof(levObjectDefInfo_t));
}

CLevObjectDef::~CLevObjectDef()
{
	if(m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
		delete m_instData;

		if(m_model)
		{
			m_model->Ref_Drop();

			if(m_model->Ref_Count() <= 0)
				delete m_model;
		}
	}
	else
	{
		delete m_defKeyvalues;
	}
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

		camDistance = length(m_model->m_bbox.GetSize())+0.5f;
		camera_target = m_model->m_bbox.GetCenter();
	}
	else
	{
		bbox = m_defModel->GetAABB();

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
	bool renderTranslucency = (nRenderFlags & RFLAG_TRANSLUCENCY) > 0;

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

		int nStartLOD = m_defModel->SelectLod( lodDistance ); // lod distance check

#ifdef EDITOR
		int m_bodyGroupFlags = 0x1;
#else
		int m_bodyGroupFlags = 0xFFFFFFFf;
#endif // EDITOR

		studiohdr_t* pHdr = m_defModel->GetHWData()->studio;
		for(int i = 0; i < pHdr->numBodyGroups; i++)
		{
			// check bodygroups for rendering
			if(!(m_bodyGroupFlags & (1 << i)))
				continue;

			int bodyGroupLOD = nStartLOD;
			int nLodModelIdx = pHdr->pBodyGroups(i)->lodModelIndex;
			studiolodmodel_t* lodModel = pHdr->pLodModel(nLodModelIdx);

			int nModDescId = lodModel->modelsIndexes[ bodyGroupLOD ];

			// get the right LOD model number
			while(nModDescId == -1 && bodyGroupLOD > 0)
			{
				bodyGroupLOD--;
				nModDescId = lodModel->modelsIndexes[ bodyGroupLOD ];
			}

			if(nModDescId == -1)
				continue;
	
			studiomodeldesc_t* modDesc = pHdr->pModelDesc(nModDescId);

			// render model groups that in this body group
			for(int j = 0; j < modDesc->numGroups; j++)
			{
				//materials->SetSkinningEnabled(true);

				int materialIndex = modDesc->pGroup(j)->materialIndex;
				materials->BindMaterial( m_defModel->GetMaterial(materialIndex), 0);

				//m_pModel->PrepareForSkinning( m_boneTransforms );
				m_defModel->DrawGroup( nModDescId, j );

				//materials->SetSkinningEnabled(false);
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
	m_phybatches(NULL),
	m_numBatches(0),
	m_verts (NULL),
	m_physVerts(NULL),
	m_indices (NULL),
	m_numVerts (0),
	m_numIndices (0),
	m_vertexBuffer(NULL),
	m_indexBuffer(NULL),
	m_format(NULL),
	m_hasTransparentSubsets(false),
	m_physicsMesh(NULL)
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

	delete [] m_physVerts;
	m_physVerts = NULL;

	delete [] m_indices;
	m_indices = NULL;

	m_numVerts = 0;
	m_numIndices = 0;

	for(int i = 0; i < m_numBatches; i++)
		materials->FreeMaterial(m_batches[i].pMaterial);

	delete [] m_batches;
	delete [] m_phybatches;

	m_phybatches = NULL;
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

	delete m_physicsMesh;
	m_physicsMesh = NULL;

	m_hasTransparentSubsets = false;
}

bool physModelVertexComparator(const Vector3D &a, const Vector3D &b)
{
	return compare_epsilon(a, b, 0.001f);
}

void CLevelModel::GeneratePhysicsData(bool isGround)
{
	if(!m_verts || !m_indices)
		return;

	delete m_physicsMesh;
	m_physicsMesh = NULL;

	delete [] m_phybatches;
	m_phybatches = NULL;

#ifndef EDITOR
	m_phybatches = new lmodel_batch_t[m_numBatches];

	// regenerate mesh
	DkList<Vector3D>	physVerts;
	DkList<uint16>		indices;

	for(int i = 0; i < m_numBatches; i++)
	{
		lmodel_batch_t& rendBatch = m_batches[i];
		lmodel_batch_t& phyBatch = m_phybatches[i];

		// don't forget about material
		phyBatch.pMaterial = rendBatch.pMaterial;

		DkList<Vector3D>	batchverts;
		DkList<uint32>		batchindices;

		batchverts.resize(rendBatch.numVerts);
		batchindices.resize(rendBatch.numIndices);

		for(uint32 j = 0; j < rendBatch.numIndices; j += 3)
		{
			uint16 vtxId0 = m_indices[rendBatch.startIndex+j];
			uint16 vtxId1 = m_indices[rendBatch.startIndex+j+1];
			uint16 vtxId2 = m_indices[rendBatch.startIndex+j+2];

			if(isGround)
			{
				Vector3D normal;
				ComputeTriangleNormal(m_verts[vtxId0].position, m_verts[vtxId1].position, m_verts[vtxId2].position, normal);

				if(dot(normal, vec3_up) < 0.25)
					continue;
			}

			uint16 idx0 = batchverts.addUnique( m_verts[vtxId0].position, physModelVertexComparator );
			uint16 idx1 = batchverts.addUnique( m_verts[vtxId1].position, physModelVertexComparator );
			uint16 idx2 = batchverts.addUnique( m_verts[vtxId2].position, physModelVertexComparator );
			batchindices.append(idx0);
			batchindices.append(idx1);
			batchindices.append(idx2);
		}

		phyBatch.startVertex = physVerts.numElem();
		phyBatch.startIndex = indices.numElem();

		phyBatch.numVerts = batchverts.numElem();
		phyBatch.numIndices = batchindices.numElem();

		physVerts.append(batchverts);

		for(int j = 0; j < batchindices.numElem(); j++)
			indices.append(phyBatch.startVertex + batchindices[j]);
	}

	delete [] m_verts;
	m_verts = NULL;		// no longer to be used

	delete [] m_physVerts;
	delete [] m_indices;

	int numOldVerts = m_numVerts;

	m_numVerts = physVerts.numElem();
	m_numIndices = indices.numElem();

	m_physVerts = new Vector3D[m_numVerts];
	m_indices = new uint16[m_numIndices];

	memcpy(m_physVerts, physVerts.ptr(), sizeof(Vector3D)*m_numVerts);
	memcpy(m_indices, indices.ptr(), sizeof(uint16)*m_numIndices);

	physVerts.clear();
	indices.clear();

	m_physicsMesh = new CEqBulletIndexedMesh(	(ubyte*)m_physVerts, sizeof(Vector3D),
												(ubyte*)m_indices, sizeof(uint16), m_numVerts, m_numIndices);
#else
	m_physicsMesh = new CEqBulletIndexedMesh(	(ubyte*)m_verts + offsetOf(lmodeldrawvertex_t, position), sizeof(lmodeldrawvertex_t),
												(ubyte*)m_indices, sizeof(uint16), m_numVerts, m_numIndices);
#endif // EDITOR

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

		

#ifndef EDITOR
		lmodel_batch_t& batch = m_phybatches[i];
#else
		lmodel_batch_t& batch = m_batches[i];
#endif // EDITOR

		m_physicsMesh->AddSubpart(batch.startIndex, batch.numIndices, batch.startVertex, batch.numVerts, surfParamId);
	}

#ifndef EDITOR
	m_numVerts = numOldVerts;
#endif // EDITOR
}

void CLevelModel::CreateCollisionObject( regionObject_t* ref )
{
	Matrix4x4 transform = transpose( ref->transform );

	bool isGround = ref->def->m_info.modelflags & LMODEL_FLAG_ISGROUND;

	CEqCollisionObject* collObj = new CEqCollisionObject();
	collObj->Initialize(m_physicsMesh);

	collObj->SetOrientation( Quaternion(transform.getRotationComponent()) );
	collObj->SetPosition( transform.getTranslationComponent() );

	// make as ground
	if( isGround )
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
	ref->physObject = collObj;
}

bool CLevelModel::GenereateRenderData()
{
	if(!m_indices || !m_verts)
		return false;

	if((!m_vertexBuffer || !m_indexBuffer || !m_format))
	{
		const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

		if(caps.isInstancingSupported && r_enableLevelInstancing.GetBool())
		{
			VertexFormatDesc_t pFormat[] = {
				{ 0, 3, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_FLOAT },	  // position
				{ 0, 2, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // texcoord 0

				{ 0, 3, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Normal (TC3)

				{ 2, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // quaternion
				{ 2, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // position
			};

			if (!m_format)
				m_format = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));
		}
		else
		{
			VertexFormatDesc_t pFormat[] = {
				{ 0, 3, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_FLOAT },	  // position
				{ 0, 2, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // texcoord 0

				{ 0, 3, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Normal (TC3)
			};

			if (!m_format)
				m_format = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));
		}

		ER_BufferAccess bufferType = BUFFER_STATIC;
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

void CLevelModel::Render(int nDrawFlags, const BoundingBox& aabb)
{
	if(!r_drawStaticRegionModels.GetBool())
		return;

	if(!m_vertexBuffer || !m_indexBuffer || !m_format)
		return;

	bool renderTranslucency = (nDrawFlags & RFLAG_TRANSLUCENCY) > 0;

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	if(!caps.isInstancingSupported && r_enableLevelInstancing.GetBool())
		g_pShaderAPI->Reset(STATE_RESET_VBO);

	g_pShaderAPI->SetVertexFormat(m_format);
	g_pShaderAPI->SetVertexBuffer(m_vertexBuffer, 0);
	g_pShaderAPI->SetVertexBuffer(NULL, 1);
	g_pShaderAPI->SetIndexBuffer(m_indexBuffer);

	for(int i = 0; i < m_numBatches; i++)
	{
		lmodel_batch_t& batch = m_batches[i];

		int matFlags = batch.pMaterial->GetFlags();

		bool isTransparent = (matFlags & MATERIAL_FLAG_TRANSPARENT) > 0;

		if(isTransparent != renderTranslucency)
			continue;

		materials->SetCullMode((nDrawFlags & RFLAG_FLIP_VIEWPORT_X) ? CULL_FRONT : CULL_BACK);

		materials->BindMaterial(batch.pMaterial, 0);

		materials->Apply();

		g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, batch.startIndex, batch.numIndices, 0, m_numVerts);
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
}

//----------------------------------------------------------------
//
//----------------------------------------------------------------

void CLevelModel::GetDecalPolygons( decalprimitives_t& polys, const Matrix4x4& transform )
{
	// transform volume (optimization)
	// BUG: wrong rotation
	//Volume tVolume = transform * volume;
	
	// in game only physics verts are available
	for(int i = 0; i < m_numBatches; i++)
	{
#ifndef EDITOR
		lmodel_batch_t& batch = m_phybatches[i];
#else
		lmodel_batch_t& batch = m_batches[i];
#endif // EDITOR

		if(batch.pMaterial && (batch.pMaterial->GetFlags() & polys.settings.avoidMaterialFlags))
			continue;

		for(uint32 p = 0; p < batch.numIndices; p += 3)
		{
			int si = batch.startIndex + p;

			int i1 = m_indices[si];
			int i2 = m_indices[si+1];
			int i3 = m_indices[si+2];

#ifndef EDITOR
			Vector3D p1 = m_physVerts[i1];
			Vector3D p2 = m_physVerts[i2];
			Vector3D p3 = m_physVerts[i3];
#else
			Vector3D p1 = m_verts[i1].position;
			Vector3D p2 = m_verts[i2].position;
			Vector3D p3 = m_verts[i3].position;
#endif // EDITOR

			// check it using transformed plane, do not transform every vertex of model
			//if(!tVolume.IsTriangleInside(p1,p2,p3))
			//	continue;

			// transform verts
			p1 = (transform * Vector4D(p1, 1.0f)).xyz();
			p2 = (transform * Vector4D(p2, 1.0f)).xyz();
			p3 = (transform * Vector4D(p3, 1.0f)).xyz();

			if(!polys.settings.clipVolume.IsTriangleInside(p1,p2,p3)) // slow version
				continue;

			polys.AddTriangle(p1,p2,p3);
		}
	}
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

		m_batches[i].pMaterial = materials->GetMaterial(pszPath);
		m_batches[i].pMaterial->Ref_Grab();

		if(m_batches[i].pMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT)
			m_hasTransparentSubsets = true;
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

		m_batches[i].pMaterial = materials->GetMaterial(batch.materialname);
		m_batches[i].pMaterial->Ref_Grab();

		if(m_batches[i].pMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT)
			m_hasTransparentSubsets = true;
	}

	if(m_numVerts == 0 || m_numIndices == 0)
		return;

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
		return DrvSynUnits::MaxCoordInUnits;

	float best_dist = DrvSynUnits::MaxCoordInUnits;

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

		float dist = DrvSynUnits::MaxCoordInUnits+1;

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