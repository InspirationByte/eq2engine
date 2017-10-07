//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Convex polyhedra, for engine/editor/compiler
//////////////////////////////////////////////////////////////////////////////////

#include "EqBrush.h"
#include "EditorHeader.h"
#include "IdebugOverlay.h"
#include "SurfaceDialog.h"
#include "EditableSurface.h"
#include "coord.h"
#include "MaterialListFrame.h"
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

void ChopWinding(EqBrushWinding_t &w, Plane &plane)
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
		eqlevelvertex_t vertex(newVertices[i], vec2_zero, vec3_zero);
		vertex.vertexPaint = color4_white;

		w.vertices.append(vertex);
	}
}

/**/

void MakeInfiniteWinding(EqBrushWinding_t &w, Plane &plane)
{
	// compute needed vectors
	Vector3D vRight, vUp;
	VectorVectors(plane.normal, vRight, vUp);

	// make vectors infinite
	vRight *= MAX_COORD_UNITS;
	vUp *= MAX_COORD_UNITS;

	Vector3D org = -plane.normal * plane.offset;

	// and finally fill really big winding
	w.vertices.append(eqlevelvertex_t(org - vRight + vUp, vec2_zero, plane.normal));
	w.vertices.append(eqlevelvertex_t(org + vRight + vUp, vec2_zero, plane.normal));
	w.vertices.append(eqlevelvertex_t(org + vRight - vUp, vec2_zero, plane.normal));
	w.vertices.append(eqlevelvertex_t(org - vRight - vUp, vec2_zero, plane.normal));
}

//-----------------------------------------
// Brush face member functions
//-----------------------------------------

// calculates the texture coordinates for this 
void EqBrushWinding_t::CalculateTextureCoordinates()
{
	int texSizeW = 32;
	int texSizeH = 32;

	pAssignedFace->pMaterial->GetShader()->InitParams();

	ITexture* pTex = pAssignedFace->pMaterial->GetShader()->GetBaseTexture();

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

		vertices[i].tangent = vec3_zero;
		vertices[i].binormal = vec3_zero;
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

		vertices[idx0].tangent += t;
		vertices[idx0].binormal += b;
		vertices[idx0].normal += n;

		vertices[idx1].tangent += t;
		vertices[idx1].binormal += b;
		vertices[idx1].normal += n;

		vertices[idx2].tangent += t;
		vertices[idx2].binormal += b;
		vertices[idx2].normal += n;
	}

	for(int i = 0; i < vertices.numElem(); i++)
	{
		vertices[i].tangent = normalize(vertices[i].tangent);
		vertices[i].binormal = normalize(vertices[i].binormal);
		vertices[i].normal = normalize(vertices[i].normal);

		//debugoverlay->Line3D(vertices[i].position, vertices[i].position + vertices[i].tangent*10,ColorRGBA(1,0,0,1),ColorRGBA(1,0,0,1),10);
		//debugoverlay->Line3D(vertices[i].position, vertices[i].position + vertices[i].binormal*10,ColorRGBA(0,1,0,1),ColorRGBA(0,1,0,1),10);
		//debugoverlay->Line3D(vertices[i].position, vertices[i].position + vertices[i].normal*10,ColorRGBA(0,0,1,1),ColorRGBA(0,0,1,1),10);
	}
}

// sorts the vertices, makes them as triangle list
bool EqBrushWinding_t::SortVerticesAsTriangleList()
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

		eqlevelvertex_t	vtx	= vertices[nSmallestIdx];
		vertices[ nSmallestIdx ] = vertices[i+1];
		vertices[i+1] = vtx;
	}

	return true;
}

// splits the face by this face, and results a
void EqBrushWinding_t::Split(const EqBrushWinding_t *w, EqBrushWinding_t *front, EqBrushWinding_t *back )
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
			eqlevelvertex_t	v;	// New vertex created by splitting
			float p;			// Percentage between the two points

			pAssignedFace->Plane.GetIntersectionLineFraction(w->vertices[i].position, w->vertices[iNext].position, v.position, p );

			v.texcoord = lerp(w->vertices[i].texcoord, w->vertices[iNext].texcoord, p);

			front->vertices.append(v);
			back->vertices.append(v);
		}
	}
}

// classifies the next face over this
ClassifyPoly_e	EqBrushWinding_t::Classify(EqBrushWinding_t *w)
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

// returns current object's flags
int	CEditableBrush::GetFlags()
{
	return CBaseEditableObject::GetFlags() | m_nAdditionalFlags;
}

// calculates bounding box for this brush
void CEditableBrush::CalculateBBOX()
{
	bbox.Reset();

	for(int i = 0; i < polygons.numElem(); i++)
	{
		for(int j = 0; j < polygons[i].vertices.numElem(); j++)
			bbox.AddVertex(polygons[i].vertices[j].position);
	}
}

// is brush aabb intersects another brush aabb
bool CEditableBrush::IsBrushIntersectsAABB(CEditableBrush *pBrush)
{
	BoundingBox another_box(pBrush->GetBBoxMins(),pBrush->GetBBoxMaxs());

	return bbox.Intersects(another_box);
}

void CEditableBrush::SortVertsToDraw()
{
	for(int i = 0; i < polygons.numElem(); i++)
	{
		if(!polygons[i].SortVerticesAsTriangleList())
		{
			Msg("\nERROR: Degenerate plane %d on brush (objectid=%d)\n", i, m_id);
		}
	}
}

// calculates the vertices from faces
bool CEditableBrush::CreateFromPlanes()
{
	// check planes first
	bool usePlane[MAX_BRUSH_PLANES];
	memset(usePlane,0,sizeof(usePlane));

	for ( int i = 0; i < faces.numElem(); i++ )
	{
		if(faces[i].Plane.normal == vec3_zero)
		{
			Msg("Brush (objectid=%d): Invalid plane %d\n", m_id, i);
			usePlane[i] = false;
			continue;
		}

		usePlane[i] = true;
		for (int j = 0; j < i; j++)
		{
			Texture_t* faceCheck = GetFace(j);

			// check duplicate planes
			if((dot(faces[i].Plane.normal, faceCheck->Plane.normal) > 0.999) && (fabs(faces[i].Plane.offset - faceCheck->Plane.offset) < 0.01))
			{
				Msg("Brush (objectid=%d): Duplicate plane found %d\n", m_id ,j);
				usePlane[j] = false;
			}
		}
	}
	
	polygons.clear();

	for ( int i = 0; i < faces.numElem(); i++ )
	{
		if(!usePlane[i])
		{
			faces.removeIndex(i);
			i--;
			continue;
		}

		EqBrushWinding_t poly;
		poly.pAssignedBrush = this;

		MakeInfiniteWinding(poly, faces[i].Plane);

		for ( int j = 0; j < faces.numElem(); j++ )
		{
			if(j != i)
			{
				Plane clipPlane;
				clipPlane.normal = -faces[j].Plane.normal;
				clipPlane.offset = -faces[j].Plane.offset;

				ChopWinding(poly, clipPlane);
			}
		}

		poly.pAssignedFace = &faces[i];
		polygons.append(poly);
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
	for(int i = 0; i < polygons.numElem(); i++)
	{
		if(polygons[i].vertices.numElem() < 3)
		{
			polygons.removeIndex(i);
			faces.removeIndex(i);
			i--;
		}
	}
};

// draw brush
void CEditableBrush::Render(int nViewRenderFlags)
{
	materials->SetCullMode(CULL_BACK);
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	g_pShaderAPI->SetVertexFormat(g_pLevel->GetLevelVertexFormat());
	g_pShaderAPI->SetVertexBuffer(m_pVB,0);
	g_pShaderAPI->SetIndexBuffer(NULL);

	Matrix4x4 view;
	materials->GetMatrix(MATRIXMODE_VIEW, view);

	ColorRGBA ambientColor = color4_white;

	int nFirstVertex = 0;
	for(int i = 0; i < polygons.numElem(); i++)
	{
		// cancel if transparents disabled
		if((nViewRenderFlags & VR_FLAG_NO_TRANSLUCENT) && (polygons[i].pAssignedFace->pMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT))
		{
			nFirstVertex += polygons[i].vertices.numElem()+1;
			continue;
		}
		
		// cancel if opaques disabled
		if((nViewRenderFlags & VR_FLAG_NO_OPAQUE) && !(polygons[i].pAssignedFace->pMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT))
		{
			nFirstVertex += polygons[i].vertices.numElem()+1;
			continue;
		}

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
		materials->BindMaterial(polygons[i].pAssignedFace->pMaterial, false);

		materials->Apply();

		g_pShaderAPI->DrawNonIndexedPrimitives(PRIM_TRIANGLE_FAN, nFirstVertex, polygons[i].vertices.numElem());

		if((polygons[i].pAssignedFace->nFlags & STFL_SELECTED) && !g_editormainframe->GetSurfaceDialog()->IsSelectionMaskDisabled())
		{
			ColorRGBA sel_color(1.0f,0.0f,0.0f,0.35f);
			materials->SetAmbientColor(sel_color);

			materials->BindMaterial(g_pLevel->GetFlatMaterial(), false);
			materials->SetDepthStates(true, false);

			materials->Apply();
			g_pShaderAPI->DrawNonIndexedPrimitives(PRIM_TRIANGLE_FAN, nFirstVertex, polygons[i].vertices.numElem());
		}

		nFirstVertex += polygons[i].vertices.numElem()+1;
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

void CEditableBrush::AddFace(Texture_t &face)
{
	faces.append(face);
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

	for(int i = 0; i < polygons.numElem(); i++)
	{
		polygons[i].CalculateTextureCoordinates();

		IMatVar* pVar = polygons[i].pAssignedFace->pMaterial->FindMaterialVar("roomfiller");

		if(pVar && pVar->GetInt() > 0)
			m_nAdditionalFlags |= EDFL_ROOM;

		pVar = polygons[i].pAssignedFace->pMaterial->FindMaterialVar("areaportal");

		if(pVar && pVar->GetInt() > 0)
			m_nAdditionalFlags |= EDFL_AREAPORTAL;

		pVar = polygons[i].pAssignedFace->pMaterial->FindMaterialVar("playerclip");

		if(pVar && pVar->GetInt() > 0)
			m_nAdditionalFlags |= EDFL_CLIP;

		pVar = polygons[i].pAssignedFace->pMaterial->FindMaterialVar("npcclip");

		if(pVar && pVar->GetInt() > 0)
			m_nAdditionalFlags |= EDFL_CLIP;

		pVar = polygons[i].pAssignedFace->pMaterial->FindMaterialVar("physicsclip");

		if(pVar && pVar->GetInt() > 0)
			m_nAdditionalFlags |= EDFL_CLIP;
	}

	DkList<eqlevelvertex_t> verts;

	for(int i = 0; i < polygons.numElem(); i++)
	{
		verts.append(polygons[i].vertices);

		// HACK: add the first vertex to the last
		verts.append(polygons[i].vertices[0]);
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

void CEditableBrush::RenderGhost(Vector3D &addpos, Vector3D &addscale, Vector3D &addrot, bool use_center, Vector3D &center)
{
	Vector3D object_center = (GetBBoxMins() + GetBBoxMaxs())*0.5f;

	Vector3D bbox_center = (GetBBoxMins() + GetBBoxMaxs())*0.5f;

	if(use_center)
		bbox_center += center;

	/*
	Matrix4x4 rotationmat = translate(bbox_center) * (rotateXYZ(DEG2RAD(addrot.x),DEG2RAD(addrot.y),DEG2RAD(addrot.z))*translate(-bbox_center));
	Matrix4x4 scalemat = translate(bbox_center) * (scale(addscale.x,addscale.y,addscale.z)*translate(-bbox_center));
	*/

	Matrix4x4 rotationmat = rotateXYZ4(DEG2RAD(addrot.x),DEG2RAD(addrot.y),DEG2RAD(addrot.z));
	Matrix4x4 scalemat = (scale4(addscale.x,addscale.y,addscale.z));

	Matrix4x4 rendermatrix = (translate(bbox_center)*(scalemat*rotationmat)*translate(-bbox_center))*translate(addpos);

	materials->SetMatrix(MATRIXMODE_WORLD, /*rotationmat*scalemat*translate(addpos)*/rendermatrix);

	g_pShaderAPI->SetVertexFormat(g_pLevel->GetLevelVertexFormat());
	g_pShaderAPI->SetVertexBuffer(m_pVB,0);
	g_pShaderAPI->SetIndexBuffer(NULL);

	materials->BindMaterial(g_pLevel->GetFlatMaterial());

	int nFirstVertex = 0;
	for(int i = 0; i < polygons.numElem(); i++)
	{
		g_pShaderAPI->DrawNonIndexedPrimitives(PRIM_LINE_STRIP, nFirstVertex, polygons[i].vertices.numElem()+1);

		nFirstVertex += polygons[i].vertices.numElem()+1;
	}
}

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
		faces[i].Plane = Plane(polygons[i].vertices[0].position, polygons[i].vertices[1].position, polygons[i].vertices[2].position);

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

	for (int i = 0; i < faces.numElem(); i++)
	{
		float frac = 1.0f;
		if(faces[i].Plane.GetIntersectionWithRay(start, dir, outintersection))
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
	for (int i = 0; i < faces.numElem(); i++)
	{
		if(faces[i].Plane.ClassifyPoint(point) == CP_FRONT)
			return false;
	}

	return true;
}

bool CEditableBrush::IsPointInside_Epsilon(Vector3D &point, float eps)
{
	for (int i = 0; i < faces.numElem(); i++)
	{
		if(faces[i].Plane.Distance(point) > eps)
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
	int num_faces = faces.numElem();
	pStream->Write(&num_faces, 1, sizeof(int));

	for(int i = 0; i < faces.numElem(); i++)
	{
		ReadWriteFace_t face;
		memset(&face, 0,sizeof(ReadWriteFace_t));

		face.NPlane = faces[i].Plane;
		face.UAxis = faces[i].UAxis;
		face.VAxis = faces[i].VAxis;
		face.vTexScale = faces[i].vScale;
		face.fTexRotation = faces[i].fRotation;

		if(faces[i].pMaterial)
		{
			strcpy(face.materialname, faces[i].pMaterial->GetName());
		}
		else
			strcpy(face.materialname, "ERROR");

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

		Texture_t addFace;

		addFace.Plane = face.NPlane;
		addFace.UAxis = face.UAxis;
		addFace.VAxis = face.VAxis;
		addFace.vScale = face.vTexScale;
		addFace.fRotation = face.fTexRotation;

		addFace.pMaterial = materials->GetMaterial(face.materialname, true);

		faces.append(addFace);
	}

	// calculate face verts
	UpdateRenderData();

	return true;
}

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

void CEditableBrush::CutBrushByPlane(Plane &plane, CEditableBrush** ppNewBrush)
{
	CEditableBrush* pNewBrush = new CEditableBrush;
	
	for(int i = 0; i < faces.numElem(); i++)
	{
		for(int j = 0; j < polygons[i].vertices.numElem(); j++)
		{
			ClassifyPlane_e plClass = plane.ClassifyPoint(polygons[i].vertices[j].position);

			// only on back of plane
			if(plClass == CP_BACK)
			{
				pNewBrush->AddFace(faces[i]);
				break;
			}
		}
	}

	if(pNewBrush->GetFaceCount() == 0)
		return;

	{
		// add new faces
		Texture_t face;

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
		face.pMaterial = g_editormainframe->GetMaterials()->GetSelectedMaterial();

		// append the face
		pNewBrush->AddFace(face);
	}

	pNewBrush->UpdateRenderData();
	*ppNewBrush = pNewBrush;
}

bool CEditableBrush::IsWindingFullyInsideBrush(EqBrushWinding_t* pWinding)
{
	int nInside = 0;

	for(int i = 0; i < pWinding->vertices.numElem(); i++)
		nInside += IsPointInside(pWinding->vertices[i].position);

	if(nInside == pWinding->vertices.numElem())
		return true;

	return false;
}

bool CEditableBrush::IsWindingIntersectsBrush(EqBrushWinding_t* pWinding)
{
	for(int i = 0; i < faces.numElem(); i++)
	{
		for(int j = 0; j < faces.numElem(); j++)
		{
			Vector3D out_point;

			if(pWinding->pAssignedFace->Plane.GetIntersectionWithPlanes(faces[i].Plane, faces[j].Plane, out_point))
			{
				for(int k = 0; k < faces.numElem(); k++)
				{
					if(faces[k].Plane.ClassifyPoint(out_point) == CP_FRONT)
						return false;
				}
			}
		}
	}

	return true;
}

bool CEditableBrush::IsTouchesBrush(EqBrushWinding_t* pWinding)
{
	// find one plane that touches another plane
	for(int i = 0; i < polygons.numElem(); i++)
	{
		if(pWinding->Classify(&polygons[i]) == CPL_ONPLANE)
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
	
	for(int i = 0; i < faces.numElem(); i++)
	{
		kvkeybase_t* pThisFace = pFacesSec->AddKeyBase(varargs("f%d", i));
		pThisFace->AddKeyBase("nplane", varargs("%f %f %f %f", 
			faces[i].Plane.normal.x,faces[i].Plane.normal.y,faces[i].Plane.normal.z,
			faces[i].Plane.offset
			));

		pThisFace->AddKeyBase("uplane", varargs("%f %f %f %f", 
			faces[i].UAxis.normal.x,faces[i].UAxis.normal.y,faces[i].UAxis.normal.z,
			faces[i].UAxis.offset
			));

		pThisFace->AddKeyBase("vplane", varargs("%f %f %f %f", 
			faces[i].VAxis.normal.x,faces[i].VAxis.normal.y,faces[i].VAxis.normal.z,
			faces[i].VAxis.offset
			));

		pThisFace->AddKeyBase("texscale", varargs("%f %f", 
			faces[i].vScale.x,faces[i].vScale.y
			));

		pThisFace->AddKeyBase("rotation", varargs("%f", faces[i].fRotation));
		pThisFace->AddKeyBase("material", (char*)faces[i].pMaterial->GetName());
		pThisFace->AddKeyBase("smoothinggroup", varargs("%d", faces[i].nSmoothingGroup));
		pThisFace->AddKeyBase("nocollide", varargs("%d", faces[i].nFlags & STFL_NOCOLLIDE));
		pThisFace->AddKeyBase("nosubdivision", varargs("%d", faces[i].nFlags & STFL_NOSUBDIVISION));
	}
}

// stores object in keyvalues
bool CEditableBrush::LoadFromKeyValues(kvkeybase_t* pSection)
{
	kvkeybase_t* pFacesSec = pSection->FindKeyBase("faces", KV_FLAG_SECTION);
	
	for(int i = 0; i < pFacesSec->keys.numElem(); i++)
	{
		kvkeybase_t* pThisFace = pFacesSec->keys[i];

		Texture_t face;

		kvkeybase_t* pPair = pThisFace->FindKeyBase("nplane");

		Vector4D plane = UTIL_StringToColor4(pPair->values[0]);

		face.Plane.normal = plane.xyz();
		face.Plane.offset = plane.w;

		pPair = pThisFace->FindKeyBase("uplane");
		plane = UTIL_StringToColor4(pPair->values[0]);

		face.UAxis.normal = plane.xyz();
		face.UAxis.offset = plane.w;

		pPair = pThisFace->FindKeyBase("vplane");
		plane = UTIL_StringToColor4(pPair->values[0]);

		face.VAxis.normal = plane.xyz();
		face.VAxis.offset = plane.w;

		pPair = pThisFace->FindKeyBase("texscale");
		face.vScale = UTIL_StringToVector2(pPair->values[0]);

		pPair = pThisFace->FindKeyBase("rotation");
		face.fRotation = atof(pPair->values[0]);

		pPair = pThisFace->FindKeyBase("material");
		face.pMaterial = materials->GetMaterial(pPair->values[0], true);

		pPair = pThisFace->FindKeyBase("smoothinggroup");
		face.nSmoothingGroup = atoi(pPair->values[0]);

		pPair = pThisFace->FindKeyBase("nocollide");
		face.nFlags |= KV_GetValueBool(pPair, false) ? STFL_NOCOLLIDE : 0;

		pPair = pThisFace->FindKeyBase("nosubdivision");
		face.nFlags |= KV_GetValueBool(pPair, false) ? STFL_NOSUBDIVISION : 0;

		// add face
		faces.append(face);
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
		Texture_t face;

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
		face.pMaterial = g_editormainframe->GetMaterials()->GetSelectedMaterial();

		// append the face
		pBrush->AddFace(face);
	}

	// calculates face verts
	pBrush->UpdateRenderData();

	return pBrush;
}