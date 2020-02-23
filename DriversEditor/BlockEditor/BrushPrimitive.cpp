//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Convex polyhedra, for engine/editor/compiler
//////////////////////////////////////////////////////////////////////////////////

#include "BrushPrimitive.h"
#include "materialsystem/MeshBuilder.h"
#include <malloc.h>

#include "world.h"

#include "IDebugOverlay.h"
#include "coord.h"
#include "Utils/GeomTools.h"

//-----------------------------------------
// Brush face member functions
//-----------------------------------------

// calculates the texture coordinates for this 
void winding_t::CalculateTextureCoordinates(lmodeldrawvertex_t* verts, int vertCount)
{
	int texSizeW = 32;
	int texSizeH = 32;

	ITexture* pTex = face.pMaterial->GetBaseTexture();

	if(pTex)
	{
		texSizeW = pTex->GetWidth();
		texSizeH = pTex->GetHeight();
	}

	const float texelW = 1.0f / texSizeW;
	const float texelH = 1.0f / texSizeH;

	Vector3D axisAngles = VectorAngles(face.Plane.normal);
	AngleVectors(axisAngles, NULL, &face.UAxis.normal, &face.VAxis.normal);

	face.VAxis.normal *= -1;
	//VectorVectors(face.Plane.normal, face.UAxis.normal, face.VAxis.normal);

	Matrix3x3 texMatrix(face.UAxis.normal, face.VAxis.normal, face.Plane.normal);

	Matrix3x3 rotationMat = rotateZXY3( 0.0f, 0.0f, DEG2RAD(face.fRotation));
	texMatrix = rotationMat*texMatrix;

	face.UAxis.normal = -texMatrix.rows[0];
	face.VAxis.normal = texMatrix.rows[1];
	
	for(int i = 0; i < vertCount; i++ )
	{
		float U, V;
		
		U = dot(face.UAxis.normal, verts[i].position);
		U /= ( ( float )texSizeW ) * face.vScale.x * texelW;
		U += face.UAxis.offset * texelW;

		V = dot(face.VAxis.normal, verts[i].position);
		V /= ( ( float )texSizeH ) * face.vScale.y * texelH;
		V += face.VAxis.offset * texelH;

		verts[i].texcoord.x = U;
		verts[i].texcoord.y = V;

		//verts[i].tangent = vec3_zero;
		//verts[i].binormal = vec3_zero;
		verts[i].normal = vec3_zero;
	}

	int num_triangles = ((vertCount < 4) ? 1 : (2 + vertCount - 4));

	for(int i = 0; i < num_triangles; i++ )
	{
		int idx0 = 0;
		int idx1 = i+1;
		int idx2 = i+2;

		Vector3D t,b,n;
		ComputeTriangleTBN( verts[idx0].position, verts[idx1].position, verts[idx2].position,
							verts[idx0].texcoord, verts[idx1].texcoord, verts[idx2].texcoord,
							n,t,b);

		//verts[idx0].tangent += t;
		//verts[idx0].binormal += b;
		verts[idx0].normal += n;

		//verts[idx1].tangent += t;
		//verts[idx1].binormal += b;
		verts[idx1].normal += n;

		//verts[idx2].tangent += t;
		//verts[idx2].binormal += b;
		verts[idx2].normal += n;
	}

	for(int i = 0; i < vertCount; i++)
	{
		//verts[i].tangent = normalize(verts[i].tangent);
		//verts[i].binormal = normalize(verts[i].binormal);
		verts[i].normal = normalize(verts[i].normal);
	}
}

// sorts the drawVerts, makes them as triangle list
bool winding_t::SortIndices()
{
	if(vertex_ids.numElem() < 3)
	{
		MsgError("ERROR: Polygon has less than 3 vertices (%d)\n", vertex_ids.numElem());
		return true;
	}

	const DkList<Vector3D>& brushVerts = brush->m_verts;

	// get the center of that poly
	float countFac = 1.0f / (float)vertex_ids.numElem();

	Vector3D center(0);
	for ( int i = 0; i < vertex_ids.numElem(); i++ )
		center += brushVerts[vertex_ids[i]] * countFac;

	// sort them now
	for ( int i = 0; i < vertex_ids.numElem() -2; i++ )
	{
		Vector3D iVert = brushVerts[vertex_ids[i]];

		float	fSmallestAngle	= -1;
		int		nSmallestIdx	= -1;

		Vector3D a = normalize(iVert - center);
		Plane p(iVert, center, center + face.Plane.normal, true );

		for(int j = i+1; j < vertex_ids.numElem(); j++)
		{
			Vector3D jVert = brushVerts[vertex_ids[j]];

			// the vertex is in front or lies on the plane
			if (p.ClassifyPoint(jVert) != CP_BACK)
			{
				Vector3D b = normalize(jVert - center);

				float fAngle = dot(b,a);

				if ( fAngle > fSmallestAngle )
				{
					fSmallestAngle	= fAngle;
					nSmallestIdx	= j;
				}
			}
		}

		if (nSmallestIdx == -1)
			return false;

		// swap the indices
		QuickSwap(vertex_ids[nSmallestIdx], vertex_ids[i + 1]);
	}

	return true;
}

Vector3D winding_t::GetCenter() const
{
	if (!vertex_ids.numElem())
	{
		// use brush center instead (not likely to be executed)
		Vector3D center = brush->GetBBox().GetCenter();
		return center - face.Plane.normal * face.Plane.Distance(center);
	}

	const DkList<Vector3D>& verts = brush->m_verts;

	const float invDiv = 1.0f / (float)vertex_ids.numElem();

	Vector3D vec(0.0f);
	for (int i = 0; i < vertex_ids.numElem(); i++)
		vec += verts[vertex_ids[i]] * invDiv;

	return vec;
}

int winding_t::CheckRayIntersectionWithVertex(const Vector3D &start, const Vector3D &dir, float vertexScale)
{
	float nearestVertDist = MAX_COORD_UNITS;
	int nearestVert = -1;

	const DkList<Vector3D>& verts = brush->m_verts;

	for (int i = 0; i < vertex_ids.numElem(); i++)
	{
		Vector3D vert = verts[vertex_ids[i]];

		Plane pl(dir, -dot(dir, vert));

		Vector3D rayPlaneOut;
		if (pl.GetIntersectionWithRay(start, dir, rayPlaneOut))
		{
			float dist = length(vert - rayPlaneOut);
			if (dist < nearestVertDist && dist <= vertexScale)
			{
				nearestVert = vertex_ids[i];
				nearestVertDist = dist;
			}
		}
	}

	return nearestVert;
}

// classifies the next face over this
ClassifyPoly_e winding_t::Classify(winding_t *w) const
{
	bool	bFront = false, bBack = false;
	float	dist;

	const DkList<Vector3D>& wVerts = w->brush->m_verts;

	for ( int i = 0; i < w->vertex_ids.numElem(); i++ )
	{
		dist = face.Plane.Distance(wVerts[w->vertex_ids[i]]);

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

void winding_t::Transform(const Matrix4x4& mat)
{
	Vector4D O(face.Plane.normal * -face.Plane.offset, 1.0f);
	Vector4D N(face.Plane.normal, 0.0f);

	O = mat * O;				// transform point
	N = transpose(!mat) * N;	// transform (rotate) the normal

	// reinitialize the plane
	face.Plane = Plane(N.xyz(), -dot(O.xyz(), N.xyz()), true);
}

/*
// makes editable surface from this winding (and no way back)
CEditableSurface* EqBrushWinding_t::MakeEditableSurface()
{
	CEditableSurface* pSurface = new CEditableSurface;

	DkList<int>	indices;

	int num_triangles = ((drawVerts.numElem() < 4) ? 1 : (2 + drawVerts.numElem() - 4));
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

	pSurface->MakeCustomMesh(drawVerts.ptr(), indices.ptr(), drawVerts.numElem(), indices.numElem());
	pSurface->SetMaterial(face.pMaterial);

	return pSurface;
}
*/
//-----------------------------------------
// Brush member functions
//-----------------------------------------

CBrushPrimitive::CBrushPrimitive()
{
	m_pVB = NULL;
}

CBrushPrimitive::~CBrushPrimitive()
{
	OnRemove();
}

// calculates the vertices from faces
bool CBrushPrimitive::AssignWindingVertices()
{
	m_verts.clear();
	m_bbox.Reset();

	CalculateVerts(m_verts);

	// add new vertices to BBOX
	for (int i = 0; i < m_verts.numElem(); i++)
		m_bbox.AddVertex(m_verts[i]);

	// clear vertex ids as we're recalculating
	for (int i = 0; i < m_windingFaces.numElem(); i++)
	{
		winding_t& winding = m_windingFaces[i];
		winding.vertex_ids.clear();
	}

	int numVerts = 0;

	// add vertex indices
	// as it's almost the same as GetBrushVerts works, it's safe to use same indexes
	for (int i = 0; i < m_windingFaces.numElem(); i++)
	{
		winding_t& iWinding = m_windingFaces[i];

		const Plane& iPlane = iWinding.face.Plane;

		for (int j = i + 1; j < m_windingFaces.numElem(); j++)
		{
			winding_t& jWinding = m_windingFaces[j];
			const Plane& jPlane = jWinding.face.Plane;

			for (int k = j + 1; k < m_windingFaces.numElem(); k++)
			{
				winding_t& kWinding = m_windingFaces[k];
				const Plane& kPlane = kWinding.face.Plane;

				// calculate vertices by intersection of three planes
				Vector3D point;
				if (iPlane.GetIntersectionWithPlanes(jPlane, kPlane, point) && IsPointInside(point))
				{
					// those verts can be recalculated; base algorithm is unchanged
					// add only indices
					int vertex_id = numVerts++;
					iWinding.vertex_ids.append(vertex_id);
					jWinding.vertex_ids.append(vertex_id);
					kWinding.vertex_ids.append(vertex_id);
				}
			}
		}
	}

	// remove empty faces and sort indices
	ValidateWindings();

	return (m_windingFaces.numElem() > 0);
}

void CBrushPrimitive::ValidateWindings()
{
	// remove empty faces
	for(int i = 0; i < m_windingFaces.numElem(); i++)
	{
		if(m_windingFaces[i].vertex_ids.numElem() < 3)
		{
			m_windingFaces.fastRemoveIndex(i);
			i--;
			continue;
		}

		m_windingFaces[i].SortIndices();
	}
};

// draw brush
void CBrushPrimitive::Render(int nViewRenderFlags)
{
	materials->SetCullMode(CULL_BACK);
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	g_pShaderAPI->SetVertexFormat(g_worldGlobals.levelObjectVF);
	g_pShaderAPI->SetVertexBuffer(m_pVB,0);
	g_pShaderAPI->SetIndexBuffer(NULL);

	Matrix4x4 view;
	materials->GetMatrix(MATRIXMODE_VIEW, view);

	ColorRGBA ambientColor = materials->GetAmbientColor();

	int nFirstVertex = 0;
	for(int i = 0; i < m_windingFaces.numElem(); i++)
	{
		winding_t& winding = m_windingFaces[i];
		int numVerts = winding.vertex_ids.numElem();

		if(winding.face.nFlags & BRUSH_FACE_SELECTED)
		{
			ColorRGBA sel_color(1.0f,0.5f,0.5f,1.0f);
			materials->SetAmbientColor(sel_color);
		}
		else
			materials->SetAmbientColor(ambientColor);

		// apply the material (slow in editor)
		materials->BindMaterial(winding.face.pMaterial, 0);
		materials->Apply();

		g_pShaderAPI->DrawNonIndexedPrimitives(PRIM_TRIANGLE_FAN, nFirstVertex, numVerts);

		nFirstVertex += numVerts + 1;
	}
}

void CBrushPrimitive::RenderGhost(int face /*= -1*/)
{
	materials->SetCullMode(CULL_BACK);

	g_pShaderAPI->SetVertexFormat(g_worldGlobals.levelObjectVF);
	g_pShaderAPI->SetVertexBuffer(m_pVB, 0);
	g_pShaderAPI->SetIndexBuffer(NULL);

	materials->SetAmbientColor(color4_white);
	materials->BindMaterial(materials->GetDefaultMaterial());
	g_pShaderAPI->SetTexture(nullptr, nullptr, 0);

	int nFirstVertex = 0;
	for (int i = 0; i < m_windingFaces.numElem(); i++)
	{
		int numVerts = m_windingFaces[i].vertex_ids.numElem() + 1;

		if(face == -1 || face == i)
			g_pShaderAPI->DrawNonIndexedPrimitives(PRIM_LINE_STRIP, nFirstVertex, numVerts);

		nFirstVertex += numVerts;
	}
}

void CBrushPrimitive::RenderGhostCustom(const DkList<Vector3D>& verts, int face /*= -1*/)
{
	materials->SetCullMode(CULL_BACK);

	//g_pShaderAPI->SetVertexFormat(g_worldGlobals.levelObjectVF);
	//g_pShaderAPI->SetVertexBuffer(m_pVB, 0);
	//g_pShaderAPI->SetIndexBuffer(NULL);

	materials->SetAmbientColor(color4_white);
	materials->BindMaterial(materials->GetDefaultMaterial());
	g_pShaderAPI->SetTexture(nullptr, nullptr, 0);

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	for (int i = 0; i < m_windingFaces.numElem(); i++)
	{
		winding_t& winding = m_windingFaces[i];
		int numVerts = winding.vertex_ids.numElem();

		if (face == -1 || face == i)
		{
			meshBuilder.Begin(PRIM_LINE_STRIP);
			for (int j = 0; j < numVerts+1; j++)
			{
				meshBuilder.Position3fv(verts[winding.vertex_ids[j % numVerts]]);
				meshBuilder.AdvanceVertex();
			}
			meshBuilder.End();
		}
	}
}

void CBrushPrimitive::AddFace(brushFace_t &face)
{
	winding_t winding;
	winding.face = face;
	winding.brush = this;

	int id = m_windingFaces.append(winding);
	m_windingFaces[id].faceId = id;
}

bool CBrushPrimitive::Update()
{
	if (!AssignWindingVertices())
		return false;

	UpdateRenderBuffer();

	return true;
}

void CBrushPrimitive::UpdateRenderBuffer()
{
	DkList<lmodeldrawvertex_t> vertexBuffer;

	for (int i = 0; i < m_windingFaces.numElem(); i++)
	{
		winding_t& winding = m_windingFaces[i];
		DkList<lmodeldrawvertex_t> windingVerts;
		
		for (int j = 0; j < winding.vertex_ids.numElem(); j++)
		{
			lmodeldrawvertex_t vert(m_verts[winding.vertex_ids[j]], vec3_zero, vec2_zero);
			windingVerts.append(vert);
		}

		// calculate texture coordinates for newly generated face draw verts
		winding.CalculateTextureCoordinates(windingVerts.ptr(), windingVerts.numElem());
		vertexBuffer.append(windingVerts);
		vertexBuffer.append(windingVerts[0]);
	}

	if(m_pVB)
	{
		lmodeldrawvertex_t* pData = NULL;

		if(m_pVB->Lock(0, vertexBuffer.numElem(), (void**)&pData, false))
		{
			memcpy(pData, vertexBuffer.ptr(),sizeof(lmodeldrawvertex_t)*vertexBuffer.numElem());

			m_pVB->Unlock();
		}
	}
	else
	{
		m_pVB = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, vertexBuffer.numElem(), sizeof(lmodeldrawvertex_t), vertexBuffer.ptr());
	}
}

bool CBrushPrimitive::Transform(const Matrix4x4& mat)
{
	for (int i = 0; i < m_windingFaces.numElem(); i++)
		m_windingFaces[i].Transform(mat);

	return Update();
}

void CBrushPrimitive::OnRemove()
{
	g_pShaderAPI->DestroyVertexBuffer(m_pVB);
	m_pVB = NULL;
}

void CBrushPrimitive::CalculateVerts(DkList<Vector3D>& verts)
{
	for (int i = 0; i < m_windingFaces.numElem(); i++)
	{
		const Plane& iPlane = m_windingFaces[i].face.Plane;
		for (int j = i + 1; j < m_windingFaces.numElem(); j++)
		{
			const Plane& jPlane = m_windingFaces[j].face.Plane;
			for (int k = j + 1; k < m_windingFaces.numElem(); k++)
			{
				const Plane& kPlane = m_windingFaces[k].face.Plane;

				Vector3D point;
				if (iPlane.GetIntersectionWithPlanes(jPlane, kPlane, point))
				{
					if (IsPointInside(point))
						verts.append(point);
				}
			}
		}
	}
}

float CBrushPrimitive::CheckLineIntersection(const Vector3D &start, const Vector3D &end, Vector3D &intersectionPos, int& face)
{
	bool isinstersects = false;

	Vector3D outintersection;

	float best_fraction = 1.0f;

	Vector3D dir = normalize(end - start);

	for (int i = 0; i < m_windingFaces.numElem(); i++)
	{
		const brushFace_t& brushFace = m_windingFaces[i].face;

		float frac = 1.0f;
		if (brushFace.Plane.GetIntersectionWithRay(start, dir, outintersection))
		{
			frac = lineProjection(start, end, outintersection);

			if (frac < best_fraction && frac >= 0 && IsPointInside(outintersection + dir * 0.1f))
			{
				best_fraction = frac;
				isinstersects = true;
				intersectionPos = outintersection;
				face = i;
			}
		}
	}

	return best_fraction;
}

// is brush aabb intersects another brush aabb
bool CBrushPrimitive::IsBrushIntersectsAABB(CBrushPrimitive *pBrush)
{
	return m_bbox.Intersects(pBrush->GetBBox());
}

bool CBrushPrimitive::IsPointInside_Epsilon(Vector3D &point, float eps)
{
	for (int i = 0; i < m_windingFaces.numElem(); i++)
	{
		if (m_windingFaces[i].face.Plane.Distance(point) > eps)
			return false;
	}

	return true;
}

bool CBrushPrimitive::IsPointInside(Vector3D &point)
{
	for (int i = 0; i < m_windingFaces.numElem(); i++)
	{
		if(m_windingFaces[i].face.Plane.ClassifyPoint(point) == CP_FRONT)
			return false;
	}

	return true;
}

bool CBrushPrimitive::IsWindingFullyInsideBrush(winding_t* pWinding)
{
	int nInside = 0;

	DkList<int>& indices = pWinding->vertex_ids;

	for(int i = 0; i < indices.numElem(); i++)
		nInside += IsPointInside(m_verts[indices[i]]);

	if(nInside == indices.numElem())
		return true;

	return false;
}

bool CBrushPrimitive::IsWindingIntersectsBrush(winding_t* pWinding)
{
	for(int i = 0; i < m_windingFaces.numElem(); i++)
	{
		for(int j = 0; j < m_windingFaces.numElem(); j++)
		{
			Vector3D out_point;

			if(pWinding->face.Plane.GetIntersectionWithPlanes(m_windingFaces[i].face.Plane, m_windingFaces[j].face.Plane, out_point))
			{
				for(int k = 0; k < m_windingFaces.numElem(); k++)
				{
					if(m_windingFaces[k].face.Plane.ClassifyPoint(out_point) == CP_FRONT)
						return false;
				}
			}
		}
	}

	return true;
}

bool CBrushPrimitive::IsTouchesBrush(winding_t* pWinding)
{
	// find one plane that touches another plane
	for(int i = 0; i < m_windingFaces.numElem(); i++)
	{
		if(pWinding->Classify(&m_windingFaces[i]) == CPL_ONPLANE)
			return true;
	}

	return false;
}

// copies this object
CBrushPrimitive* CBrushPrimitive::Clone()
{
	CBrushPrimitive* clone = new CBrushPrimitive();

	for (int i = 0; i < m_windingFaces.numElem(); i++)
	{
		brushFace_t& tex = m_windingFaces[i].face;
		tex.nFlags &= ~BRUSH_FACE_SELECTED;

		clone->AddFace(tex);
	}

	clone->m_bbox = m_bbox;
	clone->Update();
	return clone;
}

void CBrushPrimitive::CutBrushByPlane(Plane &plane, CBrushPrimitive** ppNewBrush)
{
	CBrushPrimitive* newBrush = new CBrushPrimitive;

	for (int i = 0; i < m_windingFaces.numElem(); i++)
	{
		winding_t& winding = m_windingFaces[i];
		for (int j = 0; j < winding.vertex_ids.numElem(); j++)
		{
			ClassifyPlane_e plClass = plane.ClassifyPoint(m_verts[winding.vertex_ids[j]]);

			// only on back of plane
			if (plClass == CP_BACK)
			{
				newBrush->AddFace(m_windingFaces[i].face);
				break;
			}
		}
	}

	if (newBrush->GetFaceCount() == 0)
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
		newBrush->AddFace(face);
	}

	if (!newBrush->Update())
	{
		delete newBrush;
		return;
	}

	*ppNewBrush = newBrush;
}

//-----------------------------------------------------------------------------------

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
bool CBrushPrimitive::WriteObject(IVirtualStream* pStream)
{
	// write face count
	int num_faces = m_windingFaces.numElem();
	pStream->Write(&num_faces, 1, sizeof(int));

	for (int i = 0; i < m_windingFaces.numElem(); i++)
	{
		brushFace_t& face = m_windingFaces[i].face;

		ReadWriteFace_t wface;
		memset(&face, 0, sizeof(ReadWriteFace_t));

		wface.NPlane = face.Plane;
		wface.UAxis = face.UAxis;
		wface.VAxis = face.VAxis;
		wface.vTexScale = face.vScale;
		wface.fTexRotation = face.fRotation;

		if (face.pMaterial)
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
bool CBrushPrimitive::ReadObject(IVirtualStream* pStream)
{
	// write face count
	int num_faces;
	pStream->Read(&num_faces, 1, sizeof(int));

	for (int i = 0; i < num_faces; i++)
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

		AddFace(addFace);
	}

	// calculate face verts
	Update();

	return true;
}

// stores object in keyvalues
void CBrushPrimitive::SaveToKeyValues(kvkeybase_t* pSection)
{
	kvkeybase_t* pFacesSec = pSection->AddKeyBase("faces");
	
	for(int i = 0; i < m_windingFaces.numElem(); i++)
	{
		brushFace_t& face = m_windingFaces[i].face;

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
bool CBrushPrimitive::LoadFromKeyValues(kvkeybase_t* pSection)
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
		AddFace(face);
	}

	// calculate face verts
	Update();

	return true;
}

// creates a brush from volume, e.g a selection box
CBrushPrimitive* CBrushPrimitive::CreateFromVolume(const Volume& volume, IMaterial* material)
{
	CBrushPrimitive* brush = new CBrushPrimitive;

	// define a 6 planes
	for(int i = 0; i < 6; i++)
	{
		brushFace_t face;

		// default some values
		face.fRotation = 0.0f;
		face.vScale = HFIELD_POINT_SIZE;

		// make the N plane from current iteration
		face.Plane = volume.GetPlane(i);
		face.Plane.normal *= -1.0f;
		face.Plane.offset *= -1.0f;

		// make the U and V texture axes
		VectorVectors(face.Plane.normal, face.UAxis.normal, face.VAxis.normal);
		face.UAxis.offset = 0;
		face.VAxis.offset = 0;

		// apply the currently selected material
		face.pMaterial = material;

		// append the face
		brush->AddFace(face);
	}

	// calculate face verts
	if (!brush->Update())
	{
		// don't create empty brushes!
		delete brush;
		return nullptr;
	}

	return brush;
}