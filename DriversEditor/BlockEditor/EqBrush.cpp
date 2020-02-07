//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Convex polyhedra, for engine/editor/compiler
//////////////////////////////////////////////////////////////////////////////////

#include "EqBrush.h"
#include <malloc.h>

#include "world.h"

#include "IDebugOverlay.h"
#include "coord.h"
#include "Utils/GeomTools.h"

#define MAX_BRUSH_PLANES		256
#define MAX_POINTS_ON_WINDING	128
#define MAX_WINDING_VERTS		256

int ClipVertsAgainstPlane(Plane &plane, int vertexCount, Vector3D *vertex, Vector3D *newVertex)
{
	bool negative[MAX_WINDING_VERTS];
	float t;
	Vector3D v, v1, v2;

	// classify vertices
	int negativeCount = 0;
	for (int i = 0; i < vertexCount; i++)
	{
		negative[i] = (plane.ClassifyPoint(vertex[i]) != CP_FRONT);
		negativeCount += negative[i];
	}
	
	if(negativeCount == vertexCount)
		return 0; // completely culled, we have no polygon

	if(negativeCount == 0)
		return -1;			// not clipped

	int count = 0;

	for (int i = 0; i < vertexCount; i++)
	{
		// prev is the index of the previous vertex
		int	prev = (i != 0) ? i - 1 : vertexCount - 1;
		
		if (negative[i])
		{
			if (!negative[prev])
			{
				Vector3D v1 = vertex[prev];
				Vector3D v2 = vertex[i];

				// Current vertex is on negative side of plane,
				// but previous vertex is on positive side.
				Vector3D edge = v1 - v2;
				t = plane.Distance(v1) / dot(plane.normal, edge);

				newVertex[count] = lerp(v1,v2,t);
				count++;
			}
		}
		else
		{
			if (negative[prev])
			{
				Vector3D v1 = vertex[i];
				Vector3D v2 = vertex[prev];

				// Current vertex is on positive side of plane,
				// but previous vertex is on negative side.
				Vector3D edge = v1 - v2;

				t = plane.Distance(v1) / dot(plane.normal, edge);

				newVertex[count] = lerp(v1,v2,t);
				
				count++;
			}
			
			// include current vertex
			newVertex[count] = vertex[i];
			count++;
		}
	}
	
	// return new clipped vertex count
	return count;
}

void ChopWinding(winding_t &w, Plane &plane)
{
	Vector3D newVertices[MAX_POINTS_ON_WINDING];
	Vector3D vertices[MAX_POINTS_ON_WINDING];

	int nVerts = w.vertices.numElem();

	for(int i = 0; i < nVerts; i++)
		vertices[i] = w.vertices[i].position;

	int nNewVerts = ClipVertsAgainstPlane(plane, nVerts, vertices, newVertices);
	if(nNewVerts == -1)
		return;

	w.vertices.clear();

	for(int i = 0; i < nNewVerts; i++)
	{
		lmodeldrawvertex_t vertex(newVertices[i], vec3_zero, vec2_zero);
		w.vertices.append(vertex);
	}
}

/**/

void MakeInfiniteWinding(winding_t &w, Plane &plane)
{
	// compute needed vectors
	Vector3D vRight, vUp;
	VectorVectors(plane.normal, vRight, vUp);

	// make vectors infinite
	vRight *= MAX_COORD_UNITS;
	vUp *= MAX_COORD_UNITS;

	Vector3D org = -plane.normal * plane.offset;

	// and finally fill really big winding
	w.vertices.append(lmodeldrawvertex_t(org - vRight + vUp, plane.normal, vec2_zero));
	w.vertices.append(lmodeldrawvertex_t(org + vRight + vUp, plane.normal, vec2_zero));
	w.vertices.append(lmodeldrawvertex_t(org + vRight - vUp, plane.normal, vec2_zero));
	w.vertices.append(lmodeldrawvertex_t(org - vRight - vUp, plane.normal, vec2_zero));
}

//-----------------------------------------
// Brush face member functions
//-----------------------------------------

// calculates the texture coordinates for this 
void winding_t::CalculateTextureCoordinates()
{
	int texSizeW = 32;
	int texSizeH = 32;

	ITexture* pTex = pAssignedFace->pMaterial->GetBaseTexture();

	if(pTex)
	{
		texSizeW = pTex->GetWidth();
		texSizeH = pTex->GetHeight();
	}

	Vector3D axisAngles = VectorAngles(pAssignedFace->Plane.normal);
	AngleVectors(axisAngles, NULL, &pAssignedFace->UAxis.normal, &pAssignedFace->VAxis.normal);

	pAssignedFace->VAxis.normal *= -1;
	//VectorVectors(pAssignedFace->Plane.normal, pAssignedFace->UAxis.normal, pAssignedFace->VAxis.normal);

	Matrix3x3 texMatrix(pAssignedFace->UAxis.normal, pAssignedFace->VAxis.normal, pAssignedFace->Plane.normal);

	Matrix3x3 rotationMat = rotateZXY3( 0.0f, 0.0f, DEG2RAD(pAssignedFace->fRotation));
	texMatrix = rotationMat*texMatrix;

	pAssignedFace->UAxis.normal = -texMatrix.rows[0];
	pAssignedFace->VAxis.normal = texMatrix.rows[1];
	
	for(int i = 0; i < vertices.numElem(); i++ )
	{
		float U, V;
		
		U = dot(pAssignedFace->UAxis.normal, vertices[i].position);
		U /= ( ( float )texSizeW ) * pAssignedFace->vScale.x;
		U += ( pAssignedFace->UAxis.offset / ( float )texSizeW );

		V = dot(pAssignedFace->VAxis.normal, vertices[i].position);
		V /= ( ( float )texSizeH ) * pAssignedFace->vScale.y;
		V += ( pAssignedFace->VAxis.offset / ( float )texSizeH );

		vertices[i].texcoord.x = U;
		vertices[i].texcoord.y = V;

		//vertices[i].tangent = vec3_zero;
		//vertices[i].binormal = vec3_zero;
		vertices[i].normal = vec3_zero;
	}

	int num_triangles = ((vertices.numElem() < 4) ? 1 : (2 + vertices.numElem() - 4));

	for(int i = 0; i < num_triangles; i++ )
	{
		int idx0 = 0;
		int idx1 = i+1;
		int idx2 = i+2;

		Vector3D t,b,n;
		ComputeTriangleTBN(	vertices[idx0].position, vertices[idx1].position, vertices[idx2].position,
							vertices[idx0].texcoord, vertices[idx1].texcoord, vertices[idx2].texcoord, 
							n,t,b);

		//vertices[idx0].tangent += t;
		//vertices[idx0].binormal += b;
		vertices[idx0].normal += n;

		//vertices[idx1].tangent += t;
		//vertices[idx1].binormal += b;
		vertices[idx1].normal += n;

		//vertices[idx2].tangent += t;
		//vertices[idx2].binormal += b;
		vertices[idx2].normal += n;
	}

	for(int i = 0; i < vertices.numElem(); i++)
	{
		//vertices[i].tangent = normalize(vertices[i].tangent);
		//vertices[i].binormal = normalize(vertices[i].binormal);
		vertices[i].normal = normalize(vertices[i].normal);

		//debugoverlay->Line3D(vertices[i].position, vertices[i].position + vertices[i].tangent*10,ColorRGBA(1,0,0,1),ColorRGBA(1,0,0,1),10);
		//debugoverlay->Line3D(vertices[i].position, vertices[i].position + vertices[i].binormal*10,ColorRGBA(0,1,0,1),ColorRGBA(0,1,0,1),10);
		//debugoverlay->Line3D(vertices[i].position, vertices[i].position + vertices[i].normal*10,ColorRGBA(0,0,1,1),ColorRGBA(0,0,1,1),10);
	}
}

// sorts the vertices, makes them as triangle list
bool winding_t::SortVerticesAsTriangleList()
{
	if(vertices.numElem() < 3)
	{
		MsgError("ERROR: Polygon has less than 3 vertices (%d)\n", vertices.numElem());
		return true;
	}

	// get the center of that poly
	Vector3D center(0);
	for ( int i = 0; i < vertices.numElem(); i++ )
		center += vertices[i].position;

	center /= vertices.numElem();

	// sort them now
	for ( int i = 0; i < vertices.numElem()-2; i++ )
	{
		float	fSmallestAngle	= -1;
		int		nSmallestIdx	= -1;

		Vector3D a = normalize(vertices[i].position - center);
		Plane p( vertices[i].position, center, center + pAssignedFace->Plane.normal );

		for(int j = i+1; j < vertices.numElem(); j++)
		{
			// the vertex is in front or lies on the plane
			if (p.ClassifyPoint (vertices[j].position) != CP_BACK)
			{
				Vector3D b = normalize(vertices[j].position - center);

				float fAngle = dot(b,a);

				if ( fAngle > fSmallestAngle )
				{
					fSmallestAngle	= fAngle;
					nSmallestIdx	= j;
				}
				/*
				Vector3D checkIVS = normalize(vertices[j].position - vertices[i].position);

				//if(dot(a, checkIVS) < 0.0f)
				{
					Msg("NOT A CONVEX POLYGON (%g)\n", dot(a, checkIVS));
					//return false;
				}*/
			}
		}

		if (nSmallestIdx == -1)
		{
			return false;
		}

		lmodeldrawvertex_t vtx = vertices[nSmallestIdx];
		vertices[ nSmallestIdx ] = vertices[i+1];
		vertices[i+1] = vtx;
	}

	return true;
}

// splits the face by this face, and results a
void winding_t::Split(const winding_t *w, winding_t *front, winding_t *back )
{
	ClassifyPlane_e	*pCP = (ClassifyPlane_e*)stackalloc(w->vertices.numElem()*sizeof(ClassifyPlane_e));

	for ( int i = 0; i < w->vertices.numElem(); i++ )
		pCP[ i ] = pAssignedFace->Plane.ClassifyPoint ( w->vertices[i].position );

	front->pAssignedBrush	= w->pAssignedBrush;
	back->pAssignedBrush	= w->pAssignedBrush;

	front->pAssignedFace	= w->pAssignedFace;
	back->pAssignedFace		= w->pAssignedFace;

	for ( int i = 0; i < w->vertices.numElem(); i++ )
	{
		//
		// Add point to appropriate list
		//
		switch (pCP[i])
		{
			case CP_FRONT:
			{
				front->vertices.append(w->vertices[i]);
			}
			break;
			case CP_BACK:
			{
				back->vertices.append(w->vertices[i]);
			}
			break;
			case CP_ONPLANE:
			{
				front->vertices.append(w->vertices[i]);
				back->vertices.append(w->vertices[i]);
			}
			break;
		}

		//
		// Check if edges should be split
		//
		int		iNext	= i + 1;
		bool	bIgnore	= false;

		if ( i == w->vertices.numElem()-1)
			iNext = 0;

		if((pCP[i] == CP_ONPLANE) && (pCP[iNext] != CP_ONPLANE))
			bIgnore = true;
		else if((pCP[iNext] == CP_ONPLANE) && ( pCP[i] != CP_ONPLANE))
			bIgnore = true;

		if (!bIgnore && (pCP[i] != pCP[iNext]))
		{
			lmodeldrawvertex_t	v;	// New vertex created by splitting
			float p;			// Percentage between the two points

			pAssignedFace->Plane.GetIntersectionLineFraction(w->vertices[i].position, w->vertices[iNext].position, v.position, p );

			v.texcoord = lerp(w->vertices[i].texcoord, w->vertices[iNext].texcoord, p);

			front->vertices.append(v);
			back->vertices.append(v);
		}
	}
}

// classifies the next face over this
ClassifyPoly_e winding_t::Classify(winding_t *w)
{
	bool	bFront = false, bBack = false;
	float	dist;

	for ( int i = 0; i < w->vertices.numElem(); i++ )
	{
		dist = pAssignedFace->Plane.Distance(w->vertices[i].position);

		if ( dist > 0.0001f )
		{
			if ( bBack )
				return CPL_SPLIT;

			bFront = true;
		}
		else if ( dist < -0.0001f )
		{
			if ( bFront )
				return CPL_SPLIT;

			bBack = true;
		}
	}

	if ( bFront )
		return CPL_FRONT;
	else if ( bBack )
		return CPL_BACK;

	return CPL_ONPLANE;
}
/*
// makes editable surface from this winding (and no way back)
CEditableSurface* EqBrushWinding_t::MakeEditableSurface()
{
	CEditableSurface* pSurface = new CEditableSurface;

	DkList<int>	indices;

	int num_triangles = ((vertices.numElem() < 4) ? 1 : (2 + vertices.numElem() - 4));
	int num_indices = num_triangles*3;

	for(int i = 0; i < num_triangles; i++)
	{
		int idx0 = 0;
		int idx1 = i+1;
		int idx2 = i+2;

		indices.append(idx0);
		indices.append(idx1);
		indices.append(idx2);
	}

	pSurface->MakeCustomMesh(vertices.ptr(), indices.ptr(), vertices.numElem(), indices.numElem());
	pSurface->SetMaterial(pAssignedFace->pMaterial);

	return pSurface;
}
*/
//-----------------------------------------
// Brush member functions
//-----------------------------------------

CEditableBrush::CEditableBrush()
{
	m_pVB = NULL;
	m_nAdditionalFlags = 0;
}

CEditableBrush::~CEditableBrush()
{
	OnRemove();
}

// calculates bounding box for this brush
void CEditableBrush::CalculateBBOX()
{
	m_bbox.Reset();

	for(int i = 0; i < m_polygons.numElem(); i++)
	{
		for(int j = 0; j < m_polygons[i].vertices.numElem(); j++)
			m_bbox.AddVertex(m_polygons[i].vertices[j].position);
	}
}

// is brush aabb intersects another brush aabb
bool CEditableBrush::IsBrushIntersectsAABB(CEditableBrush *pBrush)
{
	BoundingBox another_box(pBrush->GetBBoxMins(),pBrush->GetBBoxMaxs());

	return m_bbox.Intersects(another_box);
}

void CEditableBrush::SortVertsToDraw()
{
	for(int i = 0; i < m_polygons.numElem(); i++)
	{
		if(!m_polygons[i].SortVerticesAsTriangleList())
			Msg("\nERROR: Degenerate plane %d on brush\n", i);
	}
}

// calculates the vertices from faces
bool CEditableBrush::CreateFromPlanes()
{
	// check planes first
	bool usePlane[MAX_BRUSH_PLANES];
	memset(usePlane,0,sizeof(usePlane));

	for ( int i = 0; i < m_faces.numElem(); i++ )
	{
		if(m_faces[i].Plane.normal == vec3_zero)
		{
			Msg("Brush: Invalid plane %d\n", i);
			usePlane[i] = false;
			continue;
		}

		usePlane[i] = true;
		for (int j = 0; j < i; j++)
		{
			brushFace_t* faceCheck = GetFace(j);

			// check duplicate planes
			if((dot(m_faces[i].Plane.normal, faceCheck->Plane.normal) > 0.999) && (fabs(m_faces[i].Plane.offset - faceCheck->Plane.offset) < 0.01))
			{
				Msg("Brush: Duplicate plane found %d\n", j);
				usePlane[j] = false;
			}
		}
	}
	
	m_polygons.clear();

	for ( int i = 0; i < m_faces.numElem(); i++ )
	{
		if(!usePlane[i])
		{
			m_faces.removeIndex(i);
			i--;
			continue;
		}

		winding_t poly;
		poly.pAssignedBrush = this;

		MakeInfiniteWinding(poly, m_faces[i].Plane);

		for ( int j = 0; j < m_faces.numElem(); j++ )
		{
			if(j != i)
			{
				Plane clipPlane;
				clipPlane.normal = -m_faces[j].Plane.normal;
				clipPlane.offset = -m_faces[j].Plane.offset;

				ChopWinding(poly, clipPlane);
			}
		}

		poly.pAssignedFace = &m_faces[i];
		m_polygons.append(poly);
	}

	// remove empty faces
	RemoveEmptyFaces();

	// and sort vertices
	SortVertsToDraw();

	return true;
}

void CEditableBrush::RemoveEmptyFaces()
{
	// remove empty faces
	for(int i = 0; i < m_polygons.numElem(); i++)
	{
		if(m_polygons[i].vertices.numElem() < 3)
		{
			m_polygons.removeIndex(i);
			m_faces.removeIndex(i);
			i--;
		}
	}
};

// draw brush
void CEditableBrush::Render(int nViewRenderFlags)
{
	materials->SetCullMode(CULL_BACK);
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	g_pShaderAPI->SetVertexFormat(g_worldGlobals.levelObjectVF);
	g_pShaderAPI->SetVertexBuffer(m_pVB,0);
	g_pShaderAPI->SetIndexBuffer(NULL);

	Matrix4x4 view;
	materials->GetMatrix(MATRIXMODE_VIEW, view);

	ColorRGBA ambientColor = color4_white;

	int nFirstVertex = 0;
	for(int i = 0; i < m_polygons.numElem(); i++)
	{
		winding_t& winding = m_polygons[i];

		/*
		// cancel if transparents disabled
		if((nViewRenderFlags & VR_FLAG_NO_TRANSLUCENT) && (winding.pAssignedFace->pMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT))
		{
			nFirstVertex += polygons[i].vertices.numElem()+1;
			continue;
		}
		
		// cancel if opaques disabled
		if((nViewRenderFlags & VR_FLAG_NO_OPAQUE) && !(winding.pAssignedFace->pMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT))
		{
			nFirstVertex += winding.vertices.numElem()+1;
			continue;
		}*/

		/*
		if((polygons[i].pAssignedFace->nFlags & STFL_SELECTED) && !g_editormainframe->GetSurfaceDialog()->IsSelectionMaskDisabled())
		{
			ColorRGBA sel_color(1.0f,0.0f,0.0f,1.0f);
			materials->SetAmbientColor(sel_color);
		}
		else
		*/
			materials->SetAmbientColor(ambientColor);

		// apply the material (slow in editor)
		materials->BindMaterial(winding.pAssignedFace->pMaterial, 0);
		materials->Apply();

		g_pShaderAPI->DrawNonIndexedPrimitives(PRIM_TRIANGLE_FAN, nFirstVertex, winding.vertices.numElem());
		/*
		if((polygons[i].pAssignedFace->nFlags & STFL_SELECTED) && !g_editormainframe->GetSurfaceDialog()->IsSelectionMaskDisabled())
		{
			ColorRGBA sel_color(1.0f,0.0f,0.0f,0.35f);
			materials->SetAmbientColor(sel_color);

			materials->BindMaterial(g_pLevel->GetFlatMaterial(), 0);
			materials->SetDepthStates(true, false);

			materials->Apply();
			g_pShaderAPI->DrawNonIndexedPrimitives(PRIM_TRIANGLE_FAN, nFirstVertex, polygons[i].vertices.numElem());
		}*/

		nFirstVertex += winding.vertices.numElem()+1;
	}

	/*
	IMeshBuilder* pMesh = g_pShaderAPI->CreateMeshBuilder();
	for(int i = 0; i < polygons.numElem(); i++)
	{
		// apply the material (slow in editor)
		materials->BindMaterial(polygons[i].pAssignedFace->pMaterial);

		pMesh->Begin(PRIM_TRIANGLE_FAN);
		for(int j = 0; j < polygons[i].vertices.numElem(); j++)
		{
			pMesh->Position3fv(polygons[i].vertices[j].position);
			pMesh->Color3f(1.0f,0,0);
			pMesh->AdvanceVertex();
		}
		pMesh->End();
	}
	g_pShaderAPI->DestroyMeshBuilder(pMesh);
	*/
}

void CEditableBrush::AddFace(brushFace_t &face)
{
	m_faces.append(face);
}

/*
void CEditableBrush::AddFace(Texture_t &face, EqBrushWinding_t &winding)
{
	int face_id = faces.append(face);

	winding.pAssignedFace = &faces[face_id];
	polygons.append(winding);
}
*/
void CEditableBrush::UpdateRenderData()
{
	CreateFromPlanes();
	CalculateBBOX();

	UpdateRenderBuffer();
}

void CEditableBrush::UpdateRenderBuffer()
{
	// adjust flags

	// reset first!
	m_nAdditionalFlags = 0;

	for(int i = 0; i < m_polygons.numElem(); i++)
	{
		winding_t& winding = m_polygons[i];

		winding.CalculateTextureCoordinates();
		/*
		IMatVar* pVar = winding.pAssignedFace->pMaterial->FindMaterialVar("roomfiller");

		if(pVar && pVar->GetInt() > 0)
			m_nAdditionalFlags |= EDFL_ROOM;

		pVar = winding.pAssignedFace->pMaterial->FindMaterialVar("areaportal");

		if(pVar && pVar->GetInt() > 0)
			m_nAdditionalFlags |= EDFL_AREAPORTAL;

		pVar = winding.pAssignedFace->pMaterial->FindMaterialVar("playerclip");

		if(pVar && pVar->GetInt() > 0)
			m_nAdditionalFlags |= EDFL_CLIP;

		pVar = winding.pAssignedFace->pMaterial->FindMaterialVar("npcclip");

		if(pVar && pVar->GetInt() > 0)
			m_nAdditionalFlags |= EDFL_CLIP;

		pVar = winding.pAssignedFace->pMaterial->FindMaterialVar("physicsclip");

		if(pVar && pVar->GetInt() > 0)
			m_nAdditionalFlags |= EDFL_CLIP;
		*/
	}

	DkList<lmodeldrawvertex_t> verts;

	for(int i = 0; i < m_polygons.numElem(); i++)
	{
		winding_t& winding = m_polygons[i];

		verts.append(winding.vertices);

		// HACK: add the first vertex to the last
		verts.append(winding.vertices[0]);
	}

	if(m_pVB)
	{
		eqlevelvertex_t* pData = NULL;

		if(m_pVB->Lock(0, verts.numElem(), (void**)&pData, false))
		{
			memcpy(pData,verts.ptr(),sizeof(eqlevelvertex_t)*verts.numElem());

			m_pVB->Unlock();
		}
	}
	else
	{
		m_pVB = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, verts.numElem(), sizeof(eqlevelvertex_t), verts.ptr());
	}
}

/*
// basic mesh modifications
void CEditableBrush::Translate(Vector3D &position)
{
	for(int i = 0; i < faces.numElem(); i++)
	{
		faces[i].Plane.offset -= dot(faces[i].Plane.normal, position);
	}

	UpdateRenderData();
}

void CEditableBrush::Scale(Vector3D &scale, bool use_center, Vector3D &scale_center)
{
	if(scale == vec3_zero)
		return;

	if(scale == Vector3D(1))
		return;

	Vector3D bbox_center = bbox.GetCenter();

	if(use_center)
		bbox_center = scale_center;

	bool invert = false;
	if(scale.x < 0)
		invert = !invert;
	if(scale.y < 0)
		invert = !invert;
	if(scale.z < 0)
		invert = !invert;

	for(int i = 0; i < polygons.numElem(); i++)
	{
		for(int j = 0; j < polygons[i].vertices.numElem(); j++)
		{
			polygons[i].vertices[j].position -= bbox_center;
			polygons[i].vertices[j].position *= scale;
			polygons[i].vertices[j].position += bbox_center;
		}

		// redefine planes
		faces[i].Plane = Plane(polygons[i].vertices[0].position, polygons[i].vertices[1].position, polygons[i].vertices[2].position, true);

		if(invert)
		{
			faces[i].Plane.normal = -faces[i].Plane.normal;
			faces[i].Plane.offset = -faces[i].Plane.offset;
		}
	}

	UpdateRenderData();
}

void CEditableBrush::Rotate(Vector3D &rotation_angles, bool use_center, Vector3D &rotation_center)
{
	Vector3D bbox_center = bbox.GetCenter();

	if(use_center)
		bbox_center = rotation_center;

	Matrix3x3 rotation = rotateXYZ3(DEG2RAD(rotation_angles.x),DEG2RAD(rotation_angles.y),DEG2RAD(rotation_angles.z));

	for(int i = 0; i < faces.numElem(); i++)
	{
		// move planes
		// TODO: texture lock variabled
		faces[i].Plane.offset += dot(faces[i].Plane.normal, bbox_center);
		faces[i].UAxis.offset += dot(faces[i].UAxis.normal, bbox_center);
		faces[i].VAxis.offset += dot(faces[i].VAxis.normal, bbox_center);

		// rotate surface normal
		faces[i].Plane.normal = rotation*faces[i].Plane.normal;

		// rotate texture axes
		faces[i].UAxis.normal = rotation*faces[i].UAxis.normal;
		faces[i].VAxis.normal = rotation*faces[i].VAxis.normal;

		// move planes back
		// TODO: texture lock variabled
		faces[i].Plane.offset -= dot(faces[i].Plane.normal, bbox_center);
		faces[i].UAxis.offset -= dot(faces[i].UAxis.normal, bbox_center);
		faces[i].VAxis.offset -= dot(faces[i].VAxis.normal, bbox_center);
	}

	UpdateRenderData();
}
*/
void CEditableBrush::OnRemove(bool bOnLevelCleanup)
{
	g_pShaderAPI->DestroyVertexBuffer(m_pVB);
	m_pVB = NULL;
}

float CEditableBrush::CheckLineIntersection(const Vector3D &start, const Vector3D &end, Vector3D &intersectionPos)
{
	bool isinstersects = false;

	Vector3D outintersection;

	float best_fraction = 1.0f;

	Vector3D dir = normalize(end - start);

	for (int i = 0; i < m_faces.numElem(); i++)
	{
		brushFace_t& face = m_faces[i];

		float frac = 1.0f;
		if(face.Plane.GetIntersectionWithRay(start, dir, outintersection))
		{
			frac = lineProjection(start, end, outintersection);

			if(frac < best_fraction && frac >= 0 && IsPointInside(outintersection + dir*0.1f))
			{
				best_fraction = frac;
				isinstersects = true;
				intersectionPos = outintersection;
			}
		}
	}

	return best_fraction;
}

bool CEditableBrush::IsPointInside(Vector3D &point)
{
	for (int i = 0; i < m_faces.numElem(); i++)
	{
		if(m_faces[i].Plane.ClassifyPoint(point) == CP_FRONT)
			return false;
	}

	return true;
}

bool CEditableBrush::IsPointInside_Epsilon(Vector3D &point, float eps)
{
	for (int i = 0; i < m_faces.numElem(); i++)
	{
		if(m_faces[i].Plane.Distance(point) > eps)
			return false;
	}

	return true;
}

// updates texturing
void CEditableBrush::UpdateSurfaceTextures()
{
	UpdateRenderBuffer();
}

struct ReadWriteFace_t
{
	Plane							NPlane;		// normal plane
	Plane							UAxis;		// tangent plane
	Plane							VAxis;		// binormal plane

	// texture scale
	Vector2D						vTexScale;

	// texture rotation
	float							fTexRotation;

	// face material
	char							materialname[256];
};

// saves this object
bool CEditableBrush::WriteObject(IVirtualStream* pStream)
{
	// write face count
	int num_faces = m_faces.numElem();
	pStream->Write(&num_faces, 1, sizeof(int));

	for(int i = 0; i < m_faces.numElem(); i++)
	{
		brushFace_t& face = m_faces[i];

		ReadWriteFace_t wface;
		memset(&face, 0,sizeof(ReadWriteFace_t));

		wface.NPlane = face.Plane;
		wface.UAxis = face.UAxis;
		wface.VAxis = face.VAxis;
		wface.vTexScale = face.vScale;
		wface.fTexRotation = face.fRotation;

		if(face.pMaterial)
		{
			strcpy(wface.materialname, face.pMaterial->GetName());
		}
		else
			strcpy(wface.materialname, "ERROR");

		pStream->Write(&face, 1, sizeof(ReadWriteFace_t));
	}

	return true;
}

// read this object
bool CEditableBrush::ReadObject(IVirtualStream* pStream)
{
	// write face count
	int num_faces;
	pStream->Read(&num_faces, 1, sizeof(int));

	for(int i = 0; i < num_faces; i++)
	{
		ReadWriteFace_t face;
		pStream->Read(&face, 1, sizeof(ReadWriteFace_t));

		brushFace_t addFace;

		addFace.Plane = face.NPlane;
		addFace.UAxis = face.UAxis;
		addFace.VAxis = face.VAxis;
		addFace.vScale = face.vTexScale;
		addFace.fRotation = face.fTexRotation;

		addFace.pMaterial = materials->GetMaterial(face.materialname);

		m_faces.append(addFace);
	}

	// calculate face verts
	UpdateRenderData();

	return true;
}
/*
// copies this object
CBaseEditableObject* CEditableBrush::CloneObject()
{
	CEditableBrush* pCloned = new CEditableBrush;

	for(int i = 0; i < faces.numElem(); i++)
	{
		Texture_t tex = faces[i];
		tex.nFlags &= ~STFL_SELECTED;

		pCloned->AddFace(tex);
	}

	pCloned->SetName(GetName());
	pCloned->bbox = bbox;

	pCloned->UpdateRenderData();

	return pCloned;
}
*/
void CEditableBrush::CutBrushByPlane(Plane &plane, CEditableBrush** ppNewBrush)
{
	CEditableBrush* pNewBrush = new CEditableBrush;
	
	for(int i = 0; i < m_faces.numElem(); i++)
	{
		for(int j = 0; j < m_polygons[i].vertices.numElem(); j++)
		{
			ClassifyPlane_e plClass = plane.ClassifyPoint(m_polygons[i].vertices[j].position);

			// only on back of plane
			if(plClass == CP_BACK)
			{
				pNewBrush->AddFace(m_faces[i]);
				break;
			}
		}
	}

	if(pNewBrush->GetFaceCount() == 0)
		return;

	{
		// add new faces
		brushFace_t face;

		// default some values
		face.fRotation = 0.0f;
		face.vScale = Vector2D(0.25f, 0.25f); // g_config.defaultTexScale

		// make the N plane from current iteration
		face.Plane = plane;

		// make the U and V texture axes
		VectorVectors(face.Plane.normal, face.UAxis.normal, face.VAxis.normal);
		face.UAxis.offset = 0;
		face.VAxis.offset = 0;

		// apply the currently selected material
		//face.pMaterial = g_editormainframe->GetMaterials()->GetSelectedMaterial();

		// append the face
		pNewBrush->AddFace(face);
	}

	pNewBrush->UpdateRenderData();
	*ppNewBrush = pNewBrush;
}

bool CEditableBrush::IsWindingFullyInsideBrush(winding_t* pWinding)
{
	int nInside = 0;

	for(int i = 0; i < pWinding->vertices.numElem(); i++)
		nInside += IsPointInside(pWinding->vertices[i].position);

	if(nInside == pWinding->vertices.numElem())
		return true;

	return false;
}

bool CEditableBrush::IsWindingIntersectsBrush(winding_t* pWinding)
{
	for(int i = 0; i < m_faces.numElem(); i++)
	{
		for(int j = 0; j < m_faces.numElem(); j++)
		{
			Vector3D out_point;

			if(pWinding->pAssignedFace->Plane.GetIntersectionWithPlanes(m_faces[i].Plane, m_faces[j].Plane, out_point))
			{
				for(int k = 0; k < m_faces.numElem(); k++)
				{
					if(m_faces[k].Plane.ClassifyPoint(out_point) == CP_FRONT)
						return false;
				}
			}
		}
	}

	return true;
}

bool CEditableBrush::IsTouchesBrush(winding_t* pWinding)
{
	// find one plane that touches another plane
	for(int i = 0; i < m_polygons.numElem(); i++)
	{
		if(pWinding->Classify(&m_polygons[i]) == CPL_ONPLANE)
			return true;
	}

	return false;
}

/*
void BuildWinding(EqBrushWinding_t* winding, level_build_data_t* pLevelBuildData)
{
	IMatVar* pNodraw = winding->pAssignedFace->pMaterial->FindMaterialVar("nodraw");
	IMatVar* pNoCollide = winding->pAssignedFace->pMaterial->FindMaterialVar("nocollide");

	bool nodraw = false;
	bool nocollide = false;

	if(pNodraw && pNodraw->GetInt() > 0)
		nodraw = true;

	if(pNoCollide && pNoCollide->GetInt() > 0)
		nocollide = true;

	if(nodraw && nocollide)
		return;

	// if winding has no verts, remove
	if(winding->vertices.numElem() == 0)
		return;

	BoundingBox bbox;

	int start_vert = pLevelBuildData->vertices.numElem();
	int start_indx = pLevelBuildData->indices.numElem();

	int phys_start_vert = pLevelBuildData->phys_vertices.numElem();
	int phys_start_indx = pLevelBuildData->phys_indices.numElem();

	int num_triangles = ((winding->vertices.numElem() < 4) ? 1 : (2 + winding->vertices.numElem() - 4));
	int num_indices = num_triangles*3;

	int material_id = pLevelBuildData->used_materials.findIndex(winding->pAssignedFace->pMaterial);

	if(material_id == -1)
		material_id = pLevelBuildData->used_materials.append(winding->pAssignedFace->pMaterial);

	if(!nodraw)
	{
		// add visual indices
		for(int j = 0; j < winding->vertices.numElem(); j++)
		{
			bbox.AddVertex(winding->vertices[j].position);
			pLevelBuildData->vertices.append(winding->vertices[j]);
		}

		for(int j = 0; j < num_triangles; j++)
		{
			int idx0 = 0;
			int idx1 = j + 1;
			int idx2 = j + 2;

			// add vertex indices
			pLevelBuildData->indices.append(start_vert+idx0);
			pLevelBuildData->indices.append(start_vert+idx1);
			pLevelBuildData->indices.append(start_vert+idx2);
		}

		// build surface data
		eqlevelsurf_t surf;

		surf.material_id = material_id;
		surf.flags = 0;
		surf.firstindex = start_indx;
		surf.firstvertex = start_vert;

		surf.numindices = num_indices;
		surf.numvertices = winding->vertices.numElem();

		surf.mins = bbox.minPoint;
		surf.maxs = bbox.maxPoint;

		// for first time it needs to be attached to root
		//surf.octreeid = 0;

		// for now occlusion wasn't built
		//surf.occdata_geom_id = -1;

		// add surface
		pLevelBuildData->surfaces.append(surf);
	}

	if(!nocollide)
	{
		// add physics vertices
		for(int j = 0; j < winding->vertices.numElem(); j++)
			pLevelBuildData->phys_vertices.append(winding->vertices[j]);

		for(int j = 0; j < num_triangles; j++)
		{
			int idx0 = 0;
			int idx1 = j + 1;
			int idx2 = j + 2;

			// add physics indices
			pLevelBuildData->phys_indices.append(phys_start_vert+idx0);
			pLevelBuildData->phys_indices.append(phys_start_vert+idx1);
			pLevelBuildData->phys_indices.append(phys_start_vert+idx2);
		}

		eqlevelphyssurf_t phys_surface;

		phys_surface.material_id = material_id;
		phys_surface.firstindex = phys_start_indx;
		phys_surface.firstvertex = phys_start_vert;

		phys_surface.numindices = num_indices;
		phys_surface.numvertices = winding->vertices.numElem();

		IMatVar* pSurfPropsVar = winding->pAssignedFace->pMaterial->FindMaterialVar("surfaceprops");

		if(pSurfPropsVar)
			strcpy(phys_surface.surfaceprops, pSurfPropsVar->GetString());
		else
			strcpy(phys_surface.surfaceprops, "default");

		// no occlusion for physics surfaces
//			phys_surface.occdata_geom_id = -1;

		// add surface
		pLevelBuildData->phys_surfaces.append(phys_surface);
	}
}
*/

// stores object in keyvalues
void CEditableBrush::SaveToKeyValues(kvkeybase_t* pSection)
{
	kvkeybase_t* pFacesSec = pSection->AddKeyBase("faces");
	
	for(int i = 0; i < m_faces.numElem(); i++)
	{
		brushFace_t& face = m_faces[i];

		kvkeybase_t* pThisFace = pFacesSec->AddKeyBase(varargs("f%d", i));
		pThisFace->AddKeyBase("nplane", varargs("%f %f %f %f", 
			face.Plane.normal.x, face.Plane.normal.y, face.Plane.normal.z,
			face.Plane.offset
			));

		pThisFace->AddKeyBase("uplane", varargs("%f %f %f %f", 
			face.UAxis.normal.x, face.UAxis.normal.y, face.UAxis.normal.z,
			face.UAxis.offset
			));

		pThisFace->AddKeyBase("vplane", varargs("%f %f %f %f", 
			face.VAxis.normal.x, face.VAxis.normal.y, face.VAxis.normal.z,
			face.VAxis.offset
			));

		pThisFace->AddKeyBase("texscale", varargs("%f %f", 
			face.vScale.x, face.vScale.y
			));

		pThisFace->AddKeyBase("rotation", varargs("%f", face.fRotation));
		pThisFace->AddKeyBase("material", (char*)face.pMaterial->GetName());
		pThisFace->AddKeyBase("smoothinggroup", varargs("%d", face.nSmoothingGroup));
		//pThisFace->AddKeyBase("nocollide", varargs("%d", face.nFlags & STFL_NOCOLLIDE));
		//pThisFace->AddKeyBase("nosubdivision", varargs("%d", face.nFlags & STFL_NOSUBDIVISION));
	}
}

// stores object in keyvalues
bool CEditableBrush::LoadFromKeyValues(kvkeybase_t* pSection)
{
	kvkeybase_t* pFacesSec = pSection->FindKeyBase("faces", KV_FLAG_SECTION);
	
	for(int i = 0; i < pFacesSec->keys.numElem(); i++)
	{
		kvkeybase_t* pThisFace = pFacesSec->keys[i];

		brushFace_t face;

		kvkeybase_t* pPair = pThisFace->FindKeyBase("nplane");

		Vector4D plane = KV_GetVector4D(pPair);

		face.Plane.normal = plane.xyz();
		face.Plane.offset = plane.w;

		pPair = pThisFace->FindKeyBase("uplane");
		plane = KV_GetVector4D(pPair);

		face.UAxis.normal = plane.xyz();
		face.UAxis.offset = plane.w;

		pPair = pThisFace->FindKeyBase("vplane");
		plane = KV_GetVector4D(pPair);

		face.VAxis.normal = plane.xyz();
		face.VAxis.offset = plane.w;

		pPair = pThisFace->FindKeyBase("texscale");
		face.vScale = KV_GetVector2D(pPair);

		pPair = pThisFace->FindKeyBase("rotation");
		face.fRotation = KV_GetValueFloat(pPair);

		pPair = pThisFace->FindKeyBase("material");
		face.pMaterial = materials->GetMaterial(KV_GetValueString(pPair));

		pPair = pThisFace->FindKeyBase("smoothinggroup");
		face.nSmoothingGroup = KV_GetValueInt(pPair);

		//pPair = pThisFace->FindKeyBase("nocollide");
		//face.nFlags |= KV_GetValueBool(pPair, false) ? STFL_NOCOLLIDE : 0;

		//pPair = pThisFace->FindKeyBase("nosubdivision");
		//face.nFlags |= KV_GetValueBool(pPair, false) ? STFL_NOSUBDIVISION : 0;

		// add face
		m_faces.append(face);
	}

	// calculate face verts
	UpdateRenderData();

	return true;
}

/*
// called when whole level builds
void CEditableBrush::BuildObject(level_build_data_t* pLevelBuildData)
{
	bool bMakeOccluder = false;
	bool bOccluderNotAllFaces = false;

	for(int i = 0; i < faces.numElem(); i++)
	{
		IMatVar* pOccluder = faces[i].pMaterial->FindMaterialVar("occlusion");
		if(pOccluder && pOccluder->GetInt() > 0)
		{
			// make occluder brush
			bMakeOccluder = true;
		}
		else
		{
			if(bMakeOccluder)
				bOccluderNotAllFaces = true;

			bMakeOccluder = false;
		}
	}

	if(bOccluderNotAllFaces)
	{
		MsgError("Warning: Brush (objectid=%d): not all brush face uses occluder material, forcing it to be occluder\n", m_id);
		bMakeOccluder = true;
	}

	if(bMakeOccluder)
	{
		int nFirstPlane = pLevelBuildData->occluderplanes.numElem();
		int nNumPlanes = faces.numElem();

		for(int i = 0; i < nNumPlanes; i++)
		{
			pLevelBuildData->occluderplanes.append(Vector4D(faces[i].Plane.normal, faces[i].Plane.offset));
		}

		eqlevelvolumeoccluder_t occluder;

		occluder.firstPlane = nFirstPlane;
		occluder.numPlanes = nNumPlanes;
		occluder.mins = bbox.minPoint;
		occluder.maxs = bbox.maxPoint;

		pLevelBuildData->occluders.append(occluder);

		// don't generate visual and physics part of this brush
		return;
	}

	
	// add without copying
	for(int i = 0; i < polygons.numElem(); i++)
		pLevelBuildData->brushwinding.append(&polygons[i]);

	//UpdateRenderBuffer();
}
*/

// creates a brush from volume, e.g a selection box
CEditableBrush* CreateBrushFromVolume(Volume *pVolume)
{
	CEditableBrush* pBrush = new CEditableBrush;

	// define a 6 planes
	for(int i = 0; i < 6; i++)
	{
		brushFace_t face;

		// default some values
		face.fRotation = 0.0f;
		face.vScale = Vector2D(0.25f, 0.25f); // g_config.defaultTexScale

		// make the N plane from current iteration
		face.Plane = pVolume->GetPlane(i);

		// make the U and V texture axes
		VectorVectors(face.Plane.normal, face.UAxis.normal, face.VAxis.normal);
		face.UAxis.offset = 0;
		face.VAxis.offset = 0;

		// apply the currently selected material
		//face.pMaterial = g_editormainframe->GetMaterials()->GetSelectedMaterial();

		// append the face
		pBrush->AddFace(face);
	}

	// calculates face verts
	pBrush->UpdateRenderData();

	return pBrush;
}