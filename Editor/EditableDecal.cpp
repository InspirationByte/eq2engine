//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editable decal class
//////////////////////////////////////////////////////////////////////////////////

#include "EditableDecal.h"
#include "IDebugOverlay.h"
#include "EditorLevel.h"
#include "materialsystem/IMaterialSystem.h"
#include "SelectionEditor.h"
#include "EditorMainFrame.h"

#include "EditableSurface.h"
#include "EqBrush.h"

// for string2vector
#include "EntityDef.h"

#include "Utils/GeomTools.h"

CEditableDecal::CEditableDecal()
{
	m_numAllVerts = 0;
	m_numAllIndices = 0;

	m_bbox[0] = Vector3D(-16);
	m_bbox[1] = Vector3D(16);

	m_pVB = NULL;
	m_pIB = NULL;

	m_pVertexData = NULL;
	m_pIndexData = NULL;

	m_nModifyTimes = 0;

	//m_vecNormal = Vector3D(0,1,0);
	memset(&m_surftex, 0, sizeof(m_surftex));

	// TODO: editor settings, default texture scale
	m_surftex.vScale = Vector2D(0.25f);

	m_surftex.Plane.normal = Vector3D(0,1,0); // here is unused
	m_surftex.Plane.offset = 0;

	m_surftex.UAxis.normal = Vector3D(1,0,0);
	m_surftex.UAxis.offset = 0;
	m_surftex.VAxis.normal = Vector3D(0,0,1);
	m_surftex.VAxis.offset = 0;
}

CEditableDecal::~CEditableDecal()
{
	Destroy();
}

void CEditableDecal::Destroy()
{
	if(m_pVB)
		g_pShaderAPI->DestroyVertexBuffer(m_pVB);

	m_pVB = NULL;

	if(m_pIB)
		g_pShaderAPI->DestroyIndexBuffer(m_pIB);

	m_pIB = NULL;

	m_numAllVerts = 0;
	m_numAllIndices = 0;

	m_bbox[0] = MAX_COORD_UNITS;
	m_bbox[1] = -MAX_COORD_UNITS;

	if(m_pVertexData)
		delete [] m_pVertexData;

	if(m_pIndexData)
		delete [] m_pIndexData;

	m_pIndexData = NULL;
	m_pVertexData = NULL;
}

void CEditableDecal::Render(int nViewRenderFlags)
{
	if(m_nModifyTimes || m_numAllVerts <= 0 || m_numAllIndices <= 0 || !m_pVB || !m_pIB)
		return;

	// cancel if transparents disabled
	if((nViewRenderFlags & VR_FLAG_NO_TRANSLUCENT))
		return;

	ColorRGBA color = ColorRGBA(1,1,1,1);

	if(m_nFlags & EDFL_SELECTED)
		color = ColorRGBA(1,0,0,1);

	materials->SetAmbientColor(color);

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	materials->SetCullMode(CULL_BACK);
	materials->BindMaterial(m_surftex.pMaterial, false);

	g_pShaderAPI->SetVertexFormat(g_pLevel->GetLevelVertexFormat());
	g_pShaderAPI->SetVertexBuffer(m_pVB, 0);
	g_pShaderAPI->SetIndexBuffer(m_pIB);

	materials->Apply();

	int	numDrawVerts				= m_numAllVerts;
	int	numDrawIndices				= m_numAllIndices;

	g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, 0, numDrawIndices, 0, numDrawVerts);

	 color = ColorRGBA(1,1,0,1);

	if(m_nFlags & EDFL_SELECTED)
		color = ColorRGBA(1,0,0,1);

	materials->SetAmbientColor(color);

	g_pShaderAPI->Reset(STATE_RESET_VBO);

	debugoverlay->Box3D(m_bbox[0], m_bbox[1], ColorRGBA(m_groupColor,1));

	IMeshBuilder* pBuilder = g_pShaderAPI->CreateMeshBuilder();

	Vector3D min = m_position - Vector3D(2);
	Vector3D max = m_position + Vector3D(2);

	materials->BindMaterial(g_pLevel->GetFlatMaterial(), true);

	pBuilder->Begin(PRIM_TRIANGLE_STRIP);
		pBuilder->Position3f(min.x, max.y, max.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(max.x, max.y, max.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(min.x, max.y, min.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(max.x, max.y, min.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(min.x, min.y, min.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(max.x, min.y, min.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(min.x, min.y, max.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(max.x, min.y, max.z);
		pBuilder->AdvanceVertex();

		pBuilder->Position3f(max.x, min.y, max.z);
		pBuilder->AdvanceVertex();

		pBuilder->Position3f(max.x, min.y, min.z);
		pBuilder->AdvanceVertex();

		pBuilder->Position3f(max.x, min.y, min.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(max.x, max.y, min.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(max.x, min.y, max.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(max.x, max.y, max.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(min.x, min.y, max.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(min.x, max.y, max.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(min.x, min.y, min.z);
		pBuilder->AdvanceVertex();
		pBuilder->Position3f(min.x, max.y, min.z);
		pBuilder->AdvanceVertex();
	pBuilder->End();

	g_pShaderAPI->DestroyMeshBuilder(pBuilder);
}

void CEditableDecal::RenderGhost(Vector3D &addpos, Vector3D &addscale, Vector3D &addrot, bool use_center, Vector3D &center)
{
	ColorRGBA color = ColorRGBA(m_groupColor,1);

	if(m_nFlags & EDFL_SELECTED)
		color = ColorRGBA(1,0,0,1);

	debugoverlay->Box3D(m_bbox[0], m_bbox[1], color);
}

Matrix4x4 CEditableDecal::GetRenderWorldTransform()
{
	return identity4();
}

Vector3D CEditableDecal::GetBoundingBoxMins()
{
	return m_bbox[0];
}

Vector3D CEditableDecal::GetBoundingBoxMaxs()
{
	return m_bbox[1];
}

// rendering bbox
Vector3D CEditableDecal::GetBBoxMins()
{
	return m_bbox[0];
}

Vector3D CEditableDecal::GetBBoxMaxs()
{
	return m_bbox[1];
}

void CEditableDecal::RebuildBoundingBox()
{
	m_bbox[0] = m_position-m_scale;
	m_bbox[1] = m_position+m_scale;
}

// basic mesh modifications
void CEditableDecal::Translate(Vector3D &position)
{
	//m_bbox[0]+=position;
	//m_bbox[1]+=position;

	m_position += position;

	BeginModify();
	UpdateDecalGeom();
	EndModify();
}

void CEditableDecal::Scale(Vector3D &scale, bool use_center, Vector3D &scale_center)
{
	//m_bbox[0] *= scale;
	//m_bbox[1] *= scale;

	m_scale *= scale;

	BeginModify();
	UpdateDecalGeom();
	EndModify();
}

void CEditableDecal::Rotate(Vector3D &rotation_angles, bool use_center, Vector3D &rotation_center)
{
	Matrix3x3 rotation = rotateXYZ3(DEG2RAD(rotation_angles.x),DEG2RAD(rotation_angles.y),DEG2RAD(rotation_angles.z));

	m_surftex.Plane.normal = normalize( rotation * m_surftex.Plane.normal );
}

extern bool IsAnyVertsInBBOX(CEditableSurface* pSurface, Volume &sel_box);

void AppendGeomToLists(Vector3D &origin, Volume &clipVolume, DkList<eqlevelvertex_t> &verts, DkList<int> &indices, CBaseEditableObject* pObject, bool useNormalToClip, Vector3D &vNormal)
{
	switch(pObject->GetType())
	{
		case EDITABLE_STATIC:
		case EDITABLE_TERRAINPATCH:
			{
				//Msg("Added static\n");

				CEditableSurface* pSurface = (CEditableSurface*)pObject;
				IMatVar* pNoDecals = pSurface->GetSurfaceTexture(0)->pMaterial->FindMaterialVar("nodecals");
				if(pNoDecals && pNoDecals->GetInt() > 0)
					return;

				verts.resize(verts.numElem() + pSurface->GetVertexCount());
				indices.resize(indices.numElem() + pSurface->GetIndexCount());

				// add only facing triangles

				for(int i = 0; i < pSurface->GetIndexCount(); i+=3)
				{
					int i0 = pSurface->GetIndices()[i];
					int i1 = pSurface->GetIndices()[i+1];
					int i2 = pSurface->GetIndices()[i+2];

					Plane triPlane(	pSurface->GetVertices()[i0].position,
									pSurface->GetVertices()[i1].position,
									pSurface->GetVertices()[i2].position);

					
					if(!clipVolume.IsTriangleInside(pSurface->GetVertices()[i0].position,
														pSurface->GetVertices()[i1].position,
														pSurface->GetVertices()[i2].position))
														continue;

					bool mustAdd = false;
														
					if(useNormalToClip)
						mustAdd = (dot(triPlane.normal, vNormal) > 0.0f);
					else
						mustAdd = (triPlane.Distance(origin) > 0.0f);

					//if(mustAdd)
					{
						int id0 = verts.append(pSurface->GetVertices()[i0]);
						int id1 = verts.append(pSurface->GetVertices()[i1]);
						int id2 = verts.append(pSurface->GetVertices()[i2]);

						indices.append(id0);
						indices.append(id1);
						indices.append(id2);

						//debugoverlay->Polygon3D(verts[i0].position, verts[i1].position, verts[i2].position, ColorRGBA(1,0,0,0.45), 1000);
					}
				}
			}
			break;
		case EDITABLE_BRUSH:
			{
				CEditableBrush* pBrush = (CEditableBrush*)pObject;

				for(int i = 0; i < pBrush->GetFaceCount(); i++)
				{
					IMatVar* pNoDecals = pBrush->GetFace(i)->pMaterial->FindMaterialVar("nodecals");
					if(pNoDecals && pNoDecals->GetInt() > 0)
						continue;

					bool mustAdd = false;
														
					if(useNormalToClip)
						mustAdd = (dot(pBrush->GetFace(i)->Plane.normal, vNormal) > 0.0f);
					else
						mustAdd = (pBrush->GetFace(i)->Plane.Distance(origin) > 0.0f);

					//if(mustAdd)
					{
						int nVerts = pBrush->GetFacePolygon(i)->vertices.numElem();
						int num_triangles = ((nVerts < 4) ? 1 : (2 + nVerts - 4));

						verts.resize(verts.numElem() + nVerts);
						indices.resize(indices.numElem() + num_triangles*3);

						int startvertex = verts.numElem();

						verts.append(pBrush->GetFacePolygon(i)->vertices);

						for(int j = 0; j < num_triangles; j++)
						{
							int idx0 = startvertex;
							int idx1 = startvertex+j+1;
							int idx2 = startvertex+j+2;

							//debugoverlay->Polygon3D(verts[idx0].position, verts[idx1].position, verts[idx2].position, ColorRGBA(1,0,0,0.45), 1000);

							indices.append(idx0);
							indices.append(idx1);
							indices.append(idx2);
						}
					}
				}
			}
			break;
	}
}

void SimplifyDecalGeometry(eqlevelvertex_t* pSrcVerts, int numVerts, int* pSrcIndices, int numIndices)
{


}

void CEditableDecal::UpdateDecalGeom()
{
	// get all world geometry, clip by our box like brush clips own faces and load render buffer

	RebuildBoundingBox();

	Volume clipVolume;
	clipVolume.LoadBoundingBox(m_bbox[0], m_bbox[1]);

	DkList<eqlevelvertex_t> srcverts;
	DkList<int>				srcindices;

	// test visible objects
	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		CBaseEditableObject* pObject = (CBaseEditableObject*)g_pLevel->GetEditable(i);

		if(pObject->GetType() > EDITABLE_BRUSH)
			continue;

		if(clipVolume.IsBoxInside(	pObject->GetBoundingBoxMins().x,pObject->GetBoundingBoxMaxs().x,
									pObject->GetBoundingBoxMins().y,pObject->GetBoundingBoxMaxs().y,
									pObject->GetBoundingBoxMins().z,pObject->GetBoundingBoxMaxs().z))
		{
			// add verts to list
			AppendGeomToLists(m_position, clipVolume, srcverts, srcindices, pObject, (m_surftex.nFlags & STFL_CUSTOMTEXCOORD), m_surftex.Plane.normal);
		}
	}

	// load up geometry
	m_numAllIndices = srcindices.numElem();
	m_numAllVerts = srcverts.numElem();

	if(m_pVertexData)
		delete [] m_pVertexData;

	if(m_pIndexData)
		delete [] m_pIndexData;

	m_pVertexData = new eqlevelvertex_t[m_numAllVerts];
	m_pIndexData = new int[m_numAllIndices];

	memcpy(m_pVertexData, srcverts.ptr(), sizeof(eqlevelvertex_t) * srcverts.numElem());
	memcpy(m_pIndexData, srcindices.ptr(), sizeof(int) * srcindices.numElem());

	for(int i = 0; i < 6; i++)
	{
		Plane pl = clipVolume.GetPlane(i);
		ClipDecal( pl );
	}

	// simplify to the planes
	SimplifyDecalGeometry(m_pVertexData, m_numAllVerts, m_pIndexData, m_numAllIndices);

	Vector3D box_size = m_bbox[1]-m_bbox[0];

	if(m_surftex.nFlags & STFL_CUSTOMTEXCOORD)
	{

		int texSizeW = 1;
		int texSizeH = 1;

		ITexture* pTex = m_surftex.pMaterial->GetBaseTexture();

		if(pTex)
		{
			texSizeW = pTex->GetWidth();
			texSizeH = pTex->GetHeight();
		}

		Vector3D axisAngles = VectorAngles(m_surftex.Plane.normal);
		AngleVectors(axisAngles, NULL, &m_surftex.UAxis.normal, &m_surftex.VAxis.normal);

		m_surftex.VAxis.normal *= -1;

		//VectorVectors(m_surftex.Plane.normal, m_surftex.UAxis.normal, m_surftex.VAxis.normal);

		Matrix3x3 texMatrix(m_surftex.UAxis.normal, m_surftex.VAxis.normal, m_surftex.Plane.normal);

		Matrix3x3 rotationMat = rotateZXY3( 0.0f, 0.0f, DEG2RAD(m_surftex.fRotation));
		texMatrix = rotationMat*texMatrix;

		m_surftex.UAxis.normal = texMatrix.rows[0];
		m_surftex.VAxis.normal = texMatrix.rows[1];

		for(int i = 0; i < m_numAllVerts; i++)
		{
			float one_over_w = 1.0f / fabs(dot(box_size,m_surftex.UAxis.normal*m_surftex.UAxis.normal) * m_surftex.vScale.x);
			float one_over_h = 1.0f / fabs(dot(box_size,m_surftex.VAxis.normal*m_surftex.VAxis.normal) * m_surftex.vScale.y);

			float U, V;
		
			U = dot(m_surftex.UAxis.normal, m_position-m_pVertexData[i].position) * one_over_w + 0.5f;
			V = dot(m_surftex.VAxis.normal, m_position-m_pVertexData[i].position) * one_over_h - 0.5f;

			m_pVertexData[i].texcoord.x = U;
			m_pVertexData[i].texcoord.y = -V;
		}
	}
	else
	{
		// generate new texture coordinates
		for(int i = 0; i < m_numAllVerts; i++)
		{
			float one_over_w = 1.0f / fabs(dot(box_size,m_pVertexData[i].tangent*m_pVertexData[i].tangent));
			float one_over_h = 1.0f / fabs(dot(box_size,m_pVertexData[i].binormal*m_pVertexData[i].binormal));

			m_pVertexData[i].texcoord.x = abs(dot(m_position-m_pVertexData[i].position,m_pVertexData[i].tangent*sign(m_pVertexData[i].tangent))*one_over_w+0.5f);
			m_pVertexData[i].texcoord.y = abs(dot(m_position-m_pVertexData[i].position,m_pVertexData[i].binormal*sign(m_pVertexData[i].binormal))*one_over_h+0.5f);
		}
	}
}

void CEditableDecal::ClipDecal(Plane &plane)
{
	DkList<eqlevelvertex_t> new_vertices;
	DkList<int>				new_indices;

	new_vertices.resize(m_numAllIndices);
	new_indices.resize(m_numAllIndices);

	for(int i = 0; i < m_numAllIndices; i += 3)
	{
		eqlevelvertex_t v[3];
		float d[3];

		for (int j = 0; j < 3; j++)
		{
			v[j] = m_pVertexData[m_pIndexData[i + j]];
			d[j] = plane.Distance(v[j].position);
		}

		if (d[0] >= 0 && d[1] >= 0 && d[2] >= 0)
		{
			new_indices.append(new_vertices.numElem());
			new_indices.append(new_vertices.numElem() + 1);
			new_indices.append(new_vertices.numElem() + 2);

			new_vertices.append(m_pVertexData[m_pIndexData[i]]);
			new_vertices.append(m_pVertexData[m_pIndexData[i+1]]);
			new_vertices.append(m_pVertexData[m_pIndexData[i+2]]);

			/*
			if (front)
			{
				front->copyVertex(nFrontVertices++, *this, i);
				front->copyVertex(nFrontVertices++, *this, i + 1);
				front->copyVertex(nFrontVertices++, *this, i + 2);
			}
			*/
		}
		else if (d[0] < 0 && d[1] < 0 && d[2] < 0)
		{
			/*
			if (back)
			{
				back->copyVertex(nBackVertices++, *this, i);
				back->copyVertex(nBackVertices++, *this, i + 1);
				back->copyVertex(nBackVertices++, *this, i + 2);
			}
			*/
		} 
		else 
		{
			int front0 = 0;
			int prev = 2;
			for (int k = 0; k < 3; k++)
			{
				if (d[prev] < 0 && d[k] >= 0)
				{
					front0 = k;
					break;
				}
				prev = k;
			}

			int front1 = (front0 + 1) % 3;
			int front2 = (front0 + 2) % 3;
			if (d[front1] >= 0)
			{
				float x0 = d[front1] / dot(plane.normal, v[front1].position - v[front2].position);
				float x1 = d[front0] / dot(plane.normal, v[front0].position - v[front2].position);

				new_indices.append(new_vertices.numElem());
				new_indices.append(new_vertices.numElem() + 1);
				new_indices.append(new_vertices.numElem() + 2);

				new_vertices.append(m_pVertexData[m_pIndexData[i+front0]]);
				new_vertices.append(m_pVertexData[m_pIndexData[i+front1]]);
				new_vertices.append(InterpolateLevelVertex(m_pVertexData[m_pIndexData[i+front1]], m_pVertexData[m_pIndexData[i+front2]], x0));

				new_indices.append(new_vertices.numElem());
				new_indices.append(new_vertices.numElem() + 1);
				new_indices.append(new_vertices.numElem() + 2);

				new_vertices.append(m_pVertexData[m_pIndexData[i+front0]]);
				new_vertices.append(InterpolateLevelVertex(m_pVertexData[m_pIndexData[i+front1]], m_pVertexData[m_pIndexData[i+front2]], x0));
				new_vertices.append(InterpolateLevelVertex(m_pVertexData[m_pIndexData[i+front0]], m_pVertexData[m_pIndexData[i+front2]], x1));
			} 
			else 
			{
				float x0 = d[front0] / dot(plane.normal, v[front0].position - v[front1].position);
				float x1 = d[front0] / dot(plane.normal, v[front0].position - v[front2].position);

				new_indices.append(new_vertices.numElem());
				new_indices.append(new_vertices.numElem() + 1);
				new_indices.append(new_vertices.numElem() + 2);

				new_vertices.append(m_pVertexData[m_pIndexData[i+front0]]);
				new_vertices.append(InterpolateLevelVertex(m_pVertexData[m_pIndexData[i+front0]], m_pVertexData[m_pIndexData[i+front1]], x0));
				new_vertices.append(InterpolateLevelVertex(m_pVertexData[m_pIndexData[i+front0]], m_pVertexData[m_pIndexData[i+front2]], x1));
			}
		}
	}

	if(new_indices.numElem() > 0)
	{
		DkList<int>				indices;
		DkList<eqlevelvertex_t> vertices;

		indices.resize(m_numAllIndices);
		vertices.resize(m_numAllIndices);

		for(int i = 0; i < new_indices.numElem(); i++)
		{
			int idx = vertices.findIndex(new_vertices[i]);
			if(idx == -1)
				idx = vertices.append(new_vertices[i]);

			indices.append(idx);
		}

		new_indices.clear();
		new_vertices.clear();

		m_numAllVerts = vertices.numElem();
		m_numAllIndices = indices.numElem();

		if(m_pVertexData)
		{
			delete [] m_pVertexData;
			m_pVertexData = NULL;
		}

		if(m_pIndexData)
		{
			delete [] m_pIndexData;
			m_pIndexData = NULL;
		}

		m_pVertexData = new eqlevelvertex_t[m_numAllVerts];
		m_pIndexData = new int[m_numAllIndices];

		memcpy(m_pVertexData, vertices.ptr(), sizeof(eqlevelvertex_t) * vertices.numElem());
		memcpy(m_pIndexData, indices.ptr(), sizeof(int) * indices.numElem());
	}
}

// begin/end modification (end unlocks VB)
void CEditableDecal::BeginModify()
{
	m_nModifyTimes++;
}

void CEditableDecal::EndModify()
{
	if(m_nModifyTimes > 1)
		m_nModifyTimes--;

	if(m_nModifyTimes != 1 || m_numAllVerts <= 0 || m_numAllIndices <= 0 || !m_pVertexData || !m_pIndexData)
		return;

	// can lock VBO or not?
	bool update_only = true;
	
	if(m_pVB)
		g_pShaderAPI->DestroyVertexBuffer(m_pVB);

	m_pVB = NULL;
	
	if(m_pIB)
		g_pShaderAPI->DestroyIndexBuffer(m_pIB);

	m_pIB = NULL;
	
	// create buffers if not exist
	if(!m_pVB)
	{
		m_pVB = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, m_numAllVerts, sizeof(eqlevelvertex_t), m_pVertexData);
		update_only = false;
	}

	if(!m_pIB)
	{
		m_pIB = g_pShaderAPI->CreateIndexBuffer(m_numAllIndices, 4, BUFFER_DYNAMIC, m_pIndexData);
		//Msg("EQIB ptr created: %p\n", m_pIB);
		update_only = false;
	}

	// only update
	if(update_only)
	{
		g_pShaderAPI->Reset(STATE_RESET_VBO);

		eqlevelvertex_t* vbo_vertex_data = NULL;
		if(m_pVB->Lock(0,m_numAllVerts,(void**)&vbo_vertex_data,false))
		{
			// copy to vertex buffer
			memcpy(vbo_vertex_data, m_pVertexData, m_numAllVerts*sizeof(eqlevelvertex_t));

			m_pVB->Unlock();
		}

		//Msg("EQIB ptr: %p\n", m_pIB);

		int* indexdata = NULL;
		if(m_pIB->Lock(0, m_numAllIndices, (void**)&indexdata, false))
		{
			// copy to index buffer
			memcpy(indexdata, m_pIndexData, m_numAllIndices*sizeof(int));

			m_pIB->Unlock();
		}
	}

	m_nModifyTimes--;
}

float CEditableDecal::CheckLineIntersection(const Vector3D &start, const Vector3D &end,Vector3D &outPos)
{
	Vector3D ray_dir = end-start;

	Vector3D min = m_position - Vector3D(8);
	Vector3D max = m_position + Vector3D(8);

	Volume bboxVolume;
	bboxVolume.LoadBoundingBox(min, max);

	if(bboxVolume.IsIntersectsRay(start, normalize(ray_dir), outPos))
	{
		float frac = lineProjection(start,end,outPos);

		if(frac < 1 && frac > 0)
			return frac;
	}

	return 1.0f;
}

void CEditableDecal::OnRemove(bool bOnLevelCleanup)
{
	Destroy();
}

// saves this object
bool CEditableDecal::WriteObject(IVirtualStream*	pStream)
{
	// prepare and write texture projection info
	ReadWriteSurfaceTexture_t surfData;
	surfData.Plane = m_surftex.Plane;
	surfData.UAxis = m_surftex.UAxis;
	surfData.VAxis = m_surftex.VAxis;
	surfData.scale = m_surftex.vScale;
	surfData.nFlags = m_surftex.nFlags;

	if(m_surftex.pMaterial)
		strcpy(surfData.material, m_surftex.pMaterial->GetName());
	else
		strcpy(surfData.material, "ERROR");

	pStream->Write(&surfData, 1, sizeof(ReadWriteSurfaceTexture_t));

	pStream->Write(m_position, 1, sizeof(Vector3D));
	pStream->Write(m_scale, 1, sizeof(Vector3D));

	return true;
}

// read this object
bool CEditableDecal::ReadObject(IVirtualStream*	pStream)
{
	// read texture projection info
	ReadWriteSurfaceTexture_t surfData;

	pStream->Read(&surfData, 1, sizeof(ReadWriteSurfaceTexture_t));

	m_surftex.Plane = surfData.Plane;
	m_surftex.UAxis = surfData.UAxis;
	m_surftex.VAxis = surfData.VAxis;
	m_surftex.vScale = surfData.scale;
	m_surftex.nFlags = surfData.nFlags;

	m_surftex.pMaterial = materials->FindMaterial(surfData.material,true);

	pStream->Read(m_position, 1, sizeof(Vector3D));
	pStream->Read(m_scale, 1, sizeof(Vector3D));

	BeginModify();
	UpdateDecalGeom();
	EndModify();

	return true;
}

// stores object in keyvalues
void CEditableDecal::SaveToKeyValues(kvkeybase_t* pSection)
{
	pSection->AddKeyBase("origin", varargs("%g %g %g", m_position.x, m_position.y, m_position.z));
	pSection->AddKeyBase("size", varargs("%g %g %g", m_scale.x, m_scale.y, m_scale.z));
	pSection->AddKeyBase("material", (char*)m_surftex.pMaterial->GetName());

	pSection->AddKeyBase("nplane", varargs("%f %f %f %f", 
		m_surftex.Plane.normal.x,m_surftex.Plane.normal.y,m_surftex.Plane.normal.z,
		m_surftex.Plane.offset
		));

	pSection->AddKeyBase("uplane", varargs("%f %f %f %f", 
		m_surftex.UAxis.normal.x,m_surftex.UAxis.normal.y,m_surftex.UAxis.normal.z,
		m_surftex.UAxis.offset
		));

	pSection->AddKeyBase("vplane", varargs("%f %f %f %f", 
		m_surftex.VAxis.normal.x,m_surftex.VAxis.normal.y,m_surftex.VAxis.normal.z,
		m_surftex.VAxis.offset
		));

	pSection->AddKeyBase("texscale", varargs("%f %f", 
		m_surftex.vScale.x,m_surftex.vScale.y
		));

	pSection->AddKeyBase("rotation", varargs("%f", m_surftex.fRotation));
	pSection->AddKeyBase("customtex", varargs("%d", (m_surftex.nFlags & STFL_CUSTOMTEXCOORD)));
}

// stores object in keyvalues
bool CEditableDecal::LoadFromKeyValues(kvkeybase_t* pSection)
{
	kvkeybase_t* pair = pSection->FindKeyBase("origin");
	if(pair)
		m_position = UTIL_StringToColor3(pair->values[0]);

	pair = pSection->FindKeyBase("size");
	if(pair)
		m_scale = UTIL_StringToColor3(pair->values[0]);

	pair = pSection->FindKeyBase("material");
	if(pair)
		m_surftex.pMaterial = materials->FindMaterial(pair->values[0], true);

	pair = pSection->FindKeyBase("nplane");

	Vector4D plane;

	if(pair)
	{
		plane = UTIL_StringToColor4(pair->values[0]);

		m_surftex.Plane.normal = plane.xyz();
		m_surftex.Plane.offset = plane.w;
	}

	pair = pSection->FindKeyBase("uplane");
	if(pair)
	{
		plane = UTIL_StringToColor4(pair->values[0]);

		m_surftex.UAxis.normal = plane.xyz();
		m_surftex.UAxis.offset = plane.w;
	}

	pair = pSection->FindKeyBase("vplane");
	if(pair)
	{
		plane = UTIL_StringToColor4(pair->values[0]);

		m_surftex.VAxis.normal = plane.xyz();
		m_surftex.VAxis.offset = plane.w;
	}

	pair = pSection->FindKeyBase("texscale");
	if(pair)
		m_surftex.vScale = UTIL_StringToVector2(pair->values[0]);

	pair = pSection->FindKeyBase("rotation");
	if(pair)
		m_surftex.fRotation = atof(pair->values[0]);

	pair = pSection->FindKeyBase("customtex");
	if(pair)
		m_surftex.nFlags = atoi(pair->values[0]) ? STFL_CUSTOMTEXCOORD : 0;

	BeginModify();
	UpdateDecalGeom();
	EndModify();

	return true;
}

// copies this object
CBaseEditableObject* CEditableDecal::CloneObject()
{
	CEditableDecal* pDecal = new CEditableDecal;
	pDecal->SetPosition(GetPosition());
	pDecal->SetScale(GetScale());
	pDecal->SetName(GetName());

	pDecal->BeginModify();
	pDecal->UpdateDecalGeom();
	pDecal->EndModify();

	pDecal->m_surftex = m_surftex;

	return pDecal;
}