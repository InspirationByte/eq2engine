//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers level data and renderer
//				You might consider it as some ugly code :(
//////////////////////////////////////////////////////////////////////////////////

#include "levfile.h"
#include "level.h"
#include "physics.h"
#include "VirtualStream.h"
#include "world.h"

#include "utils/GeomTools.h"

#include "eqPhysics/eqBulletIndexedMesh.h"
#include "../shared_engine/physics/BulletConvert.h"

using namespace EqBulletUtils;

#pragma todo("LEVEL - add visibility graph chunks")

#define AI_NAVIGATION_ROAD_PRIORITY (1)

ConVar r_enableLevelInstancing("r_enableLevelInstancing", "1", "Enable level models instancing if available", CV_ARCHIVE);

//------------------------------------------------------------------------------------

#define LMODEL_VERSION		(1)

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
	m_numIndices (0)
{
	m_vertexBuffer = NULL;
	m_indexBuffer = NULL;
	m_format = NULL;

	m_level = 0;

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
																(ubyte*)m_indices, sizeof(uint32), m_numVerts, m_numIndices);

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
	DkList<int>					indices;

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
			int vtxId0 = m_indices[m_batches[i].startIndex+j];
			int vtxId1 = m_indices[m_batches[i].startIndex+j+1];
			int vtxId2 = m_indices[m_batches[i].startIndex+j+2];

			if(isGround)
			{
				Vector3D normal;
				ComputeTriangleNormal(m_verts[vtxId0].position, m_verts[vtxId1].position, m_verts[vtxId2].position, normal);

				if(dot(normal, vec3_up) < 0.25)
					continue;
			}

			int idx0 = batchverts.addUnique( m_verts[vtxId0], physModelVertexComparator );
			int idx1 = batchverts.addUnique( m_verts[vtxId1], physModelVertexComparator );
			int idx2 = batchverts.addUnique( m_verts[vtxId2], physModelVertexComparator );
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
	m_indices = new int[m_numIndices];

	memcpy(m_verts, verts.ptr(), sizeof(lmodeldrawvertex_t)*m_numVerts);
	memcpy(m_indices, indices.ptr(), sizeof(uint32)*m_numIndices);

	verts.clear();
	indices.clear();

	CEqBulletIndexedMesh* mesh = new CEqBulletIndexedMesh(	((ubyte*)m_verts)+offsetOf(lmodeldrawvertex_t, position), sizeof(lmodeldrawvertex_t),
															(ubyte*)m_indices, sizeof(uint32), m_numVerts, m_numIndices);
	
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

void CLevelModel::CreateCollisionObjects( CLevelRegion* reg, regobjectref_t* ref )
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
		if(ref->container->m_info.modelflags & LMODEL_FLAG_ISGROUND)
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
		
#ifdef USE_INSTANCING
		if(caps.isInstancingSupported && r_enableLevelInstancing.GetBool())
		{
			VertexFormatDesc_t pFormat[] = {
				{ 0, 3, VERTEXTYPE_VERTEX, ATTRIBUTEFORMAT_FLOAT },	  // position
				{ 0, 2, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // texcoord 0

				{ 0, 3, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Normal (TC3)

				{ 2, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Matrix4x4
				{ 2, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT },
				{ 2, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT },
				{ 2, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT },
			};

			if (!m_format)
				m_format = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));
		}
		else
#endif // USE_INSTANCING
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

		DevMsg(2, "Creating model buffer, %d verts %d indices in %d batches\n", m_numVerts, m_numIndices, m_numBatches);

		m_vertexBuffer = g_pShaderAPI->CreateVertexBuffer(bufferType, vb_lock_size, sizeof(lmodeldrawvertex_t), m_verts);
		m_indexBuffer = g_pShaderAPI->CreateIndexBuffer(ib_lock_size, sizeof(uint32), bufferType, m_indices);
	}

	return true;

	/*
	int* lockIB = NULL;
	if(indices.numElem() && m_indexbuffer->Lock(0, indices.numElem(), (void**)&lockIB, false))
	{
		if(lockIB)
			memcpy(lockIB, indices.ptr(), indices.numElem()*sizeof(int));

		m_indexbuffer->Unlock();
	}

	hfielddrawvertex_t* lockVB = NULL;
	if(verts.numElem() && m_vertexbuffer->Lock(0, verts.numElem(), (void**)&lockVB, false))
	{
		if(lockVB)
			memcpy(lockVB, verts.ptr(), verts.numElem()*sizeof(hfielddrawvertex_t));

		m_vertexbuffer->Unlock();
	}*/
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
#ifdef NO_GAME
	Cleanup();

	DkList<lmodeldrawvertex_t>	vertexdata;
	DkList<int>					indexdata;

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
	m_indices = new int[m_numIndices];

	memcpy(m_verts, vertexdata.ptr(), sizeof(lmodeldrawvertex_t)*m_numVerts);
	memcpy(m_indices, indexdata.ptr(), sizeof(int)*m_numIndices);

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
	m_indices = new int[m_numIndices];

	// write data
	stream->Read(m_verts, 1, sizeof(lmodeldrawvertex_t)*m_numVerts);
	stream->Read(m_indices, 1, sizeof(uint32)*m_numIndices);

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
	stream->Write(m_indices, 1, sizeof(uint32)*m_numIndices);
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
//-----------------------------------------------------------------------------------------

CLevelRegion::CLevelRegion()
{
	m_level = NULL;

	m_render = true;
	m_isLoaded = false;
	m_scriptEventCallbackCalled = true;
	m_queryTimes.SetValue(0);

	m_roads = NULL;
	m_navGrid = NULL;
	m_navGridStateList = NULL;

	m_numRoadCells = 0;

	memset(m_heightfield, 0, sizeof(m_heightfield));
}

CLevelRegion::~CLevelRegion()
{
	Cleanup();

	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i])
			delete m_heightfield[i];
	}
}

Vector3D CLevelRegion::CellToPosition(int x, int y) const
{
	CHeightTileField& defField = *m_heightfield[0];

	hfieldtile_t* tile = defField.GetTile(x, y);

	Vector3D tile_position;

	if(tile)
		tile_position = defField.m_position + Vector3D(x*HFIELD_POINT_SIZE, tile->height*HFIELD_HEIGHT_STEP, y*HFIELD_POINT_SIZE);
	else
		tile_position = defField.m_position + Vector3D(x*HFIELD_POINT_SIZE, 0, y*HFIELD_POINT_SIZE);

	return tile_position;
}

IVector2D CLevelRegion::GetTileAndNeighbourRegion(int x, int y, CLevelRegion** reg) const
{
	CHeightTileField& defField = *m_heightfield[0];

	// if we're out of bounds - try to find neightbour tile
	if(	(x >= defField.m_sizew || y >= defField.m_sizeh) ||
		(x < 0 || y < 0))
	{
		if(defField.m_posidx_x < 0 || defField.m_posidx_y < 0)
		{
			return NULL;
		}

		// only -1/+1, no more
		int ofs_x = (x < 0) ? -1 : ((x >= defField.m_sizew) ? 1 : 0 );
		int ofs_y = (y < 0) ? -1 : ((y >= defField.m_sizeh) ? 1 : 0 );

		// достать соседа
		(*reg) = m_level->GetRegionAt(IVector2D(defField.m_posidx_x + ofs_x, defField.m_posidx_y + ofs_y));

		if(*reg)
		{
			// rolling
			int tofs_x = ROLLING_VALUE(x, (*reg)->m_heightfield[0]->m_sizew);
			int tofs_y = ROLLING_VALUE(y, (*reg)->m_heightfield[0]->m_sizeh);

			return IVector2D(tofs_x, tofs_y);
		}
		else
			return IVector2D(0,0);
	}

	(*reg) = (CLevelRegion*)this;

	return IVector2D(x,y);
}
	

Matrix4x4 GetModelRefRenderMatrix(CLevelRegion* reg, regobjectref_t* ref)
{
	// models are usually placed at heightfield 0
	CHeightTileField& defField = *reg->m_heightfield[0];

	Matrix4x4 m = rotateXYZ4(DEG2RAD(ref->rotation.x), DEG2RAD(ref->rotation.y), DEG2RAD(ref->rotation.z));

	Vector3D addHeight(0.0f);

	if(ref->container->m_info.type == LOBJ_TYPE_OBJECT_CFG)
		addHeight.y = -ref->container->m_defModel->GetBBoxMins().y;

	if(ref->tile_dependent)
	{
		hfieldtile_t* tile = defField.GetTile( ref->tile_x, ref->tile_y );

		Vector3D tilePosition(ref->tile_x*HFIELD_POINT_SIZE, tile->height*HFIELD_HEIGHT_STEP, ref->tile_y*HFIELD_POINT_SIZE);
		Vector3D modelPosition = defField.m_position + tilePosition + addHeight;

		if( (ref->container->m_info.modelflags & LMODEL_FLAG_ALIGNTOCELL) &&
			ref->container->m_info.type != LOBJ_TYPE_OBJECT_CFG )
		{
			Vector3D t,b,n;
			defField.GetTileTBN( ref->tile_x, ref->tile_y, t,b,n );
			
			Matrix4x4 tileAngle(Vector4D(b, 0), Vector4D(n, 0), Vector4D(t, 0), Vector4D(0,0,0,1));

			tileAngle = (!tileAngle)*m;
			//Matrix4x4 tileShear(Vector4D(b, 0), Vector4D(0,1,0,0), Vector4D(t, 0), Vector4D(0,0,0,1));

			//Matrix4x4 tilemat = shearY(cosf(atan2f(b.x, b.y)))*tileTBN;

			m = translate(modelPosition)/* * tileShear*/ * tileAngle;
		}
		else
			m = translate(modelPosition)*m;
	}
	else
	{
		m = translate(ref->position + addHeight)*m;
	}

	return m;
}

void RenderModelDef( objectcont_t* obj, float fDistance, const BoundingBox& bbox, bool preloadMaterials = false, int nRenderFlags = 0)
{
	if(!obj)
		return;

	if(obj->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
		if(preloadMaterials)
		{
			obj->m_model->PreloadTextures();
			materials->Wait();
		}

		//debugoverlay->Box3D(bbox.minPoint, bbox.maxPoint, ColorRGBA(1,1,0,0.1));

		obj->m_model->Render( nRenderFlags, bbox );
	}
	else
	{
		if( !obj->m_defModel )
			return;

		materials->SetCullMode((nRenderFlags & RFLAG_FLIP_VIEWPORT_X) ? CULL_FRONT : CULL_BACK);

		studiohdr_t* pHdr = obj->m_defModel->GetHWData()->pStudioHdr;

		int nLOD = obj->m_defModel->SelectLod( fDistance ); // lod distance check

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
				IMaterial* pMaterial = obj->m_defModel->GetMaterial(nModDescId, j);

				if(preloadMaterials)
				{
					materials->PutMaterialToLoadingQueue(pMaterial);
					materials->Wait();
				}

				materials->BindMaterial(pMaterial, false);

				g_pGameWorld->ApplyLighting(bbox);

				//m_pModel->PrepareForSkinning( m_BoneMatrixList );
				obj->m_defModel->DrawGroup( nModDescId, j );
			}
		}
	}
}

#ifdef EDITOR
void CLevelRegion::Ed_Prerender()
{
	for(int i = 0; i < m_objects.numElem(); i++)
	{
		objectcont_t* cont = m_objects[i]->container;

		Matrix4x4 mat = GetModelRefRenderMatrix(this, m_objects[i]);

		DrawDefLightData(mat, m_objects[i]->container->m_lightData, 1.0f);
	}
}
#endif // EDITOR

static Vector3D s_sortCamPos;

int occluderComparator(levOccluderLine_t* const& a, levOccluderLine_t* const& b)
{
	Vector3D am = lerp(a->start, a->end, 0.5f);
	Vector3D bm = lerp(b->start, b->end, 0.5f);
	float ad = lengthSqr(s_sortCamPos-am);
	float bd = lengthSqr(s_sortCamPos-bm);

	return ad>bd;
}

void CLevelRegion::CollectVisibleOccluders( occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition )
{
	static DkList<levOccluderLine_t*> occluders;
	occluders.clear(false);

	for(int i = 0; i < m_occluders.numElem(); i++)
		occluders.append( &m_occluders[i] );

	s_sortCamPos = cameraPosition;

	// sort occluders
	occluders.sort( occluderComparator );

	for(int i = 0; i < occluders.numElem(); i++)
	{
		Vector3D verts[4] = {
			occluders[i]->start,
			occluders[i]->end,
			occluders[i]->start + Vector3D(0, occluders[i]->height, 0),
			occluders[i]->end + Vector3D(0, occluders[i]->height, 0)
		};

		BoundingBox bbox;
		bbox.AddVertices(verts,4);

		bool basicVisibility = frustumOccluders.frustum.IsBoxInside(bbox.minPoint,bbox.maxPoint);

		bool visibleThruOccl = (frustumOccluders.IsSphereVisibleThruOcc(verts[0], 0.0f) ||
								frustumOccluders.IsSphereVisibleThruOcc(verts[1], 0.0f) ||
								frustumOccluders.IsSphereVisibleThruOcc(verts[2], 0.0f) ||
								frustumOccluders.IsSphereVisibleThruOcc(verts[3], 0.0f));

		if( basicVisibility && visibleThruOccl)
		{
			frustumOccluders.occluderSets.append( new occludingVolume_t(occluders[i], cameraPosition) );
		}
	}
}

void CLevelRegion::Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& occlFrustum, int nRenderFlags)
{
#ifndef EDITOR
	if(!m_isLoaded)
		return;
#else
	Volume frustum;
#endif // EDITOR

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		regobjectref_t* ref = m_objects[i];	
		objectcont_t* cont = ref->container;


#ifdef EDITOR
//----------------------------------------------------------------
// IN-EDITOR RENDERER
//----------------------------------------------------------------

		float fDist = length(cameraPosition - ref->position);

		Matrix4x4 mat = GetModelRefRenderMatrix(this, ref);
		frustum.LoadAsFrustum(viewProj * mat);

		if(cont->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			//if( frustum.IsBoxInside(cont->m_model->m_bbox.minPoint, cont->m_model->m_bbox.maxPoint) )
			if( occlFrustum.IsSphereVisible( ref->position, length(ref->bbox.GetSize())) )
			{
				if (caps.isInstancingSupported)
				{
					if(!cont->m_instData)	// make new instancing data
						cont->m_instData = new levObjInstanceData_t;

					cont->m_instData->instances[cont->m_instData->numInstances++].worldTransform = mat;
				}
				else
				{
					float fDist = length(cameraPosition - ref->position);

					materials->SetMatrix(MATRIXMODE_WORLD, mat);
					RenderModelDef(cont, fDist, ref->bbox, false, nRenderFlags);
				}
			}
		}

		else
		{
			materials->SetMatrix(MATRIXMODE_WORLD, mat);

			// render studio model
			//if( frustum.IsBoxInside(cont->m_defModel->GetBBoxMins(), cont->m_defModel->GetBBoxMaxs()) )

			BoundingBox bbox(cont->m_defModel->GetBBoxMins(), cont->m_defModel->GetBBoxMaxs());

			if( occlFrustum.IsSphereVisible( ref->position, length(bbox.GetSize())) )
				RenderModelDef(ref->container, fDist, ref->bbox, false, nRenderFlags);
		}
#else
//----------------------------------------------------------------
// IN-GAME RENDERER
//----------------------------------------------------------------

		if(cont->m_info.type != LOBJ_TYPE_INTERNAL_STATIC)
			continue;

		Matrix4x4 mat = GetModelRefRenderMatrix(this, ref);
		//frustum.LoadAsFrustum(viewProj * mat);

		//if( frustum.IsBoxInside(cont->m_model->m_bbox.minPoint, cont->m_model->m_bbox.maxPoint) )
		if( occlFrustum.IsSphereVisible( ref->position, length(ref->bbox.GetSize())) )
		{
			if(	caps.isInstancingSupported &&
				r_enableLevelInstancing.GetBool())
			{
				cont->m_instData->instances[cont->m_instData->numInstances++].worldTransform = mat;
			}
			else
			{
			/*
				if( ref->object )
				{
					ref->object->Draw( nRenderFlags );
				}
				else	
				{*/
					float fDist = length(cameraPosition - ref->position);
					materials->SetMatrix(MATRIXMODE_WORLD, mat);

					RenderModelDef(cont, fDist, ref->bbox, false, nRenderFlags);
				//}
			}
		}
#endif	// EDITOR
	}

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i])
			m_heightfield[i]->Render( nRenderFlags, occlFrustum );
	}
}

void CLevelRegion::Init()
{
	if(!m_heightfield[0])
		m_heightfield[0] = new CHeightTileFieldRenderable();

#ifdef EDITOR
	for(int i = 1; i < ENGINE_REGION_MAX_HFIELDS; i++)
	{
		m_heightfield[i] = new CHeightTileFieldRenderable();
		m_heightfield[i]->m_fieldIdx = i;
	}
#endif // EDITOR
}

void CLevelRegion::InitRoads()
{
	CHeightTileField& defField = *m_heightfield[0];

	int numRoadCells = defField.m_sizew * defField.m_sizeh;

	if(numRoadCells > 0 && !m_roads)
	{
		m_roads = new levroadcell_t[ numRoadCells ];

		for(int x = 0; x < defField.m_sizew; x++)
		{
			for(int y = 0; y < defField.m_sizeh; y++)
			{
				int idx = y * defField.m_sizew + x;

				m_roads[idx].type = ROADTYPE_NOROAD;
				m_roads[idx].flags = 0;
				m_roads[idx].posX = x;
				m_roads[idx].posY = y;
				m_roads[idx].direction = ROADDIR_NORTH;
			}
		}
	}

	if (!m_navGrid)
	{
		m_navWide = defField.m_sizew*AI_NAVIGATION_GRID_SCALE;
		m_navTall = defField.m_sizeh*AI_NAVIGATION_GRID_SCALE;

		int navGridSize = m_navWide*m_navTall;

		m_navGrid = new ubyte[navGridSize];
		memset(m_navGrid, 0x4, navGridSize);

		m_navGridStateList = new navcell_t[navGridSize];
		memset(m_navGridStateList, 0, navGridSize);
	}
}

void FreeLevObjectRef(regobjectref_t* obj)
{
	if(obj->container->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
		CLevelModel* mod = obj->container->m_model;

		mod->Ref_Drop();

		// the model cannot be removed if it's not loaded with region
		if(mod->Ref_Count() <= 0)
			delete mod;

		for(int i = 0; i < obj->collisionObjects.numElem(); i++)
			g_pPhysics->m_physics.DestroyStaticObject( obj->collisionObjects[i] );
	}
	else
	{
		// delete associated object from game world
		if( g_pGameWorld->IsValidObject(obj->object) )
		{
			g_pGameWorld->RemoveObject( obj->object );
		}

		obj->object = NULL;
	}

	delete obj;
}

void CLevelRegion::Cleanup()
{
	m_level->m_mutex.Lock();

	for(int i = 0; i < GetNumHFields(); i++)
		g_pPhysics->RemoveHeightField( m_heightfield[i] );

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		FreeLevObjectRef( m_objects[i] );
	}

	m_level->m_mutex.Unlock();

	m_objects.clear();
	m_occluders.clear();

	if(m_roads)
		delete [] m_roads;

	m_roads = NULL;

	if(m_navGrid)
		delete [] m_navGrid;

	m_navGrid = NULL;

	if(m_navGridStateList)
		delete [] m_navGridStateList;

	m_navGridStateList = NULL;

	m_numRoadCells = 0;

	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i])
			m_heightfield[i]->CleanRenderData();
	}

	m_isLoaded = false;
	
	m_queryTimes.SetValue(1);
}

bool CLevelRegion::IsRegionEmpty()
{
	if(m_objects.numElem() == 0 && 
		(m_heightfield[0] ? m_heightfield[0]->IsEmpty() : true) && 
		(m_heightfield[1] ? m_heightfield[1]->IsEmpty() : true))
		return true;

	return false;
}

CHeightTileFieldRenderable* CLevelRegion::GetHField( int idx ) const
{
	return m_heightfield[idx];
}

int CLevelRegion::GetNumHFields() const
{
	for(int i = 0; i < ENGINE_REGION_MAX_HFIELDS; i++)
	{
		if(m_heightfield[i] == NULL)
			return i;
	}

	return ENGINE_REGION_MAX_HFIELDS;
}

int	CLevelRegion::GetNumNomEmptyHFields() const
{
	int nCount = 0;

	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i] && !m_heightfield[i]->IsEmpty()  )
			nCount++;
	}

	return nCount;
}

#ifdef EDITOR

float CheckStudioRayIntersection(IEqModel* pModel, Vector3D& ray_start, Vector3D& ray_dir)
{
	BoundingBox bbox(pModel->GetBBoxMins(), pModel->GetBBoxMaxs());

	float f1,f2;
	if(!bbox.IntersectsRay(ray_start, ray_dir, f1,f2))
		return MAX_COORD_UNITS;

	float best_dist = MAX_COORD_UNITS;
	float fraction = 1.0f;

	studiohdr_t* pHdr = pModel->GetHWData()->pStudioHdr;
	int nLod = 0;

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

			for(uint32 k = 0; k < pGroup->numindices; k+=3)
			{
				Vector3D v0,v1,v2;

				v0 = pGroup->pVertex(pIndices[k])->point;
				v1 = pGroup->pVertex(pIndices[k+1])->point;
				v2 = pGroup->pVertex(pIndices[k+2])->point;

				float dist = MAX_COORD_UNITS+1;

				if(IsRayIntersectsTriangle(v0,v1,v2, ray_start, ray_dir, dist))
				{
					if(dist < best_dist && dist > 0)
					{
						best_dist = dist;
						fraction = dist;

						//outPos = lerp(start, end, dist);
					}
				}
			}
		}
	}

	return fraction;
}

int CLevelRegion::Ed_SelectRef(const Vector3D& start, const Vector3D& dir, float& dist)
{
	int bestDistrefIdx = -1;
	float fMaxDist = MAX_COORD_UNITS;

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		Matrix4x4 wmatrix = GetModelRefRenderMatrix(this, m_objects[i]);

		Vector3D tray_start = ((!wmatrix)*Vector4D(start, 1)).xyz();
		Vector3D tray_dir = (!wmatrix.getRotationComponent())*dir;

		float raydist = MAX_COORD_UNITS;

		if(m_objects[i]->container->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			raydist = m_objects[i]->container->m_model->Ed_TraceRayDist(tray_start, tray_dir);
		}
		else
		{
			raydist = CheckStudioRayIntersection(m_objects[i]->container->m_defModel, tray_start, tray_dir);
		}

		if(raydist < fMaxDist)
		{
			fMaxDist = raydist;
			bestDistrefIdx = i;
		}
	}

	dist = fMaxDist;

	return bestDistrefIdx;
}

int CLevelRegion::Ed_ReplaceDefs(objectcont_t* whichReplace, objectcont_t* replaceTo)
{
	int numReplaced = 0;

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		if(m_objects[i]->container == whichReplace)
		{
			m_objects[i]->container = replaceTo;
			numReplaced++;
		}
	}

	return numReplaced;
}

#endif // EDITOR

//-----------------------------------------------------------------------------------------

const int LEVEL_HEIGHTFIELD_SIZE		= 64;
const int LEVEL_HEIGHTFIELD_STEP_SIZE	= (HFIELD_POINT_SIZE*LEVEL_HEIGHTFIELD_SIZE);

CGameLevel::CGameLevel() : 
	m_regions(NULL),
	m_wide(0),
	m_tall(0),
	m_cellsSize(64),
	m_regionOffsets(NULL),
	m_roadOffsets(NULL),
	m_occluderOffsets(NULL),
	m_numRegions(0),
	m_regionDataLumpOffset(0),
	m_roadDataLumpOffset(0),
	m_occluderDataLumpOffset(0),
	m_levelName("Unnamed"),
	m_instanceBuffer(NULL)
{

}

	/*
void CGameLevel::InitOLD(int wide, int tall, IVirtualStream* stream)
{
	m_wide = wide;
	m_tall = tall;
	m_cellsSize = LEVEL_HEIGHTFIELD_SIZE;
	m_regions = new CLevelRegion[m_wide*m_tall]();

	Vector3D center = Vector3D(m_wide*LEVEL_HEIGHTFIELD_STEP_SIZE, 0, m_tall*LEVEL_HEIGHTFIELD_STEP_SIZE)*0.5f - Vector3D(HFIELD_POINT_SIZE, 0, HFIELD_POINT_SIZE)*0.5f;

	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			m_regions[idx].m_heightfield[0]->Init( m_cellsSize, x, y );
			m_regions[idx].m_heightfield[0]->m_position = Vector3D(x*LEVEL_HEIGHTFIELD_STEP_SIZE, 0, y*LEVEL_HEIGHTFIELD_STEP_SIZE) - center;

			m_regions[idx].m_bbox.Reset();
			m_regions[idx].m_bbox.AddVertex(m_regions[idx].m_heightfield[0]->m_position - LEVEL_HEIGHTFIELD_STEP_SIZE/2 - Vector3D(0, 1024, 0));
			m_regions[idx].m_bbox.AddVertex(m_regions[idx].m_heightfield[0]->m_position + LEVEL_HEIGHTFIELD_STEP_SIZE/2 + Vector3D(0, 1024, 0));

			if(stream)
				m_regions[idx][0]->m_heightfield.ReadFromStream(stream);
		}
	}

	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;
			g_pPhysics->AddHeightField( &m_regions[idx].m_heightfield );
		}
	}
}
*/
void FreeLevObjectContainer(objectcont_t* container)
{
	if(container->m_info.type == LOBJ_TYPE_INTERNAL_STATIC
		&& container->m_model)
	{
		container->m_model->Ref_Drop();

		if(container->m_model->Ref_Count() <= 0)
			delete container->m_model;
	}

#ifdef USE_INSTANCING
	if(container->m_instData)
		delete container->m_instData;
#endif // USE_INSTANCING
#ifdef EDITOR
	if(container->m_preview)
		g_pShaderAPI->FreeTexture(container->m_preview);
#endif // EDITOR

	delete container;
}

void CGameLevel::Cleanup()
{
	if(!m_regions)
		return;

	Msg("Unloading level...\n");

	StopThread();

	DevMsg(2, "Stopping loader thread...\n");

	// remove regions first
	int num = m_wide*m_tall;

	DevMsg(2, "Unloading regions...\n");
	for(int i = 0; i < num; i++)
	{
		m_regions[i].Cleanup();
	}

	delete [] m_regions;
	m_regions = NULL;

	if(m_regionOffsets)
		delete [] m_regionOffsets;
	m_regionOffsets = NULL;

	if(m_roadOffsets)
		delete [] m_roadOffsets;
	m_roadOffsets = NULL;

	if(m_occluderOffsets)
		delete [] m_occluderOffsets;
	m_occluderOffsets = NULL;

	DevMsg(2, "Freeing objects...\n");
	for(int i = 0; i < m_objectDefs.numElem(); i++)
	{
		FreeLevObjectContainer(m_objectDefs[i]);
	}

	m_objectDefs.clear();

#ifdef USE_INSTANCING
	if(m_instanceBuffer)
		g_pShaderAPI->DestroyVertexBuffer(m_instanceBuffer);
	m_instanceBuffer = NULL;
#endif // USE_INSTANCING

	m_wide = 0;
	m_tall = 0;
	m_occluderDataLumpOffset = 0;
	m_numRegions = 0;
	m_regionDataLumpOffset = 0;
	m_roadDataLumpOffset = 0;
}

bool CGameLevel::Load(const char* levelname, kvkeybase_t* kvDefs)
{
	IFile* pFile = GetFileSystem()->Open(varargs("levels/%s.lev", levelname), "rb", SP_MOD);

	if(!pFile)
	{
		MsgError("can't find level '%s'\n", levelname);
		return false;
	}

	Cleanup();

	levelhdr_t hdr;

	pFile->Read(&hdr, sizeof(levelhdr_t), 1);

	if(hdr.ident != LEVEL_IDENT)
	{
		MsgError("** Invalid level file '%s'\n", levelname);
		GetFileSystem()->Close(pFile);
		return false;
	}

	if(hdr.version != LEVEL_VERSION)
	{
		MsgError("** '%s' - level is too old and/or unsupported by this game version\n", levelname);
		GetFileSystem()->Close(pFile);
		return false;
	}

	bool isOK = true;

	float startLoadTime = Platform_GetCurrentTime();

	// okay, here we go
	do
	{
		float startLumpTime = Platform_GetCurrentTime();

		levlump_t lump;
		if( pFile->Read(&lump, 1, sizeof(levlump_t)) == 0 )
		{
			isOK = false;
			break;
		}

		if(lump.type == LEVLUMP_ENDMARKER)
		{
			isOK = true;
			break;
		}
		else if(lump.type == LEVLUMP_REGIONINFO)
		{
			DevMsg(2,"LEVLUMP_REGIONINFO size = %d", lump.size);

			ReadRegionInfo(pFile);

			float loadTime = Platform_GetCurrentTime()-startLumpTime;
			DevMsg(2, " took %g seconds\n", loadTime);
		}
		else if(lump.type == LEVLUMP_REGIONS)
		{
			DevMsg(2, "LEVLUMP_REGIONS size = %d\n", lump.size);

			m_regionDataLumpOffset = pFile->Tell();

#ifdef EDITOR
			long lstart = pFile->Tell();

			// read heightfields
			for(int x = 0; x < m_wide; x++)
			{
				for(int y = 0; y < m_tall; y++)
				{
					int idx = y*m_wide+x;

					LoadRegionAt(idx, pFile);
				}
			}

			pFile->Seek(lstart + lump.size, VS_SEEK_SET);
			break;
#else
			// get out from this lump
			pFile->Seek(lump.size, VS_SEEK_CUR);
#endif
		}
		else if(lump.type == LEVLUMP_ROADS)
		{
			DevMsg(2, "LEVLUMP_ROADS size = %d\n", lump.size);

			int lump_pos = pFile->Tell();

			// read road offsets
			m_roadOffsets = new int[m_wide*m_tall];
			pFile->Read(m_roadOffsets, m_wide*m_tall, sizeof(int)); 

			m_roadDataLumpOffset = pFile->Tell();

			pFile->Seek(lump_pos+lump.size, VS_SEEK_SET);
		}
		else if(lump.type == LEVLUMP_OCCLUDERS)
		{
			DevMsg(2, "LEVLUMP_OCCLUDERS size = %d\n", lump.size);

			int lump_pos = pFile->Tell();

			// read road offsets
			m_occluderOffsets = new int[m_wide*m_tall];
			pFile->Read(m_occluderOffsets, m_wide*m_tall, sizeof(int)); 

			m_occluderDataLumpOffset = pFile->Tell();

			pFile->Seek(lump_pos+lump.size, VS_SEEK_SET);
		}
		else if(lump.type == LEVLUMP_OBJECTDEFS)
		{
			DevMsg(2, "LEVLUMP_OBJECTDEFS size = %d\n", lump.size);
#ifndef EDITOR
			MsgWarning("Seems like level uses models from editor list, it means that level is not fully built!\n");
#endif // EDITOR
			ReadObjectDefsLump(pFile, kvDefs);
		}
		else if(lump.type == LEVLUMP_HEIGHTFIELDS)
		{
			DevMsg(2, "LEVLUMP_HEIGHTFIELDS size = %d", lump.size);

			ReadHeightfieldsLump(pFile);

			float loadTime = Platform_GetCurrentTime()-startLumpTime;
			DevMsg(2, " took %g seconds\n", loadTime);
		}
		
		// TODO: other lumps

	}while(true);

#ifndef EDITOR
	StartWorkerThread( "LevelLoaderThread" );
#endif // EDITOR

	GetFileSystem()->Close(pFile);

	float loadTime = Platform_GetCurrentTime()-startLoadTime;
	DevMsg(2, "*** Level file read for %g seconds\n", loadTime);

	m_levelName = levelname;

	return isOK;
}

void CGameLevel::Init(int wide, int tall, int cells, bool clean)
{
	m_wide = wide;
	m_tall = tall;
	m_cellsSize = cells;

	m_regions = new CLevelRegion[m_wide*m_tall];

	if(clean)
	{
		m_regionOffsets = new int[m_wide*m_tall];
		m_roadOffsets = new int[m_wide*m_tall];
		m_occluderOffsets = new int[m_wide*m_tall];
		m_regionDataLumpOffset = 0;
		m_roadDataLumpOffset = 0;
		m_occluderDataLumpOffset = 0;
		m_numRegions = m_wide*m_tall;
	}

	DevMsg(2,"Creating map %d x %d regions (cell count = %d)\n", m_wide, m_tall, m_cellsSize);

	int nStepSize = HFIELD_POINT_SIZE*m_cellsSize;

	Vector3D center = Vector3D(m_wide*nStepSize, 0, m_tall*nStepSize)*0.5f - Vector3D(HFIELD_POINT_SIZE, 0, HFIELD_POINT_SIZE)*0.5f;

	// setup heightfield positions and we've done here
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			if(clean)
			{
				m_regionOffsets[idx] = -1;
				m_roadOffsets[idx] = -1;
				m_regions[idx].m_isLoaded = clean;
			}


			m_regions[idx].m_level = this;
			m_regions[idx].Init();

			for(int i = 0; i < m_regions[idx].GetNumHFields(); i++)
			{

				if(!m_regions[idx].m_heightfield[i])
					continue;

				m_regions[idx].m_heightfield[i]->m_posidx_x = x;
				m_regions[idx].m_heightfield[i]->m_posidx_y = y;
				m_regions[idx].m_heightfield[i]->m_position = Vector3D(x*nStepSize, 0, y*nStepSize) - center;
				m_regions[idx].m_regionIndex = idx;

				// init other things like road data
#ifdef EDITOR
				m_regions[idx].m_heightfield[i]->Init(m_cellsSize, x, y);
			}

			m_regions[idx].InitRoads();
#else
			}
#endif // EDITOR
		}
	}

#ifdef USE_INSTANCING

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	if(!m_instanceBuffer && caps.isInstancingSupported && r_enableLevelInstancing.GetBool())
	{
		m_instanceBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_MODEL_INSTANCES, sizeof(regObjectInstance_t));
		m_instanceBuffer->SetFlags( VERTBUFFER_FLAG_INSTANCEDATA );
	}
#endif // USE_INSTANCING
}

void CGameLevel::ReadRegionInfo(IVirtualStream* stream)
{
	levregionshdr_t hdr;
	stream->Read(&hdr, 1, sizeof(levregionshdr_t));

	m_numRegions = hdr.numRegions;
	
	DevMsg(2,"[FILE] map %d x %d regions (cell count = %d) regs = %d\n", hdr.numRegionsWide, hdr.numRegionsTall, hdr.cellsSize, m_numRegions);

	int gridArraySize = hdr.numRegionsWide* hdr.numRegionsTall;

	// read region offsets
	m_regionOffsets = new int[gridArraySize];
	stream->Read(m_regionOffsets, gridArraySize, sizeof(int));

	// initialize
	Init(hdr.numRegionsWide, hdr.numRegionsTall, hdr.cellsSize, false);
}

void CGameLevel::LoadRegionAt(int regionIndex, IVirtualStream* stream)
{
	if(m_regionOffsets[regionIndex] == -1)
		return;

	int loffset = stream->Tell();

	// Read region data
	int regOffset = m_regionDataLumpOffset + m_regionOffsets[regionIndex];
	stream->Seek(regOffset, VS_SEEK_SET);
	m_regions[regionIndex].ReadLoadRegion(stream, m_objectDefs);

	// read roads
	if(m_roadDataLumpOffset > 0 && m_roadOffsets[regionIndex] != -1)
	{
		int roadsOffset = m_roadDataLumpOffset + m_roadOffsets[regionIndex];
		stream->Seek(roadsOffset, VS_SEEK_SET);

		m_regions[regionIndex].ReadLoadRoads(stream);
	}

	// read occluders
	if(m_occluderDataLumpOffset > 0 && m_occluderOffsets[regionIndex] != -1)
	{
		int occlOffset = m_occluderDataLumpOffset + m_occluderOffsets[regionIndex];
		stream->Seek(occlOffset, VS_SEEK_SET);

		m_regions[regionIndex].ReadLoadOccluders(stream);
	}

	m_regions[regionIndex].m_scriptEventCallbackCalled = false;

	// return back
	stream->Seek(loffset, VS_SEEK_SET);
}

void CGameLevel::PreloadRegion(int x, int y)
{
	// open level file
	IFile* pFile = GetFileSystem()->Open(varargs("levels/%s.lev", m_levelName.c_str()), "rb", SP_MOD);

	if(!pFile)
		return;

	int regIdx = y*m_wide+x;

	LoadRegionAt(regIdx, pFile);

	GetFileSystem()->Close(pFile);
}

bool CGameLevel::Save(const char* levelname, bool final)
{
	IFile* pFile = GetFileSystem()->Open(varargs("levels/%s.lev", levelname), "wb", SP_MOD);

	if(!pFile)
	{
		MsgError("can't open level '%s' for write\n", levelname);
		return false;
	}

	levelhdr_t hdr;
	hdr.ident = LEVEL_IDENT;
	hdr.version = LEVEL_VERSION;

	pFile->Write(&hdr, sizeof(levelhdr_t), 1);

	// write models first if available
	WriteObjectDefsLump( pFile );
	
	// write region info
	WriteLevelRegions( pFile, levelname, final );

	levlump_t endLump;
	endLump.type = LEVLUMP_ENDMARKER;
	endLump.size = 0;

	// write lump header and data
	pFile->Write(&endLump, 1, sizeof(levlump_t));

	GetFileSystem()->Close(pFile);

	m_levelName = levelname;

	return true;
}

//-------------------------------------------------------------------------------------------------

void LoadDefLightData( wlightdata_t& out, kvkeybase_t* sec )
{
	for(int i = 0; i < sec->keys.numElem(); i++)
	{
		if(!stricmp(sec->keys[i]->name, "light"))
		{
			wlight_t light;
			light.position = KV_GetVector4D(sec->keys[i]);
			light.color = KV_GetVector4D(sec->keys[i], 4);

			// transform light position
			//Vector3D lightPos = light.position.xyz();
			//lightPos = (objDefMatrix*Vector4D(lightPos, 1.0f)).xyz();

			//light.position = Vector4D(lightPos, light.position.w);

			out.m_lights.append(light);
		}
		else if(!stricmp(sec->keys[i]->name, "glow"))
		{
			wglow_t light;
			light.position = KV_GetVector4D(sec->keys[i]);
			light.color = KV_GetVector4D(sec->keys[i], 4);
			light.type = KV_GetValueInt(sec->keys[i], 8);

			// transform light position
			//Vector3D lightPos = light.position.xyz();
			//lightPos = (objDefMatrix*Vector4D(lightPos, 1.0f)).xyz();

			//light.position = Vector4D(lightPos, light.position.w);

			out.m_glows.append(light);
		}
	}
}

ConVar r_drawStaticLights("r_drawStaticLights", "1", "Draw static lights", CV_CHEAT);

//
// from car.cpp, pls move
//
extern void DrawLightEffect(const Vector3D& position, const ColorRGBA& color, float size, int type = 0);

bool DrawDefLightData( Matrix4x4& objDefMatrix, const wlightdata_t& data, float brightness )
{
	bool lightsAdded = false;

	if(g_pGameWorld->m_envConfig.lightsType & WLIGHTS_LAMPS)
	{
		for(int i = 0; i < data.m_lights.numElem() && r_drawStaticLights.GetBool(); i++)
		{
			wlight_t light = data.m_lights[i];
			light.color.w *= brightness * g_pGameWorld->m_envConfig.streetLightIntensity;

///-------------------------------------------
			// transform light position
			Vector3D lightPos = light.position.xyz();
			lightPos = (objDefMatrix*Vector4D(lightPos, 1.0f)).xyz();

			light.position = Vector4D(lightPos, light.position.w);
///-------------------------------------------

			lightsAdded = lightsAdded || g_pGameWorld->AddLight( light );
		}

		if(!lightsAdded)
			return false;

		float extraBrightness = 0.0f;
		float extraSizeScale = 1.0f;

		if(g_pGameWorld->m_envConfig.weatherType != WEATHER_TYPE_CLEAR)
		{
			extraBrightness = 0.07f;	// add extra brightness to glows
			extraSizeScale = 1.25f;
		}

		for(int i = 0; i < data.m_glows.numElem(); i++)
		{
			// transform light position
			Vector3D lightPos = data.m_glows[i].position.xyz();
			lightPos = (objDefMatrix*Vector4D(lightPos, 1.0f)).xyz();

			if(g_pGameWorld->m_frustum.IsSphereInside(lightPos, data.m_glows[i].position.w))
			{
				ColorRGBA glowColor = data.m_glows[i].color;

				ColorRGBA extraGlowColor = lerp(glowColor*extraBrightness, Vector4D(extraBrightness), 0.25f);

				DrawLightEffect(lightPos, 
								glowColor * glowColor.w*brightness + extraBrightness, 
								data.m_glows[i].position.w * extraSizeScale, 
								data.m_glows[i].type);
			}
		}
	}

	return true;
}

//-------------------------------------------------------------------------------------------------


void CGameLevel::ReadObjectDefsLump(IVirtualStream* stream, kvkeybase_t* kvDefs)
{
	//long fpos = stream->Tell();

	int numModels = 0;
	int modelNamesSize = 0;
	stream->Read(&numModels, 1, sizeof(int));
	stream->Read(&modelNamesSize, 1, sizeof(int));

	char* modelNamesData = new char[modelNamesSize];

	stream->Read(modelNamesData, 1, modelNamesSize);

	char* modelNamePtr = modelNamesData;

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	for(int i = 0; i < numModels; i++)
	{
		objectcont_t* container = new objectcont_t;

		/*
		levmodelinfo_old_t oldStruct;

		stream->Read(&oldStruct, 1, sizeof(levmodelinfo_old_t));

		container->m_info.level = oldStruct.level;
		container->m_info.modelflags = oldStruct.flags;
		container->m_info.size = oldStruct.size;
		container->m_info.type = oldStruct.type;
		*/

		stream->Read(&container->m_info, 1, sizeof(levmodelinfo_t));

		container->m_name = modelNamePtr;

		if(container->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			CLevelModel* model = new CLevelModel();
			model->Load( stream );
			model->PreloadTextures();

			bool isGroundModel = (container->m_info.modelflags & LMODEL_FLAG_ISGROUND);

			model->GeneratePhysicsData( isGroundModel );

			model->Ref_Grab();

			container->m_model = model;
			container->m_defModel = NULL;

#ifdef USE_INSTANCING
			if(caps.isInstancingSupported && r_enableLevelInstancing.GetBool())
				container->m_instData = new levObjInstanceData_t;
#endif // USE_INSTANCING
		}
		else if(container->m_info.type == LOBJ_TYPE_OBJECT_CFG)
		{
			kvkeybase_t* foundDef = NULL;

			for(int j = 0; j < kvDefs->keys.numElem(); j++)
			{
				if(!kvDefs->keys[j]->IsSection())
					continue;

				if(!stricmp(kvDefs->keys[j]->name, "billboardlist"))
					continue;

				if( KV_GetValueBool(kvDefs->keys[j]->FindKeyBase("IsExist")) )
					continue;

				if(!stricmp(KV_GetValueString(kvDefs->keys[j], 0, "error_no_def"), modelNamePtr))
				{
					foundDef = kvDefs->keys[j];
					foundDef->SetKey("IsExist", "1");
					break;
				}
			}

			kvkeybase_t* modelName = NULL;

			if(foundDef == NULL)
			{
				MsgError("Cannot find object def '%s'\n", modelNamePtr);
			}
			else
				modelName = foundDef->FindKeyBase("model");

			// precache model
			int modelIdx = g_pModelCache->PrecacheModel( KV_GetValueString(modelName, 0, "models/error.egf"));

			if(foundDef)
				container->m_defKeyvalues.MergeFrom( foundDef, true );

			container->m_model = NULL;
			if(foundDef)
				container->m_defType = foundDef->name;
			else
				container->m_defType = "INVALID";

			container->m_defModel = g_pModelCache->GetModel(modelIdx);

#ifdef EDITOR
			LoadDefLightData(container->m_lightData, foundDef);
#endif // EDITOR
		}
		
		m_objectDefs.append(container);

		// valid?
		modelNamePtr += strlen(modelNamePtr)+1;
	}

	for(int i = 0; i < kvDefs->keys.numElem(); i++)
	{
		if(!kvDefs->keys[i]->IsSection())
			continue;

		if(!stricmp(kvDefs->keys[i]->name, "billboardlist"))
			continue;

		bool isAlreadyAdded = KV_GetValueBool(kvDefs->keys[i]->FindKeyBase("IsExist"), 0, false);

		if(isAlreadyAdded)
			continue;

		kvkeybase_t* modelName = kvDefs->keys[i]->FindKeyBase("model");

		// precache model
		int modelIdx = g_pModelCache->PrecacheModel( KV_GetValueString(modelName, 0, "models/error.egf"));

		objectcont_t* container = new objectcont_t;
		container->m_name = KV_GetValueString(kvDefs->keys[i], 0, "unnamed_def");

		MsgInfo("Adding object definition '%s'\n", container->m_name.c_str());

		container->m_defKeyvalues.MergeFrom( kvDefs->keys[i], true );

		container->m_info.type = LOBJ_TYPE_OBJECT_CFG;

		container->m_model = NULL;
		container->m_defModel = g_pModelCache->GetModel(modelIdx);

		m_objectDefs.append(container);
	}

	delete [] modelNamesData;
}

void CGameLevel::WriteObjectDefsLump(IVirtualStream* stream)
{
	// if(m_finalBuild)
	//	return;

	if(m_objectDefs.numElem() == 0)
		return;

	CMemoryStream modelsLump;
	modelsLump.Open(NULL, VS_OPEN_WRITE, 2048);

	int numModels = m_objectDefs.numElem();

	modelsLump.Write(&numModels, 1, sizeof(int));

	// write model names
	CMemoryStream modelNamesData;
	modelNamesData.Open(NULL, VS_OPEN_WRITE, 2048);

	char nullSymbol = '\0';

	for(int i = 0; i < m_objectDefs.numElem(); i++)
	{
		modelNamesData.Print(m_objectDefs[i]->m_name.c_str());
		modelNamesData.Write(&nullSymbol, 1, 1);
	}

	modelNamesData.Write(&nullSymbol, 1, 1);

	int modelNamesSize = modelNamesData.Tell();

	modelsLump.Write(&modelNamesSize, 1, sizeof(int));
	modelsLump.Write(modelNamesData.GetBasePointer(), 1, modelNamesSize);
	
	// write model data
	for(int i = 0; i < m_objectDefs.numElem(); i++)
	{
		CMemoryStream modeldata;
		modeldata.Open(NULL, VS_OPEN_WRITE, 2048);

		if(m_objectDefs[i]->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			m_objectDefs[i]->m_model->Save( &modeldata );
		}
		else if(m_objectDefs[i]->m_info.type == LOBJ_TYPE_OBJECT_CFG)
		{
			// do nothing
		}

		int modelSize = modeldata.Tell();

		m_objectDefs[i]->m_info.size = modelSize;

		modelsLump.Write(&m_objectDefs[i]->m_info, 1, sizeof(levmodelinfo_t));
		modelsLump.Write(modeldata.GetBasePointer(), 1, modelSize);
	}

	// compute lump size
	levlump_t modelLumpInfo;
	modelLumpInfo.type = LEVLUMP_OBJECTDEFS;
	modelLumpInfo.size = modelsLump.Tell();

	// write lump header and data
	stream->Write(&modelLumpInfo, 1, sizeof(levlump_t));
	stream->Write(modelsLump.GetBasePointer(), 1, modelsLump.Tell());
}

void CGameLevel::ReadHeightfieldsLump(IVirtualStream* stream)
{
	for(int i = 0; i < m_numRegions; i++)
	{
		int idx = 0;
		stream->Read(&idx, 1, sizeof(int));

		int numFields = 1;
		stream->Read(&numFields, 1, sizeof(int));

		for(int i = 0; i < numFields; i++)
		{
			if( !m_regions[idx].m_heightfield[i] )
			{
				m_regions[idx].m_heightfield[i] = new CHeightTileFieldRenderable();
				m_regions[idx].m_heightfield[i]->m_fieldIdx = i;

				m_regions[idx].m_heightfield[i]->m_posidx_x = m_regions[idx].m_heightfield[0]->m_posidx_x;
				m_regions[idx].m_heightfield[i]->m_posidx_y = m_regions[idx].m_heightfield[0]->m_posidx_y;
				m_regions[idx].m_heightfield[i]->m_position = m_regions[idx].m_heightfield[0]->m_position;
			}

			m_regions[idx].m_heightfield[i]->Init(m_cellsSize, -1, -1);
			m_regions[idx].m_heightfield[i]->ReadFromStream(stream);
		}
	}
}

void CGameLevel::WriteHeightfieldsLump(IVirtualStream* stream)
{
	CMemoryStream hfielddata;
	hfielddata.Open(NULL, VS_OPEN_WRITE, 16384);

	// build region offsets
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			// write each region if not empty
			if( !m_regions[idx].IsRegionEmpty() )
			{
				// heightfield index
				hfielddata.Write(&idx, 1, sizeof(int));

				int numFields = m_regions[idx].GetNumHFields();

				int numNonEmptyFields = m_regions[idx].GetNumNomEmptyHFields();
				hfielddata.Write(&numNonEmptyFields, 1, sizeof(int));

				// write region heightfield data
				for(int i = 0; i < numFields; i++)
				{
					if(m_regions[idx].m_heightfield[i] && !m_regions[idx].m_heightfield[i]->IsEmpty())
						m_regions[idx].m_heightfield[i]->WriteToStream( &hfielddata );
				}
			}
		}
	}

	// compute lump size
	levlump_t hfieldLumpInfo;
	hfieldLumpInfo.type = LEVLUMP_HEIGHTFIELDS;
	hfieldLumpInfo.size = hfielddata.Tell();

	// write lump header and data
	stream->Write(&hfieldLumpInfo, 1, sizeof(levlump_t));
	stream->Write(hfielddata.GetBasePointer(), 1, hfielddata.Tell());
}

void CGameLevel::WriteLevelRegions(IVirtualStream* file, const char* levelname, bool final)
{
	//long fpos = file->Tell();

	// ---------- LEVLUMP_REGIONINFO ---------- 
	levregionshdr_t regHdr;

	regHdr.numRegionsWide = m_wide;
	regHdr.numRegionsTall = m_tall;
	regHdr.cellsSize = m_cellsSize;
	regHdr.numRegions = 0; // to be proceed

	int* regionOffsetArray = new int[m_wide*m_tall];
	int* roadOffsetArray = new int[m_wide*m_tall];
	int* occluderLstOffsetArray = new int[m_wide*m_tall];

	// allocate writeable data streams
	CMemoryStream regionDataStream;
	regionDataStream.Open(NULL, VS_OPEN_WRITE, 2048);

	CMemoryStream roadDataStream;
	roadDataStream.Open(NULL, VS_OPEN_WRITE, 2048);

	CMemoryStream roadJuncDataStream;
	roadJuncDataStream.Open(NULL, VS_OPEN_WRITE, 2048);

	CMemoryStream occluderDataStream;
	occluderDataStream.Open(NULL, VS_OPEN_WRITE, 2048);

	// build region offsets
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			// write each region if not empty
			if( !m_regions[idx].IsRegionEmpty() )
			{
				regHdr.numRegions++;

				regionOffsetArray[idx] = regionDataStream.Tell();
				roadOffsetArray[idx] = roadDataStream.Tell();
				occluderLstOffsetArray[idx] = occluderDataStream.Tell();

				// write em all
				m_regions[idx].WriteRegionData( &regionDataStream, m_objectDefs, final );
				m_regions[idx].WriteRegionRoads( &roadDataStream );
				m_regions[idx].WriteRegionOccluders( &occluderDataStream );
			}
			else
			{
				regionOffsetArray[idx] = -1;
				roadOffsetArray[idx] = -1;
				occluderLstOffsetArray[idx] = -1;
			}
		}
	}

	// LEVLUMP_REGIONINFO
	{
		CMemoryStream regInfoLump;
		regInfoLump.Open(NULL, VS_OPEN_WRITE, 2048);

		// write regions header
		regInfoLump.Write(&regHdr, 1, sizeof(levregionshdr_t));

		// write region offset array
		regInfoLump.Write(regionOffsetArray, m_wide*m_tall, sizeof(int));

		// compute lump size
		levlump_t regInfoLumpInfo;
		regInfoLumpInfo.type = LEVLUMP_REGIONINFO;
		regInfoLumpInfo.size = regInfoLump.Tell();

		// write lump header and data
		file->Write(&regInfoLumpInfo, 1, sizeof(levlump_t));
		file->Write(regInfoLump.GetBasePointer(), 1, regInfoLump.Tell());
	}

	// LEVLUMP_HEIGHTFIELDS
	{
		WriteHeightfieldsLump( file );
	}

	// LEVLUMP_ROADS
	{
		CMemoryStream roadsLump;
		roadsLump.Open(NULL, VS_OPEN_WRITE, 2048);

		// write road offset array
		roadsLump.Write(roadOffsetArray, m_wide*m_tall, sizeof(int));

		// write road data
		roadsLump.Write(roadDataStream.GetBasePointer(), 1, roadDataStream.Tell());

		// compute lump size
		levlump_t roadDataLumpInfo;
		roadDataLumpInfo.type = LEVLUMP_ROADS;
		roadDataLumpInfo.size = roadsLump.Tell();

		// write lump header and data
		file->Write(&roadDataLumpInfo, 1, sizeof(levlump_t));
		file->Write(roadsLump.GetBasePointer(), 1, roadsLump.Tell());
	}
	/*
	// LEVLUMP_ROADJUNCTIONS
	{
		CMemoryStream roadJunctsLump;
		roadJunctsLump.Open(NULL, VS_OPEN_WRITE, 2048);

		// write road junction offset array
		roadJunctsLump.Write(roadOffsetArray, m_wide*m_tall, sizeof(int));

		// write road junction data
		roadJunctsLump.Write(roadJuncDataLump.GetBasePointer(), 1, roadJuncDataLump.Tell());

		// compute lump size
		levlump_t roadDataLumpInfo;
		roadDataLumpInfo.type = LEVLUMP_ROADJUNCTIONS;
		roadDataLumpInfo.size = roadJunctsLump.Tell();

		// write lump header and data
		file->Write(&roadDataLumpInfo, 1, sizeof(levlump_t));
		file->Write(roadJunctsLump.GetBasePointer(), 1, roadJunctsLump.Tell());
	}*/

	// LEVLUMP_OCCLUDERS
	{
		CMemoryStream occludersLump;
		occludersLump.Open(NULL, VS_OPEN_WRITE, 2048);

		occludersLump.Write(occluderLstOffsetArray, m_wide*m_tall, sizeof(int));
		occludersLump.Write(occluderDataStream.GetBasePointer(), 1, occluderDataStream.Tell());

		levlump_t occludersLumpInfo;
		occludersLumpInfo.type = LEVLUMP_OCCLUDERS;
		occludersLumpInfo.size = occludersLump.Tell();

		// write lump header and data
		file->Write(&occludersLumpInfo, 1, sizeof(levlump_t));

		// write occluder offset array
		file->Write(occludersLump.GetBasePointer(), 1, occludersLump.Tell());
	}
	
	// LEVLUMP_REGIONS
	{
		levlump_t regDataLumpInfo;
		regDataLumpInfo.type = LEVLUMP_REGIONS;
		regDataLumpInfo.size = regionDataStream.Tell();

		// write lump header and data
		file->Write(&regDataLumpInfo, 1, sizeof(levlump_t));
		file->Write(regionDataStream.GetBasePointer(), 1, regionDataStream.Tell());
	}
	

	delete [] regionOffsetArray;
	delete [] roadOffsetArray;
	delete [] occluderLstOffsetArray;

	//-------------------------------------------------------------------------------
}

#ifdef EDITOR
int CGameLevel::Ed_SelectRefAndReg(const Vector3D& start, const Vector3D& dir, CLevelRegion** reg, float& dist)
{
	float max_dist = MAX_COORD_UNITS;
	int bestDistrefIdx = NULL;
	CLevelRegion* bestReg = NULL;

	// build region offsets
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			float refdist = MAX_COORD_UNITS;
			int foundIdx = m_regions[idx].Ed_SelectRef(start, dir, refdist);

			if(foundIdx != -1 && (refdist < max_dist))
			{
				max_dist = refdist;
				bestReg = &m_regions[idx];
				bestDistrefIdx = foundIdx;
			}
		}
	}

	*reg = bestReg;
	dist = max_dist;
	return bestDistrefIdx;
}

#endif

/*
int FindIndexAsLevelModel(DkList<objectcont_t*>& listObjects, CLevelModel* model)
{
	for(int i = 0; i < listObjects.numElem(); i++)
	{
		if(listObjects[i]->m_model == model)
			return i;
	}

	return -1;
}
*/
int FindObjectContainer(DkList<objectcont_t*>& listObjects, objectcont_t* container)
{
	for(int i = 0; i < listObjects.numElem(); i++)
	{
		if( listObjects[i] == container )
			return i;
	}

	ASSERTMSG(false, "Programmer error, cannot find object definition (is editor cached it?)");

	return -1;
}

void CLevelRegion::WriteRegionData( IVirtualStream* stream, DkList<objectcont_t*>& listObjects, bool final )
{
	// create region model lists
	DkList<CLevelModel*>			modelList;
	DkList<levcellmodelobject_t>	cellObjectsList;

	cellObjectsList.resize(m_objects.numElem());

	// collect models and cell objects
	for(int i = 0; i < m_objects.numElem(); i++)
	{
		int objIdx = FindObjectContainer(listObjects, m_objects[i]->container);

		if(m_objects[i]->container->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			 if( final && !(m_objects[i]->container->m_info.modelflags & LMODEL_FLAG_NONUNIQUE) )
				objIdx = modelList.addUnique( m_objects[i]->container->m_model );
		}

		levcellmodelobject_t object;

		memset(object.name, 0, LEV_OBJECT_NAME_LENGTH);

		object.objIndex = objIdx;

		object.tile_x = m_objects[i]->tile_dependent ? m_objects[i]->tile_x : -1;
		object.tile_y = m_objects[i]->tile_dependent ? m_objects[i]->tile_y : -1;
		object.position = m_objects[i]->position;
		object.rotation = m_objects[i]->rotation;

		// add
		cellObjectsList.append(object);
	}

	CMemoryStream regionData;
	regionData.Open(NULL, VS_OPEN_WRITE, 8192);

	// write model data
	for(int i = 0; i < modelList.numElem(); i++)
	{
		CMemoryStream modelData;
		modelData.Open(NULL, VS_OPEN_WRITE, 8192);

		modelList[i]->Save( &modelData );

		int modelSize = modelData.Tell();

		regionData.Write(&modelSize, 1, sizeof(int));
		regionData.Write(modelData.GetBasePointer(), 1, modelData.Tell());
	}

	// write cell objects list
	for(int i = 0; i < cellObjectsList.numElem(); i++)
	{
		regionData.Write(&cellObjectsList[i], 1, sizeof(levcellmodelobject_t));
	}

	levregiondatahdr_t regdatahdr;
	regdatahdr.numModelObjects = cellObjectsList.numElem();
	regdatahdr.numModels = modelList.numElem();
	regdatahdr.size = regionData.Tell();

	stream->Write(&regdatahdr, 1, sizeof(levregiondatahdr_t));
	stream->Write(regionData.GetBasePointer(), 1, regionData.Tell());
}

void CLevelRegion::ReadLoadRegion(IVirtualStream* stream, DkList<objectcont_t*>& levelmodels)
{
	if(m_isLoaded)
		return;

	for(int i = 0; i < GetNumHFields(); i++)
	{
		m_heightfield[i]->GenereateRenderData();
	}

	DkList<CLevelModel*>	modelList;

	levregiondatahdr_t		regdatahdr;

	stream->Read(&regdatahdr, 1, sizeof(levregiondatahdr_t));

	// now read models
	for(int i = 0; i < regdatahdr.numModels; i++)
	{
		int modelSize = 0;
		stream->Read(&modelSize, 1, sizeof(int));

		CLevelModel* modelRef = new CLevelModel();
		modelRef->Load( stream );
		modelRef->PreloadTextures();

		modelList.append(modelRef);
	}

	m_objects.resize( regdatahdr.numModelObjects );

	// read model cells
	for(int i = 0; i < regdatahdr.numModelObjects; i++)
	{
		levcellmodelobject_t cellObj;
		stream->Read(&cellObj, 1, sizeof(levcellmodelobject_t));

		//levcellmodelobject_old_t cellObj;
		//stream->Read(&cellObj, 1, sizeof(levcellmodelobject_old_t));

		// TODO: add models
		regobjectref_t* ref = new regobjectref_t;

		if(regdatahdr.numModels == 0)
		{
			ref->container = levelmodels[cellObj.objIndex];

			CLevelModel* model = ref->container->m_model;
			if(model)
			{
				model->Ref_Grab();
			}
		}
		else
		{
#pragma todo("model (object) containers in region data fix")
			ref->container = NULL;

			//ref->model = modelList[cellObj.objIndex];
			//if(ref->model)
			//	ref->model->Ref_Grab();
		}

		ref->tile_x = cellObj.tile_x;
		ref->tile_y = cellObj.tile_y;

		ref->tile_dependent = (ref->tile_x != -1 || ref->tile_y != -1);
		ref->position = cellObj.position;
		ref->rotation = cellObj.rotation;

		ref->transform = GetModelRefRenderMatrix( this, ref );
		
		if(ref->container->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			// create collision objects and translate them
			CLevelModel* model = ref->container->m_model;
			model->CreateCollisionObjects( this, ref );

			for(int j = 0; j < ref->collisionObjects.numElem(); j++)
			{
				CScopedMutex m(m_level->m_mutex);

				// add physics objects
				g_pPhysics->m_physics.AddStaticObject( ref->collisionObjects[j] );
			}

			BoundingBox tbbox;

			for(int i = 0; i < 8; i++)
				tbbox.AddVertex((ref->transform*Vector4D(model->m_bbox.GetVertex(i), 1.0f)).xyz());

			// set reference bbox for light testing
			ref->bbox = tbbox;
		}
		else
		{
#ifndef EDITOR

			// create object, spawn in game cycle
			CGameObject* newObj = g_pGameWorld->CreateGameObject( ref->container->m_defType.c_str(), &ref->container->m_defKeyvalues );

			if(newObj)
			{
				Vector3D eulAngles = EulerMatrixXYZ(ref->transform.getRotationComponent());

				newObj->SetOrigin( transpose(ref->transform).getTranslationComponent() );
				newObj->SetAngles( VRAD2DEG(eulAngles) );
				newObj->SetUserData( ref );

				// network name for this object
				newObj->SetName( varargs("_reg%d_ref%d", m_regionIndex, i) );

				ref->object = newObj;

				m_level->m_mutex.Lock();
				g_pGameWorld->AddObject( newObj, false );
				m_level->m_mutex.Unlock();
			}
			
#endif
		}
		
		m_level->m_mutex.Lock();
		m_objects.append(ref);
		m_level->m_mutex.Unlock();
	}

	for(int i = 0; i < GetNumHFields(); i++)
		g_pPhysics->AddHeightField( m_heightfield[i] );

	Platform_Sleep(1);

	m_isLoaded = true;
}

void CLevelRegion::RespawnObjects()
{
#ifndef EDITOR
	for(int i = 0; i < m_objects.numElem(); i++)
	{
		regobjectref_t* ref = m_objects[i];

		if(ref->container->m_info.type != LOBJ_TYPE_OBJECT_CFG)
			continue;

		// delete associated object from game world
		if(ref->object)
		{
			//g_pGameWorld->RemoveObject(ref->object);
			
			if(g_pGameWorld->m_pGameObjects.remove(ref->object))
			{
				ref->object->OnRemove();
				delete ref->object;
			}
			
		}

		ref->object = NULL;

		ref->transform = GetModelRefRenderMatrix( this, ref );

		// create object, spawn in game cycle
		CGameObject* newObj = g_pGameWorld->CreateGameObject( ref->container->m_defType.c_str(), &ref->container->m_defKeyvalues );

		if(newObj)
		{
			Vector3D eulAngles = EulerMatrixXYZ(ref->transform.getRotationComponent());

			newObj->SetOrigin( transpose(ref->transform).getTranslationComponent() );
			newObj->SetAngles( VRAD2DEG(eulAngles) );
			newObj->SetUserData( ref );

			// network name for this object
			newObj->SetName( varargs("_reg%d_ref%d", m_regionIndex, i) );

			ref->object = newObj;

			m_level->m_mutex.Lock();
			g_pGameWorld->AddObject( newObj, false );
			m_level->m_mutex.Unlock();
		}
	}
#endif // EDITOR
}

void CLevelRegion::WriteRegionRoads( IVirtualStream* stream )
{
	CMemoryStream cells;
	cells.Open(NULL, VS_OPEN_WRITE, 2048);

	int numRoadCells = 0;

	for(int x = 0; x < m_heightfield[0]->m_sizew; x++)
	{
		for(int y = 0; y < m_heightfield[0]->m_sizeh; y++)
		{
			int idx = y*m_heightfield[0]->m_sizew + x;

			if(m_roads[idx].type == ROADTYPE_NOROAD)
				continue;

			m_roads[idx].posX = x;
			m_roads[idx].posY = y;

			cells.Write(&m_roads[idx], 1, sizeof(levroadcell_t));
			numRoadCells++;
		}
	}

	stream->Write(&numRoadCells, 1, sizeof(int));
	stream->Write(cells.GetBasePointer(), 1, cells.Tell());
}

void CLevelRegion::ReadLoadRoads(IVirtualStream* stream)
{
	InitRoads();

	stream->Read(&m_numRoadCells, 1, sizeof(int));

	if(m_numRoadCells)
	{
		//m_flatroads = new levroadcell_t*[m_numRoadCells];

		for(int i = 0; i < m_numRoadCells; i++)
		{
			levroadcell_t tmpCell;
			stream->Read(&tmpCell, 1, sizeof(levroadcell_t));

			int idx = tmpCell.posY*m_heightfield[0]->m_sizew + tmpCell.posX;
			m_roads[idx] = tmpCell;

			//m_flatroads[i] = &m_roads[idx];

#define NAVGRIDSCALE_HALF	int(AI_NAVIGATION_GRID_SCALE/2)

			// higher the priority of road nodes
			for(int j = 0; j < AI_NAVIGATION_GRID_SCALE*AI_NAVIGATION_GRID_SCALE; j++)
			{
				int ofsX = tmpCell.posX*AI_NAVIGATION_GRID_SCALE + (j % AI_NAVIGATION_GRID_SCALE);
				int ofsY = tmpCell.posY*AI_NAVIGATION_GRID_SCALE + (j % NAVGRIDSCALE_HALF);

				int navCellIdx = ofsY*m_heightfield[0]->m_sizew + ofsX;
				m_navGrid[navCellIdx] = 4 - AI_NAVIGATION_ROAD_PRIORITY;
			}
		}

		for (int i = 0; i < m_objects.numElem(); i++)
		{
			m_level->Nav_AddObstacle(this, m_objects[i]);
		}
	}
}

void CLevelRegion::WriteRegionOccluders(IVirtualStream* stream)
{
	int numOccluders = m_occluders.numElem();

	stream->Write(&numOccluders, 1, sizeof(int));
	stream->Write(m_occluders.ptr(), 1, sizeof(levOccluderLine_t)*numOccluders);
}

void CLevelRegion::ReadLoadOccluders(IVirtualStream* stream)
{
	int numOccluders = 0;
	stream->Read(&numOccluders, 1, sizeof(int));

	m_occluders.setNum(numOccluders);
	stream->Read(m_occluders.ptr(), 1, sizeof(levOccluderLine_t)*numOccluders);
}

//-----------------------------------------------------------------------------------------------------------

CLevelRegion* CGameLevel::GetRegionAtPosition(const Vector3D& pos) const
{
	IVector2D regXY;

	if( GetPointAt(pos, regXY) )
		return &m_regions[regXY.y*m_wide + regXY.x];
	else
		return NULL;
}

CHeightTileFieldRenderable* CGameLevel::GetHeightFieldAt(const IVector2D& XYPos, int idx) const
{
	if(XYPos.x < 0 || XYPos.x >= m_wide)
		return NULL;

	if(XYPos.y < 0 || XYPos.y >= m_tall)
		return NULL;

	return m_regions[XYPos.y*m_wide + XYPos.x].GetHField(idx);
}

CLevelRegion* CGameLevel::GetRegionAt(const IVector2D& XYPos) const
{
	if(XYPos.x < 0 || XYPos.x >= m_wide)
		return NULL;

	if(XYPos.y < 0 || XYPos.y >= m_tall)
		return NULL;

	return &m_regions[XYPos.y*m_wide + XYPos.x];
}


bool CGameLevel::GetPointAt(const Vector3D& pos, IVector2D& outXYPos) const
{
	int cellSize = HFIELD_POINT_SIZE*m_cellsSize;

	Vector3D center(m_wide*cellSize*-0.5f, 0, m_tall*cellSize*-0.5f);

	Vector3D zeroed_pos = pos - center;

	float p_size = (1.0f / (float)cellSize);

	Vector2D xz_pos = (zeroed_pos.xz()) * p_size;

	outXYPos.x = xz_pos.x;
	outXYPos.y = xz_pos.y;

	if(outXYPos.x < 0 || outXYPos.x >= m_wide)
		return false;

	if(outXYPos.y < 0 || outXYPos.y >= m_tall)
		return false;

	return true;
}

IVector2D CGameLevel::PositionToGlobalTilePoint( const Vector3D& pos ) const
{
	IVector2D outXYPos;
	CLevelRegion* pRegion;

	if(GetRegionAndTile(pos, &pRegion, outXYPos))
	{
		IVector2D globalPoint;

		LocalToGlobalPoint(outXYPos, pRegion, globalPoint);

		return globalPoint;
	}

	return IVector2D(0,0);
}

Vector3D CGameLevel::GlobalTilePointToPosition( const IVector2D& point ) const
{
	IVector2D outXYPos;
	CLevelRegion* pRegion;

	GlobalToLocalPoint(point, outXYPos, &pRegion);

	if(pRegion)
	{
		return pRegion->CellToPosition(outXYPos.x,outXYPos.y);
	}

	return Vector3D(0,0,0);
}

bool CGameLevel::GetRegionAndTile(const Vector3D& pos, CLevelRegion** pReg, IVector2D& outXYPos) const
{
	CLevelRegion* pRegion = GetRegionAtPosition(pos);

	if(pRegion && pRegion->m_heightfield[0]->PointAtPos(pos, outXYPos.x, outXYPos.y))
	{
		(*pReg) = pRegion;
		return true;
	}
	else
		(*pReg) = NULL;

	return false;
}

bool CGameLevel::GetRegionAndTileAt(const IVector2D& point, CLevelRegion** pReg, IVector2D& outLocalXYPos) const
{
	CLevelRegion* pRegion = NULL;

	GlobalToLocalPoint(point, outLocalXYPos, &pRegion);

	if(pRegion && pRegion->m_heightfield[0]->GetTile(outLocalXYPos.x, outLocalXYPos.y))
	{
		if(pReg)
			(*pReg) = pRegion;

		return true;
	}
	else
	{
		if(pReg)
			(*pReg) = pRegion;
	}

	return false;
}

bool CGameLevel::GetTileGlobal(const Vector3D& pos, IVector2D& outGlobalXYPos) const
{
	CLevelRegion* pRegion = NULL;

	IVector2D localPos;

	if(!GetRegionAndTile(pos, &pRegion, localPos))
		return false;

	LocalToGlobalPoint(localPos, pRegion, outGlobalXYPos);

	return true;
}

void CGameLevel::GlobalToLocalPoint( const IVector2D& point, IVector2D& outLocalPoint, CLevelRegion** pRegion ) const
{
	IVector2D regPos;
	regPos.x = floor((float)point.x / (float)(m_cellsSize));
	regPos.y = floor((float)point.y / (float)(m_cellsSize));

	IVector2D globalStart;
	globalStart.x = regPos.x * m_cellsSize;
	globalStart.y = regPos.y * m_cellsSize;

	outLocalPoint = point-globalStart;

	(*pRegion) = GetRegionAt(regPos);
}

void CGameLevel::LocalToGlobalPoint( const IVector2D& point, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const
{
	outGlobalPoint.x = pRegion->m_heightfield[0]->m_posidx_x * m_cellsSize + point.x;
	outGlobalPoint.y = pRegion->m_heightfield[0]->m_posidx_y * m_cellsSize + point.y;
}

//------------------------------------------------------------------------------------------------------------------------------------

bool IsOppositeDirectionTo(int dirA, int dirB)
{
	int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
	int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

	if( (dx[dirA] != dx[dirB] && (dx[dirA] + dx[dirB] == 0)) ||
		(dy[dirA] != dy[dirB] && (dy[dirA] + dy[dirB] == 0)))
		return true;

	return false;
}

straight_t CGameLevel::GetStraightAtPoint( const IVector2D& point, int numIterations ) const
{
	CLevelRegion* pRegion = NULL;

	straight_t straight;
	straight.direction = -1;

	IVector2D localPos;

	if( GetRegionAndTileAt(point, &pRegion, localPos ) && pRegion->m_roads )
	{
		int tileIdx = localPos.y * m_cellsSize + localPos.x;

		if(	pRegion->m_roads[tileIdx].type != ROADTYPE_STRAIGHT )
			return straight;

		int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
		int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

		straight.start = localPos;
		straight.dirChangeIter = INT_MAX;
		straight.breakIter = 0;

		LocalToGlobalPoint(localPos, pRegion, straight.start);

		int roadDir = pRegion->m_roads[tileIdx].direction;
		
		int checkRoadDir = roadDir;

		IVector2D lastPos = localPos;

		for(int i = 0; i < numIterations; i++,straight.breakIter++)
		{
			// go that direction
			IVector2D xyPos = lastPos + IVector2D(dx[checkRoadDir],dy[checkRoadDir]);//localPos + IVector2D(dx[roadDir],dy[roadDir])*i;

			lastPos = xyPos;

			CLevelRegion* pNextRegion;

			xyPos = pRegion->GetTileAndNeighbourRegion(xyPos.x, xyPos.y, &pNextRegion);
			int nextTileIdx = xyPos.y * m_cellsSize + xyPos.x;

			if(!pNextRegion->m_roads)
				continue;

			if(	pNextRegion->m_roads[nextTileIdx].type != ROADTYPE_STRAIGHT )
				break;

			if( IsOppositeDirectionTo(pNextRegion->m_roads[nextTileIdx].direction, checkRoadDir) )
				continue;

			if(	pNextRegion->m_roads[nextTileIdx].direction != checkRoadDir &&
				i < straight.dirChangeIter)
			{
				straight.dirChangeIter = i;
			}

			checkRoadDir = pNextRegion->m_roads[nextTileIdx].direction;

			// set the original direction
			straight.direction = roadDir;
			straight.end = xyPos;
			LocalToGlobalPoint(xyPos, pNextRegion, straight.end);
		}

		// don't let it be equal
		if(	straight.direction != -1 &&
			straight.start == straight.end)
		{
			// step back
			straight.start -= IVector2D(dx[straight.direction], dy[straight.direction]);
		}
	}

	return straight;
}

straight_t CGameLevel::GetStraightAtPos( const Vector3D& pos, int numIterations ) const
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return GetStraightAtPoint(globPos, numIterations);
}

roadJunction_t CGameLevel::GetJunctionAtPoint( const IVector2D& point, int numIterations ) const
{
	CLevelRegion* pRegion = NULL;

	roadJunction_t junction;
	junction.breakIter = 0;
	junction.startIter = -1;

	IVector2D localPos;

	if( GetRegionAndTileAt(point, &pRegion, localPos ) && pRegion->m_roads )
	{
		int tileIdx = localPos.y * m_cellsSize + localPos.x;

		if(	pRegion->m_roads[tileIdx].type == ROADTYPE_NOROAD )
		{
			return junction;
		}

		int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
		int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

		junction.start = localPos;
		junction.end = localPos;

		// if we already on junction
		if(pRegion->m_roads[tileIdx].type == ROADTYPE_JUNCTION)
		{
			return junction;
		}

		LocalToGlobalPoint(localPos, pRegion, junction.start);

		int roadDir = pRegion->m_roads[tileIdx].direction;
		
		int checkRoadDir = roadDir;

		IVector2D lastPos = localPos;

		for(int i = 0; i < numIterations; i++, junction.breakIter++)
		{
			// go that direction
			IVector2D xyPos = lastPos + IVector2D(dx[checkRoadDir],dy[checkRoadDir]);//localPos + IVector2D(dx[roadDir],dy[roadDir])*i;

			lastPos = xyPos;

			CLevelRegion* pNextRegion;

			xyPos = pRegion->GetTileAndNeighbourRegion(xyPos.x, xyPos.y, &pNextRegion);
			int nextTileIdx = xyPos.y * m_cellsSize + xyPos.x;

			if(!pNextRegion->m_roads)
				continue;

			if( pNextRegion->m_roads[nextTileIdx].type != ROADTYPE_JUNCTION && junction.startIter != -1 )
				break;

			if(	pNextRegion->m_roads[nextTileIdx].type == ROADTYPE_NOROAD )
				break;

			if(pNextRegion->m_roads[nextTileIdx].type == ROADTYPE_JUNCTION && junction.startIter == -1)
			{
				junction.startIter = i;
			}

			// set the original direction
			junction.end = xyPos;
			LocalToGlobalPoint(xyPos, pNextRegion, junction.end);
		}
	}

	return junction;
}

roadJunction_t CGameLevel::GetJunctionAtPos( const Vector3D& pos, int numIterations ) const
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return GetJunctionAtPoint(globPos, numIterations);
}

int GetPerpendicularDir(int dir)
{
	int nDir = dir+1;

	if(nDir > 3)
		nDir -= 4;

	return nDir;
}

int	CGameLevel::GetLaneIndexAtPoint( const IVector2D& point, int numIterations)
{
	CLevelRegion* pRegion = NULL;

	IVector2D localPos;

	int nLane = 0;

	if( GetRegionAndTileAt(point, &pRegion, localPos ) && pRegion->m_roads )
	{
		int tileIdx = localPos.y * m_cellsSize + localPos.x;

		if(	pRegion->m_roads[tileIdx].type != ROADTYPE_STRAIGHT )
			return -1;

		int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
		int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

		int laneDir = pRegion->m_roads[tileIdx].direction;
		int laneRowDir = GetPerpendicularDir(pRegion->m_roads[tileIdx].direction);

		for(int i = 0; i < numIterations; i++)
		{
			IVector2D xyPos = localPos + IVector2D(dx[laneRowDir],dy[laneRowDir])*i;//localPos + IVector2D(dx[roadDir],dy[roadDir])*i;

			CLevelRegion* pNextRegion;

			xyPos = pRegion->GetTileAndNeighbourRegion(xyPos.x, xyPos.y, &pNextRegion);
			int nextTileIdx = xyPos.y * m_cellsSize + xyPos.x;

			if(!pNextRegion->m_roads)
				continue;

			if(	pNextRegion->m_roads[nextTileIdx].type != ROADTYPE_STRAIGHT )
				break;

			if( pNextRegion->m_roads[nextTileIdx].direction != laneDir)
				break;

			nLane++;
		}
	}

	return nLane;
}

int	CGameLevel::GetLaneIndexAtPos( const Vector3D& pos, int numIterations)
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return GetLaneIndexAtPoint(globPos, numIterations);
}

int	CGameLevel::GetRoadWidthInLanesAtPoint( const IVector2D& point, int numIterations )
{
	CLevelRegion* pRegion = NULL;

	IVector2D localPos;

	// trace to the right
	int nLanes = GetLaneIndexAtPoint(point, numIterations);

	if(nLanes == -1)
		return 0;

	// and left
	if( GetRegionAndTileAt(point, &pRegion, localPos ) && pRegion->m_roads )
	{
		int tileIdx = localPos.y * m_cellsSize + localPos.x;

		if(	pRegion->m_roads[tileIdx].type != ROADTYPE_STRAIGHT )
			return -1;

		int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
		int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

		int laneDir = pRegion->m_roads[tileIdx].direction;
		int laneRowDir = GetPerpendicularDir(pRegion->m_roads[tileIdx].direction);

		for(int i = 1; i < numIterations; i++)
		{
			IVector2D nextPos = localPos - IVector2D(dx[laneRowDir],dy[laneRowDir])*i;//localPos + IVector2D(dx[roadDir],dy[roadDir])*i;

			CLevelRegion* pNextRegion;

			nextPos = pRegion->GetTileAndNeighbourRegion(nextPos.x, nextPos.y, &pNextRegion);
			int nextTileIdx = nextPos.y * m_cellsSize + nextPos.x;

			if(!pNextRegion->m_roads)
				continue;

			if(	pNextRegion->m_roads[nextTileIdx].type != ROADTYPE_STRAIGHT )
				break;

			// only parallels
			if( (pNextRegion->m_roads[nextTileIdx].direction % 2) != (laneDir % 2))
				break;

			nLanes++;
		}
	}

	return nLanes;
}

int	CGameLevel::GetRoadWidthInLanesAtPos( const Vector3D& pos, int numIterations )
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return GetRoadWidthInLanesAtPoint(globPos, numIterations);
}

int	CGameLevel::GetNumLanesAtPoint( const IVector2D& point, int numIterations )
{
	CLevelRegion* pRegion = NULL;

	IVector2D localPos;

	int nLanes = GetLaneIndexAtPoint(point, numIterations);

	if(nLanes <= 0)
		return 0;

	if( GetRegionAndTileAt(point, &pRegion, localPos ) && pRegion->m_roads )
	{
		int tileIdx = localPos.y * m_cellsSize + localPos.x;

		if(	pRegion->m_roads[tileIdx].type != ROADTYPE_STRAIGHT )
			return 0;

		IVector2D lastPos = localPos;

		int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
		int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

		int laneDir = pRegion->m_roads[tileIdx].direction;
		int laneRowDir = GetPerpendicularDir(pRegion->m_roads[tileIdx].direction);

		for(int i = 1; i < numIterations; i++)
		{
			IVector2D xyPos = lastPos - IVector2D(dx[laneRowDir],dy[laneRowDir]);//localPos + IVector2D(dx[roadDir],dy[roadDir])*i;

			lastPos = xyPos;

			CLevelRegion* pNextRegion;

			xyPos = pRegion->GetTileAndNeighbourRegion(xyPos.x, xyPos.y, &pNextRegion);
			int nextTileIdx = xyPos.y * m_cellsSize + xyPos.x;

			if(!pNextRegion->m_roads)
				continue;

			if(	pNextRegion->m_roads[nextTileIdx].type != ROADTYPE_STRAIGHT )
				break;

			if( pNextRegion->m_roads[nextTileIdx].direction != laneDir)
				break;

			nLanes++;
		}
	}

	return nLanes;
}

int	CGameLevel::GetNumLanesAtPos( const Vector3D& pos, int numIterations )
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return GetNumLanesAtPoint(globPos, numIterations);
}

levroadcell_t* CGameLevel::GetGlobalRoadTile(const Vector3D& pos, CLevelRegion** pRegion) const
{
	IVector2D globPos = PositionToGlobalTilePoint(pos);

	return GetGlobalRoadTileAt(globPos, pRegion);
}

levroadcell_t* CGameLevel::GetGlobalRoadTileAt(const IVector2D& point, CLevelRegion** pRegion) const
{
	IVector2D outXYPos;

	CLevelRegion* reg = NULL;

	if(GetRegionAndTileAt(point, &reg, outXYPos))
	{
		if(!reg->GetHField())
			return NULL;

		if(!reg->m_roads)
			return NULL;

		int ridx = reg->GetHField()->m_sizew*outXYPos.y + outXYPos.x;
		
		if(pRegion)
			*pRegion = reg;

		return &reg->m_roads[ridx];
	}

	return NULL;
}

//------------------------------------------------------------------------------------------------------------------------------------

void CGameLevel::QueryNearestRegions( const Vector3D& pos, bool waitLoad )
{
	IVector2D posXY;
	if(GetPointAt(pos,posXY))
	{
		QueryNearestRegions(posXY, waitLoad);
	}
}

void CGameLevel::QueryNearestRegions( const IVector2D& point, bool waitLoad )
{
	CLevelRegion* region = GetRegionAt(point);

	if(!region)
		return;

	int numNeedToLoad = !region->m_isLoaded && (m_regionOffsets[point.y*m_wide+point.x] != -1);

	// query this region
	region->m_queryTimes.Increment();

	int dx[8] = NEIGHBOR_OFFS_XDX(point.x, 1);
	int dy[8] = NEIGHBOR_OFFS_YDY(point.y, 1);

	// query surrounding regions
	for(int i = 0; i < 8; i++)
	{
		CLevelRegion* nregion = GetRegionAt(IVector2D(dx[i], dy[i]));

		if(nregion)
		{
			if(!nregion->m_isLoaded)
				numNeedToLoad += (m_regionOffsets[dy[i]*m_wide+dx[i]] != -1);

			nregion->m_queryTimes.Increment();
		}
	}

	if( numNeedToLoad > 0 )
	{
		// signal loader
		SignalWork();

		if( waitLoad )
			WaitForThread();
	}
}

ConVar w_freeze("w_freeze", "0", "Freeze world loading/unloading", CV_CHEAT);

#ifdef EDITOR
void CGameLevel::Ed_Prerender(const Vector3D& cameraPosition)
{
	Volume& frustum = g_pGameWorld->m_frustum;
	IVector2D camPosReg;

	// mark renderable regions
	if( GetPointAt(cameraPosition, camPosReg) )
	{
		CLevelRegion* region = GetRegionAt(camPosReg);

		// query this region
		if(region)
		{
			//region->m_queryTimes.Increment();
			region->m_render = frustum.IsBoxInside(region->m_bbox.minPoint, region->m_bbox.maxPoint);

			if(region->m_render)
			{
				region->Ed_Prerender();
				region->m_render = false;
			}
		}

		int dx[8] = NEIGHBOR_OFFS_XDX(camPosReg.x, 1);
		int dy[8] = NEIGHBOR_OFFS_YDY(camPosReg.y, 1);

		// surrounding regions frustum
		for(int i = 0; i < 8; i++)
		{
			CLevelRegion* nregion = GetRegionAt(IVector2D(dx[i], dy[i]));

			if(nregion)
			{
				//nregion->m_queryTimes.Increment();

				if(frustum.IsBoxInside(nregion->m_bbox.minPoint, nregion->m_bbox.maxPoint))
				{
					nregion->Ed_Prerender();
					nregion->m_render = false;
				}
			}
		}

		//SignalWork();
	}
}
#endif // EDITOR

void CGameLevel::CollectVisibleOccluders(occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition)
{
	frustumOccluders.frustum = g_pGameWorld->m_frustum;

	// don't render too far?
	IVector2D camPosReg;

	// mark renderable regions
	if( GetPointAt(cameraPosition, camPosReg) )
	{
		CLevelRegion* region = GetRegionAt(camPosReg);

		// query this region
		if(region)
		{
			//region->m_queryTimes.Increment();
			bool visible = frustumOccluders.frustum.IsBoxInside(region->m_bbox.minPoint, region->m_bbox.maxPoint);

			if(visible)
				region->CollectVisibleOccluders(frustumOccluders, cameraPosition);
		}

		int dx[8] = NEIGHBOR_OFFS_XDX(camPosReg.x, 1);
		int dy[8] = NEIGHBOR_OFFS_YDY(camPosReg.y, 1);

		// surrounding regions frustum
		for(int i = 0; i < 8; i++)
		{
			CLevelRegion* nregion = GetRegionAt(IVector2D(dx[i], dy[i]));

			if(nregion)
			{
				bool visible = frustumOccluders.frustum.IsBoxInside(nregion->m_bbox.minPoint, nregion->m_bbox.maxPoint);

				if(visible)
					nregion->CollectVisibleOccluders(frustumOccluders, cameraPosition);
			}
		}
	}
}

extern ConVar r_regularLightTextureUpdates;

void CGameLevel::Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& occlFrustum, int nRenderFlags)
{
	// don't render too far?
	IVector2D camPosReg;

//#ifndef EDITOR
	// mark renderable regions
	if( GetPointAt(cameraPosition, camPosReg) )
	{
		CLevelRegion* region = GetRegionAt(camPosReg);

		// query this region
		if(region)
		{
			//region->m_queryTimes.Increment();
			region->m_render = occlFrustum.IsBoxVisible( region->m_bbox );

			if(region->m_render)
			{
				region->Render( cameraPosition, viewProj, occlFrustum, nRenderFlags );
				region->m_render = false;
			}
		}

		int dx[8] = NEIGHBOR_OFFS_XDX(camPosReg.x, 1);
		int dy[8] = NEIGHBOR_OFFS_YDY(camPosReg.y, 1);

		// surrounding regions frustum
		for(int i = 0; i < 8; i++)
		{
			CLevelRegion* nregion = GetRegionAt(IVector2D(dx[i], dy[i]));

			if(nregion)
			{
				nregion->m_render = occlFrustum.IsBoxVisible( nregion->m_bbox );

				if(nregion->m_render)
				{
					nregion->Render( cameraPosition, viewProj, occlFrustum, nRenderFlags );
					nregion->m_render = false;
				}
			}
		}
	}/*
#else

	// render all in frustum
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			CLevelRegion* region = &m_regions[idx];

			// query this region
			region->m_render = frustum.IsBoxInside(region->m_bbox.minPoint, region->m_bbox.maxPoint);

			if(region->m_render)
			{
				region->Render( cameraPosition, viewProj, nRenderFlags );
				region->m_render = false;
			}
		}
	}
#endif // EDITOR*/

#ifdef USE_INSTANCING

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	if(caps.isInstancingSupported && r_enableLevelInstancing.GetBool())
	{
		regObjectInstance_t* instData;

		materials->SetInstancingEnabled( true ); // matsystem switch to use instancing in shaders
		materials->SetMatrix(MATRIXMODE_WORLD, identity4());

		// force disable vertex buffer
		g_pShaderAPI->SetVertexBuffer( NULL, 1 );

		for(int i = 0; i < m_objectDefs.numElem(); i++)
		{
			if(m_objectDefs[i]->m_info.type != LOBJ_TYPE_INTERNAL_STATIC)
				continue;

			if(!m_objectDefs[i]->m_model || !m_objectDefs[i]->m_instData)
				continue;

			if(m_objectDefs[i]->m_instData->numInstances == 0)
				continue;

			// set vertex buffer
			g_pShaderAPI->SetVertexBuffer( m_instanceBuffer, 2 );

			// before lock we have to unbind our buffer
			g_pShaderAPI->ChangeVertexBuffer( NULL, 2 );

			int numInstances = m_objectDefs[i]->m_instData->numInstances;

			// upload
			if( m_instanceBuffer->Lock(0, numInstances, (void**)&instData, false))
			{
				memcpy(instData, m_objectDefs[i]->m_instData->instances, sizeof(regObjectInstance_t)*numInstances);
			
				m_instanceBuffer->Unlock();
			}

			m_objectDefs[i]->m_model->Render(nRenderFlags, m_objectDefs[i]->m_model->m_bbox);

			// reset instance count
			m_objectDefs[i]->m_instData->numInstances = 0;

			// disable this vertex buffer or our cars will be drawn many times
			g_pShaderAPI->SetVertexBuffer( NULL, 2 );
		}

		materials->SetInstancingEnabled( false );

		// force disable vertex buffer
		g_pShaderAPI->SetVertexBuffer( NULL, 2 );
		g_pShaderAPI->ChangeVertexBuffer(NULL, 2);
	}
#endif // USE_INSTANCING
}

int	CGameLevel::Run()
{
	UpdateRegionLoading();

	return 0;
}

int CGameLevel::UpdateRegionLoading()
{
#ifndef EDITOR

	int numLoadedRegions = 0;

	float startLoadTime = Platform_GetCurrentTime();

	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

			// try preloading region
			if( !w_freeze.GetBool() && 
				!m_regions[idx].m_isLoaded && 
				(m_regions[idx].m_queryTimes.GetValue() > 0) &&
				m_regionOffsets[idx] != -1 )
			{
				PreloadRegion(x,y);

				numLoadedRegions++;
				DevMsg(2, "Region %d loaded\n", idx);
			}
		}
	}

	float loadTime = Platform_GetCurrentTime()-startLoadTime;
	if (numLoadedRegions)
		DevMsg(2, "*** %d regions loaded for %g seconds\n", numLoadedRegions, loadTime);

	return numLoadedRegions;
#else
	return 0;
#endif // EDITOR
}

int CGameLevel::UpdateRegions( RegionLoadUnloadCallbackFunc func )
{
	int numFreedRegions = 0;

	// unloading only available after loading
	if(!IsWorkDone())
		return 0;

	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;

#ifndef EDITOR
			if(!w_freeze.GetBool() &&
				m_regions[idx].m_isLoaded &&
				(m_regions[idx].m_queryTimes.GetValue() <= 0) &&
				m_regionOffsets[idx] != -1 )
			{
				// unload region
				m_regions[idx].Cleanup();
				m_regions[idx].m_scriptEventCallbackCalled = false;

				numFreedRegions++;
				DevMsg(2, "Region %d freed\n", idx);
			}
			else
			{

#pragma todo("SPAWN objects from UpdateRegions, not from loader thread")
			}
#endif // EDITOR

			//m_regions[idx].m_queryTimes.Decrement();

			//if(m_regions[idx].m_queryTimes.GetValue() < 0)
			m_regions[idx].m_queryTimes.SetValue(0);

			if( !m_regions[idx].m_scriptEventCallbackCalled )
			{
				if(func)
					(func)(&m_regions[idx], idx);

				m_regions[idx].m_scriptEventCallbackCalled = true;
			}
		}
	}

	return numFreedRegions;
}

void CGameLevel::RespawnAllObjects()
{
	for(int x = 0; x < m_wide; x++)
	{
		for(int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide+x;
			m_regions[idx].RespawnObjects();
		}
	}
}

//#ifdef EDITOR

#define OBSTACLE_STATIC_MAX_HEIGHT	(6.0f)
#define OBSTACLE_PROP_MAX_HEIGHT	(4.0f)

void CGameLevel::Nav_AddObstacle(CLevelRegion* reg, regobjectref_t* ref)
{
	/*
		transformedModel = transformModelRef(ref)
		foreach triangle in transformedModel
			tileRange = getTileRange(triangle)
			foreach tile in tileRange
				volume = getTileVolume
				if( volume.checkIntersectionWithTriangle(triangle) )
					tile.bit = 1
				
	*/

	int navCellGridSize = m_cellsSize*AI_NAVIGATION_GRID_SCALE;

	//Matrix4x4 transform = GetModelRefRenderMatrix(reg, ref);

	if(ref->container->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
		// static model processing

		if((ref->container->m_info.modelflags & LMODEL_FLAG_ISGROUND) ||
			(ref->container->m_info.modelflags & LMODEL_FLAG_NOCOLLIDE))
			return;

		CLevelModel* model = ref->container->m_model;

		for (int i = 0; i < model->m_numIndices; i += 3)
		{
			if (i + 1 > model->m_numIndices || i + 2 > model->m_numIndices)
				break;

			Vector3D p0, p1, p2;
			Vector3D v0, v1, v2;

			if (model->m_indices[i] >= model->m_numVerts ||
				model->m_indices[i + 1] >= model->m_numVerts ||
				model->m_indices[i + 2] >= model->m_numVerts)
				continue;

			p0 = model->m_verts[model->m_indices[i]].position;
			p1 = model->m_verts[model->m_indices[i + 1]].position;
			p2 = model->m_verts[model->m_indices[i + 2]].position;

			v0 = (ref->transform*Vector4D(p0, 1.0f)).xyz();
			v1 = (ref->transform*Vector4D(p1, 1.0f)).xyz();
			v2 = (ref->transform*Vector4D(p2, 1.0f)).xyz();

			Vector3D normal;
			ComputeTriangleNormal(v0, v1, v2, normal);

			//if(fabs(dot(normal,vec3_up)) > 0.8f)
			//	continue;

			if(v0.y > OBSTACLE_STATIC_MAX_HEIGHT || v1.y > OBSTACLE_STATIC_MAX_HEIGHT || v2.y > OBSTACLE_STATIC_MAX_HEIGHT)
				continue;

			//debugoverlay->Polygon3D(v0, v1, v2, ColorRGBA(1, 0, 1, 0.25f), 100.0f);

			BoundingBox vertbox;
			vertbox.AddVertex(v0);
			vertbox.AddVertex(v1);
			vertbox.AddVertex(v2);

			// get a cell range
			IVector2D min, max;
			Nav_GetCellRangeFromAABB(vertbox.minPoint, vertbox.maxPoint, min, max);

			// extend
			min.x = clamp(min.x-1, 0, navCellGridSize*m_wide);
			min.y = clamp(min.y-1, 0, navCellGridSize*m_tall);
			max.x = clamp(max.x+1, 0, navCellGridSize*m_wide);
			max.y = clamp(max.y+1, 0, navCellGridSize*m_tall);

			// in this range do...
			for (int y = min.y; y < max.y + 1; y++)
			{
				for (int x = min.x; x < max.x + 1; x++)
				{
					//Vector3D pointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(IVector2D(x, y));
					//debugoverlay->Box3D(pointPos - 0.5f, pointPos + 0.5f, ColorRGBA(1, 0, 1, 0.1f));

					ubyte& tile = Nav_GetTileAtGlobalPoint(IVector2D(x,y));
					tile = 0;
				}
			}
		}

	}
	else if(ref->container->m_defModel != NULL)
	{
		if(	ref->container->m_defType == "debris" ||
			ref->container->m_defType == "sheets" ||
			ref->container->m_defType == "misc")
			return;

		// studio model
		physmodeldata_t* physData = &ref->container->m_defModel->GetHWData()->m_physmodel;

		for(int i = 0; i < physData->numIndices; i+=3)
		{
			Vector3D v0, v1, v2;
			Vector3D p0,p1,p2;

			if (physData->indices[i] >= physData->numVertices ||
				physData->indices[i + 1] >= physData->numVertices ||
				physData->indices[i + 2] >= physData->numVertices)
				continue;

			p0 = physData->vertices[physData->indices[i]];
			p1 = physData->vertices[physData->indices[i + 1]];
			p2 = physData->vertices[physData->indices[i + 2]];

			v0 = (ref->transform*Vector4D(p0, 1.0f)).xyz();
			v1 = (ref->transform*Vector4D(p1, 1.0f)).xyz();
			v2 = (ref->transform*Vector4D(p2, 1.0f)).xyz();

			Vector3D normal;
			ComputeTriangleNormal(v0, v1, v2, normal);

			//if (fabs(dot(normal, vec3_up)) > 0.8f)
			//	continue;

			if (p0.y > OBSTACLE_PROP_MAX_HEIGHT || p1.y > OBSTACLE_PROP_MAX_HEIGHT || p2.y > OBSTACLE_PROP_MAX_HEIGHT)
				continue;

			//debugoverlay->Polygon3D(v0,v1,v2, ColorRGBA(1,1,0,0.25f), 100.0f);

			BoundingBox vertbox;
			vertbox.AddVertex(v0);
			vertbox.AddVertex(v1);
			vertbox.AddVertex(v2);

			// get a cell range
			IVector2D min, max;
			Nav_GetCellRangeFromAABB(vertbox.minPoint, vertbox.maxPoint, min, max);

			// extend
			min.x = clamp(min.x - 1, 0, navCellGridSize*m_wide);
			min.y = clamp(min.y - 1, 0, navCellGridSize*m_tall);

			max.x = clamp(max.x + 1, 0, navCellGridSize*m_wide);
			max.y = clamp(max.y + 1, 0, navCellGridSize*m_tall);


			// in this range do...
			for (int y = min.y; y < max.y + 1; y++)
			{
				for (int x = min.x; x < max.x + 1; x++)
				{
					//Vector3D pointPos = g_pGameWorld->m_level.Nav_GlobalPointToPosition(IVector2D(x, y));
					//debugoverlay->Box3D(pointPos - 0.5f, pointPos + 0.5f, ColorRGBA(1, 0, 1, 0.1f));

					ubyte& tile = Nav_GetTileAtGlobalPoint(IVector2D(x, y));
					tile = 0;
				}
			}
		}
	}


	/*
		//-----------------------------------------------------------------------------
		// Studio triangles
		//-----------------------------------------------------------------------------

		studiohdr_t* pHdr = m_pModel->GetHWData()->pStudioHdr;
		int nLod = 0;

		Matrix4x4 transform = GetRenderWorldTransform();

		int i = 0;

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

			for(int k = 0; k < pGroup->numindices; k+=3)
			{
				Vector3D v0,v1,v2;

				v0 = pGroup->pVertex(pIndices[k])->point;
				v1 = pGroup->pVertex(pIndices[k+1])->point;
				v2 = pGroup->pVertex(pIndices[k+2])->point;

				v0 = (transform*Vector4D(v0,1)).xyz();
				v1 = (transform*Vector4D(v1,1)).xyz();
				v2 = (transform*Vector4D(v2,1)).xyz();

				float dist = MAX_COORD_UNITS+1;

				if(IsRayIntersectsTriangle(v0,v1,v2, start, ray_dir, dist))
				{
					if(dist < best_dist && dist > 0)
					{
						best_dist = dist;
						fraction = dist;

						outPos = lerp(start, end, dist);
					}
				}
			}
		}
	*/

}
//#endif // EDITOR

void CGameLevel::Nav_GetCellRangeFromAABB(const Vector3D& mins, const Vector3D& maxs, IVector2D& xy1, IVector2D& xy2) const
{
	xy1 = Nav_PositionToGlobalNavPoint(mins);
	xy2 = Nav_PositionToGlobalNavPoint(maxs);
}

void CGameLevel::Nav_GlobalToLocalPoint(const IVector2D& point, IVector2D& outLocalPoint, CLevelRegion** pRegion) const
{
	int navGridSize = m_cellsSize*AI_NAVIGATION_GRID_SCALE;

	IVector2D regPos;
	regPos.x = floor((float)point.x / (float)(navGridSize));
	regPos.y = floor((float)point.y / (float)(navGridSize));

	IVector2D globalStart;
	globalStart.x = regPos.x * navGridSize;
	globalStart.y = regPos.y * navGridSize;

	outLocalPoint = point - globalStart;

	(*pRegion) = GetRegionAt(regPos);
}

void CGameLevel::Nav_LocalToGlobalPoint(const IVector2D& point, const CLevelRegion* pRegion, IVector2D& outGlobalPoint) const
{
	int navGridSize = m_cellsSize*AI_NAVIGATION_GRID_SCALE;

	outGlobalPoint.x = pRegion->m_heightfield[0]->m_posidx_x * navGridSize + point.x;
	outGlobalPoint.y = pRegion->m_heightfield[0]->m_posidx_y * navGridSize + point.y;
}

#define NAV_POINT_SIZE (HFIELD_POINT_SIZE/AI_NAVIGATION_GRID_SCALE)

//
// conversions
//
IVector2D CGameLevel::Nav_PositionToGlobalNavPoint(const Vector3D& pos) const
{
	IVector2D gridWorldSize(m_wide, m_tall);
	gridWorldSize *= m_cellsSize*AI_NAVIGATION_GRID_SCALE;

	float p_size = (1.0f / NAV_POINT_SIZE);

	Vector2D xz_pos = pos.xz() * p_size + gridWorldSize/2;

	IVector2D out;

	out.x = xz_pos.x;// + 0.5f;
	out.y = xz_pos.y;// + 0.5f;

	return out;
}

Vector3D CGameLevel::Nav_GlobalPointToPosition(const IVector2D& point) const
{
	IVector2D outXYPos;
	CLevelRegion* pRegion;

	Nav_GlobalToLocalPoint(point, outXYPos, &pRegion);

	if (pRegion)
	{
		CHeightTileField& defField = *pRegion->m_heightfield[0];

		hfieldtile_t* tile = defField.GetTile(outXYPos.x / AI_NAVIGATION_GRID_SCALE, outXYPos.y / AI_NAVIGATION_GRID_SCALE);

		Vector3D tile_position;

		if (tile)
			tile_position = defField.m_position + Vector3D(outXYPos.x*NAV_POINT_SIZE, tile->height*HFIELD_HEIGHT_STEP, outXYPos.y*NAV_POINT_SIZE);
		else
			tile_position = defField.m_position + Vector3D(outXYPos.x*NAV_POINT_SIZE, 0, outXYPos.y*NAV_POINT_SIZE);

		tile_position.x -= NAV_POINT_SIZE*0.5f;
		tile_position.z -= NAV_POINT_SIZE*0.5f;

		return tile_position;
	}

	return Vector3D(0, 0, 0);
}

float estimateDist(const IVector2D& src, const IVector2D& dest)
{
	float d;

	Vector2D dir = Vector2D(dest) - Vector2D(src);

	// Euclidian Distance
	d = length(dir);

	// Manhattan distance
	//d = fabs(dir.x)+fabs(dir.y);

	// Chebyshev distance
	//d = max(abs(dir.x), abs(dir.y));

	return d;
}

navcell_t& CGameLevel::Nav_GetCellStateAtGlobalPoint(const IVector2D& point)
{
	int navSize = m_cellsSize * AI_NAVIGATION_GRID_SCALE;

	IVector2D localPoint;
	CLevelRegion* reg;

	Nav_GlobalToLocalPoint(point, localPoint, &reg);

	CScopedMutex m(m_mutex);

	if (reg && reg->m_isLoaded)
	{
		return reg->m_navGridStateList[localPoint.y*navSize + localPoint.x];
	}

	static navcell_t emptyCell;
	emptyCell.cost = 15;
	emptyCell.flag = 1;
	emptyCell.pdir = 7;

	return emptyCell;
}

ubyte& CGameLevel::Nav_GetTileAtGlobalPoint(const IVector2D& point)
{
	int navSize = m_cellsSize * AI_NAVIGATION_GRID_SCALE;

	IVector2D localPoint;
	CLevelRegion* reg;

	Nav_GlobalToLocalPoint(point, localPoint, &reg);

	CScopedMutex m(m_mutex);

	if (reg && reg->m_navGrid)
	{
		return reg->m_navGrid[localPoint.y*navSize + localPoint.x];
	}

	static ubyte emptyTile = 255;

	return emptyTile;
}

void CGameLevel::Nav_ClearCellStates()
{
	int navSize = (m_cellsSize*AI_NAVIGATION_GRID_SCALE);

	for (int x = 0; x < m_wide; x++)
	{
		for (int y = 0; y < m_tall; y++)
		{
			int idx = y*m_wide + x;

			m_mutex.Lock();

			if(m_regions[idx].m_isLoaded)
				memset(m_regions[idx].m_navGridStateList, 0, navSize*navSize);

			m_mutex.Unlock();
		}
	}
}

bool CGameLevel::Nav_FindPath(const Vector3D& start, const Vector3D& end, pathFindResult_t& result, int iterationLimit, bool cellPriority)
{
	IVector2D startPoint = g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(start);
	IVector2D endPoint = g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(end);

	return Nav_FindPath2D(startPoint, endPoint, result, iterationLimit, cellPriority);
}

bool CGameLevel::Nav_FindPath2D(const IVector2D& start, const IVector2D& end, pathFindResult_t& result, int iterationLimit, bool cellPriority)
{
	if(start == end)
		return false;

	result.path.setNum(0,false);

	// directions
	int dx[] = NEIGHBOR_OFFS_XDX(0, (1));
	int dy[] = NEIGHBOR_OFFS_YDY(0, (1));

	// clear states before we proceed
	Nav_ClearCellStates();

	// check the start and dest points
	navcell_t& startCheck = Nav_GetCellStateAtGlobalPoint(start);
	navcell_t& endCheck = Nav_GetCellStateAtGlobalPoint(end);

	if(startCheck.flag == 0x1 || endCheck.flag == 0x1)
		return false;

	DkList<IVector2D> openSet;	// we don't need closed set
	openSet.append(start);

	bool found = false;

	int totalIterations = 0;

	IVector2D dir = end-start;

	// go through all open pos
	while (openSet.numElem() > 0)
	{
		int bestOpenSetIdx = openSet.numElem() == 1 ? 0 : -1;
		
		if(bestOpenSetIdx == -1)
		{
			float minCost = 10000.0f;

			// search for best costless point
			for (int i = 0; i < openSet.numElem(); i++)
			{
				if(openSet[i] == end)
				{
					found = true;
					break;
				}

				navcell_t& cell = Nav_GetCellStateAtGlobalPoint(openSet[i]);
				ubyte val = Nav_GetTileAtGlobalPoint(openSet[i]);

				// calc cost and check
				float g = estimateDist(end, openSet[i]);
				float h = estimateDist(openSet[i] - IVector2D(dx[cell.pdir], dy[cell.pdir]), openSet[i]);
				float cost = g + h;	// regulate by cell priority

				if(cellPriority)
				{
					cost += val;
				}
				
				if (cost < minCost)
				{
					minCost = cost;
					bestOpenSetIdx = i;
				}
			}
		}

		if(found == true)
			break;

		if (bestOpenSetIdx == -1)
			break;

		IVector2D curPoint = openSet[bestOpenSetIdx];
		navcell_t& curCell = Nav_GetCellStateAtGlobalPoint(curPoint);

		// remove from open set
		openSet.fastRemoveIndex(bestOpenSetIdx);
		curCell.flag = 0x1;	// closed

		// STOP IF WE START FAILS
		if(totalIterations > iterationLimit)
			break;

		for (ubyte i = 0; i < 8; i++)
		{
			IVector2D nextPoint(curPoint + IVector2D(dx[i], dy[i]));

			navcell_t& nextCell = Nav_GetCellStateAtGlobalPoint(nextPoint);
			ubyte next = Nav_GetTileAtGlobalPoint(nextPoint);

			if(next == 0 || nextCell.flag == 0x1)	// wall or closed
				continue;

			totalIterations++;

			nextCell.flag = 0;
			nextCell.pdir = i;

			openSet.append( nextPoint );
		}
	}

	if(found)
	{
		IVector2D testPoint = end;
		navcell_t lastCell = Nav_GetCellStateAtGlobalPoint(testPoint);

		int counter = 0;
		int lastDir = -1;

		do
		{
			// apply simplification
			if( lastDir != lastCell.pdir)
				result.path.append(testPoint);
		
			if(testPoint == start)
				break;

			IVector2D prevPoint = testPoint - IVector2D(dx[lastCell.pdir], dy[lastCell.pdir]);

			lastDir = lastCell.pdir;

			testPoint = prevPoint;
			lastCell = Nav_GetCellStateAtGlobalPoint(testPoint);
		}
		while(true);
	}

	return (result.path.numElem() > 0);
}