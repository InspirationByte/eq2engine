//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editable mesh class
//////////////////////////////////////////////////////////////////////////////////
#include "IDebugOverlay.h"
#include "Math/BoundingBox.h"
#include "EditableSurface.h"
#include "math/math_util.h"
#include "dsm_loader.h"
#include "wx_inc.h" // for file dialog
#include "MaterialListFrame.h"
#include "Utils/GeomTools.h"
#include "Utils/SmartPtr.h"

#include "dsm_obj_loader.h"

void tesselationdata_t::Destroy()
{
	nVerts = 0;
	nIndices = 0;

	if(pVertexData)
		delete [] pVertexData;

	if(pIndexData)
		delete [] pIndexData;

	pVertexData = NULL;
	pIndexData = NULL;
}

inline Vector4D QuadraticInterpolate(const Vector4D & v1, const Vector4D & v2, const Vector4D & v3, float factor)
{	
	return v1 * (1.0f - factor) * (1.0f - factor) + 2.0f * v2 * factor * (1.0f - factor)+ v3 * factor*factor;
}

inline Vector3D QuadraticInterpolate(const Vector3D & v1, const Vector3D & v2, const Vector3D & v3, float factor)
{	
	return v1 * (1.0f - factor) * (1.0f - factor) + 2.0f * v2 * factor * (1.0f - factor)+ v3 * factor*factor;
}

inline Vector2D QuadraticInterpolate(const Vector2D & v1, const Vector2D & v2, const Vector2D & v3, float factor)
{	
	return v1 * (1.0f - factor) * (1.0f - factor) + 2.0f * v2 * factor * (1.0f - factor)+ v3 * factor*factor;
}

inline float QuadraticInterpolate(const float v1, const float v2, const float v3, float factor)
{	
	return v1 * (1.0f - factor) * (1.0f - factor) + 2.0f * v2 * factor * (1.0f - factor)+ v3 * factor*factor;
}

eqlevelvertex_t InterpolateVertex(eqlevelvertex_t &v0, eqlevelvertex_t &v1, eqlevelvertex_t &v2, float factor)
{
	eqlevelvertex_t out;
	out.position = QuadraticInterpolate(v0.position,v1.position,v2.position, factor);
	out.texcoord = QuadraticInterpolate(v0.texcoord,v1.texcoord,v2.texcoord, factor);

	out.binormal = QuadraticInterpolate(v0.binormal,v1.binormal,v2.binormal, factor);
	out.normal = QuadraticInterpolate(v0.normal,v1.normal,v2.normal, factor);
	out.tangent = QuadraticInterpolate(v0.tangent,v1.tangent,v2.tangent, factor);
	out.vertexPaint = QuadraticInterpolate(v0.vertexPaint,v1.vertexPaint,v2.vertexPaint, factor);

	return out;
}

bool TesselatePatch(eqlevelvertex_t* controlPoints, eqlevelvertex_t* vertices,int tesselation)
{
	//Calculate how many vertices across/down there are
	int vertexWidth = tesselation+1;

	//Create space to hold the interpolated vertices up the three columns of control points
	eqlevelvertex_t* column0 = new eqlevelvertex_t[vertexWidth];
	eqlevelvertex_t* column1 = new eqlevelvertex_t[vertexWidth];
	eqlevelvertex_t* column2 = new eqlevelvertex_t[vertexWidth];

	if(!column0 || !column1 || !column2)
	{
		MsgError("ERROR: Not enough memory to tesselate patch\n");
		return false;
	}

	//Tesselate along the columns
	for(int i=0; i<vertexWidth; ++i)
	{
		float factor = (float)i/(vertexWidth-1);
		column0[i] = InterpolateVertex(controlPoints[0], controlPoints[3], controlPoints[6], factor);
		column1[i] = InterpolateVertex(controlPoints[1], controlPoints[4], controlPoints[7], factor);
		column2[i] = InterpolateVertex(controlPoints[2], controlPoints[5], controlPoints[8], factor);
	}

	//Tesselate across the rows to get final vertices
	for(int i=0; i<vertexWidth; ++i)
	{
		for(int j=0; j<vertexWidth; ++j)
		{
			vertices[i*vertexWidth+j] = InterpolateVertex(column0[i], column1[i], column2[i], float(j)/(vertexWidth-1));
		}
	}

	//Free temporary memory
	if(column0)
		delete [] column0;
	column0=NULL;

	if(column1)
		delete [] column1;
	column1=NULL;

	if(column2)
		delete [] column2;
	column2=NULL;
	
	return true;
}

CEditableSurface::CEditableSurface()
{
	m_numAllVerts = 0;
	m_numAllIndices = 0;

	m_nModifyTimes = 0;

	m_bbox[0] = Vector3D(-16);
	m_bbox[1] = Vector3D(16);

	m_pVB = NULL;
	m_pIB = NULL;

	m_pVertexData = NULL;
	m_pIndexData = NULL;

	m_surftype = SURFACE_INVALID;
	m_objtype = EDITABLE_STATIC;

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

	m_nFlags = 0;
}

CEditableSurface::~CEditableSurface()
{
	// DestroyUndoTable();
	Destroy();
}

void CEditableSurface::Destroy()
{
	m_tesselation.Destroy();

	if(m_pVB)
		g_pShaderAPI->DestroyVertexBuffer(m_pVB);

	m_pVB = NULL;

	if(m_pIB)
		g_pShaderAPI->DestroyIndexBuffer(m_pIB);

	g_pShaderAPI->Reset(STATE_RESET_VBO);

	m_pIB = NULL;

	m_numAllVerts = 0;
	m_numAllIndices = 0;

	m_bbox[0] = MAX_COORD_UNITS;
	m_bbox[1] = -MAX_COORD_UNITS;

	m_surftype = SURFACE_INVALID;

	if(m_pVertexData)
		delete [] m_pVertexData;

	if(m_pIndexData)
		delete [] m_pIndexData;

	m_pIndexData = NULL;
	m_pVertexData = NULL;
}

void AddOBJToWorld(const char* pszFileName)
{
	dsmmodel_t *pModel = new dsmmodel_t;

	if(LoadOBJ(pModel, (char*)pszFileName))
	{
		DkList<eqlevelvertex_t> vertexdata;
		DkList<int>				indexdata;

		Msg("Model '%s' is fully parsed, processing:\n", pszFileName);

		for(int i = 0; i < pModel->groups.numElem(); i++)
		{
			dsmgroup_t* pGroup = pModel->groups[i];

			int nNumVerts = pGroup->verts.numElem();

			int nNumTris = nNumVerts/3;

			Msg("Adding group...\n");
			
			vertexdata.assureSize(nNumVerts);
			indexdata.assureSize(nNumVerts);

			vertexdata.setNum(0, false);
			indexdata.setNum(0, false);

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
				eqlevelvertex_t vtx0;
				vtx0.position = pGroup->verts[j].position;
				vtx0.texcoord = pGroup->verts[j].texcoord;
				vtx0.normal = -pGroup->verts[j].normal;
				vtx0.vertexPaint = Vector4D(1,0,0,0);

				eqlevelvertex_t vtx1;
				vtx1.position = pGroup->verts[j+1].position;
				vtx1.texcoord = pGroup->verts[j+1].texcoord;
				vtx1.normal = -pGroup->verts[j+1].normal;
				vtx1.vertexPaint = Vector4D(1,0,0,0);

				eqlevelvertex_t vtx2;
				vtx2.position = pGroup->verts[j+2].position;
				vtx2.texcoord = pGroup->verts[j+2].texcoord;
				vtx2.normal = -pGroup->verts[j+2].normal;
				vtx2.vertexPaint = Vector4D(1,0,0,0);

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

				indexdata.append(idx[2]);
				indexdata.append(idx[1]);
				indexdata.append(idx[0]);
			}

			/*
			for(int j = 0; j < indexdata.numElem(); j+=3)
			{
				int tmp = indexdata[j];
				indexdata[j] = indexdata[j+2];
				indexdata[j+2] = tmp;
			}
			*/

			CEditableSurface* pNewSurface = new CEditableSurface;
			pNewSurface->MakeCustomMesh(vertexdata.ptr(),indexdata.ptr(), vertexdata.numElem(), indexdata.numElem());

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

			pNewSurface->SetMaterial(materials->GetMaterial(pszPath, true));

			pNewSurface->GetSurfaceTexture(0)->nFlags |= STFL_CUSTOMTEXCOORD;

			g_pLevel->AddEditable(pNewSurface);
		}
	}
	else
	{
		MsgError("Error of loading OBJ '%s'!\n", pszFileName);
	}

	FreeDSM(pModel);
	delete pModel;
}

// primitive builders
void CEditableSurface::MakePolygon()
{
	Destroy();

	m_surftex.pMaterial = g_editormainframe->GetMaterials()->GetSelectedMaterial();

	m_surftype = SURFACE_NORMAL;
	m_objtype = EDITABLE_STATIC;

	BeginModify();

	m_numAllVerts = 4;
	m_numAllIndices = 6;

	// copy vertex and index data
	m_pVertexData = new eqlevelvertex_t[m_numAllVerts];
	m_pIndexData = new int[m_numAllIndices];

	m_pVertexData[0] = eqlevelvertex_t(Vector3D(-10,0,-10), Vector2D(0,0), Vector3D(0,1,0));
	m_pVertexData[1] = eqlevelvertex_t(Vector3D(-10,0,10), Vector2D(0,1), Vector3D(0,1,0));
	m_pVertexData[2] = eqlevelvertex_t(Vector3D(10,0,-10), Vector2D(1,0), Vector3D(0,1,0));
	m_pVertexData[3] = eqlevelvertex_t(Vector3D(10,0,10), Vector2D(1,1), Vector3D(0,1,0));

	m_pIndexData[0] = 0;
	m_pIndexData[1] = 1;
	m_pIndexData[2] = 2;

	m_pIndexData[3] = 2;
	m_pIndexData[4] = 1;
	m_pIndexData[5] = 3;

	m_nWide = 1;
	m_nTall = 1;

	EndModify();
}

void CEditableSurface::MakeTerrainPatch(int wide, int tall, int tesselation)
{
	Destroy();

	if(tesselation != -1)
	{
		m_surftex.nFlags |= STFL_TESSELATED;
		m_tesselation.nTesselation = tesselation;
	}

	m_surftex.pMaterial = g_editormainframe->GetMaterials()->GetSelectedMaterial();

	m_surftype = SURFACE_NORMAL;
	m_objtype = EDITABLE_TERRAINPATCH;

	BeginModify();

	// compute vertex and index count
	m_numAllVerts = wide*tall;
	m_numAllIndices = (wide-1)*(tall-1) * 6;

	// copy vertex and index data
	m_pVertexData = new eqlevelvertex_t[m_numAllVerts];
	m_pIndexData = new int[m_numAllIndices];

	m_nWide = wide;
	m_nTall = tall;

	Msg("---  Creating terrain path with %d %d size (%d verts) ---\n", wide, tall, m_numAllVerts);

	int nTriangle = 0;

	if(m_surftex.vScale.x == 0)
		m_surftex.vScale.x = 1;

	if(m_surftex.vScale.y == 0)
		m_surftex.vScale.y = 1;

	int tex_w = 1;
	int tex_h = 1;

	if(m_surftex.pMaterial->GetBaseTexture())
	{
		tex_w = m_surftex.pMaterial->GetBaseTexture()->GetWidth();
		tex_h = m_surftex.pMaterial->GetBaseTexture()->GetHeight();
	}

	Matrix2x2 texmatrix = rotate2(DEG2RAD(m_surftex.fRotation)) * scale2((1.0f / m_surftex.vScale.x)/tex_w, (1.0f / m_surftex.vScale.y)/tex_h);

	// compute vertices
	for(int c = 0; c < tall; c++)
	{
		for(int r = 0; r < wide; r++)
		{
			int vertex_id = r + c*wide;

			float tc_x = (float)(r)/(float)(wide-1);
			float tc_y = (float)(c)/(float)(tall-1);

			float v_pos_x = (tc_x-0.5f)*2.0f;
			float v_pos_y = (tc_y-0.5f)*2.0f;

			Vector2D texCoord = (texmatrix*Vector2D(tc_x,tc_y));// + m_surftex.offset;

			m_pVertexData[vertex_id] = eqlevelvertex_t(Vector3D(v_pos_x,0,v_pos_y), texCoord, Vector3D(0,1,0));
		}
	}

	// compute indices
	// support edge turning - this creates more smoothed terrain, but needs more polygons
	bool bTurnEdge = false;
	for(int c = 0; c < (tall-1); c++)
	{
		for(int r = 0; r < (wide-1); r++)
		{
			int index1 = r + c*wide;
			int index2 = r + (c+1)*wide;
			int index3 = (r+1) + c*wide;
			int index4 = (r+1) + (c+1)*wide;

			if(!bTurnEdge)
			{
				m_pIndexData[nTriangle*3] = index1;
				m_pIndexData[nTriangle*3+1] = index2;
				m_pIndexData[nTriangle*3+2] = index3;

				nTriangle++;

				m_pIndexData[nTriangle*3] = index3;
				m_pIndexData[nTriangle*3+1] = index2;
				m_pIndexData[nTriangle*3+2] = index4;

				nTriangle++;
			}
			else
			{
				m_pIndexData[nTriangle*3] = index1;
				m_pIndexData[nTriangle*3+1] = index2;
				m_pIndexData[nTriangle*3+2] = index4;

				nTriangle++;

				m_pIndexData[nTriangle*3] = index1;
				m_pIndexData[nTriangle*3+1] = index4;
				m_pIndexData[nTriangle*3+2] = index3;

				nTriangle++;
			}

			bTurnEdge = !bTurnEdge;
		}
	}

	UpdateTextureCoords();

	EndModify();
}

void CEditableSurface::MakeCustomTerrainPatch(int wide, int tall, eqlevelvertex_t* vertices, int tesselation)
{
	// create terrain patch with this dimensions
	MakeTerrainPatch(wide,tall, tesselation);

	// copy new vertices
	BeginModify();

	for(int i = 0; i < m_numAllVerts; i++)
		m_pVertexData[i] = vertices[i];

	EndModify();
}

void CEditableSurface::MakeCustomMesh(eqlevelvertex_t* verts, int* indices, int nVerts, int nIndices)
{
	Destroy();

	m_surftype = SURFACE_MODEL;
	m_objtype = EDITABLE_STATIC;

	m_surftex.pMaterial = g_editormainframe->GetMaterials()->GetSelectedMaterial();

	BeginModify();

	m_numAllVerts = nVerts;
	m_numAllIndices = nIndices;

	// copy vertex and index data
	m_pVertexData = new eqlevelvertex_t[m_numAllVerts];
	m_pIndexData = new int[m_numAllIndices];

	memcpy(m_pVertexData, verts, m_numAllVerts * sizeof(eqlevelvertex_t));
	memcpy(m_pIndexData, indices, m_numAllIndices * sizeof(int));

	m_nWide = 1;
	m_nTall = 1;

	EndModify();
}

// material editor support
void CEditableSurface::SetMaterial(IMaterial* pMaterial)
{
	// also record mesh modification
	// RecordObjectForUndo();
	m_surftex.pMaterial = pMaterial;
}

IMaterial* CEditableSurface::GetMaterial()
{
	return m_surftex.pMaterial;
}

// begin/end modification (end unlocks VB)
void CEditableSurface::BeginModify()
{
	m_nModifyTimes++;

	// record mesh
	// RecordObjectForUndo();
}

void CEditableSurface::EndModify()
{
	if(m_nModifyTimes > 1)
		m_nModifyTimes--;

	if(m_nModifyTimes != 1 || m_numAllVerts <= 0 || m_numAllIndices <= 0 || m_surftype == SURFACE_INVALID)
		return;

	eqlevelvertex_t* pDrawVerts		= m_pVertexData;
	int* pDrawIndices				= m_pIndexData;

	int	numDrawVerts				= m_numAllVerts;
	int	numDrawIndices				= m_numAllIndices;

	m_tesselation.Destroy();

	if(m_surftex.nFlags & STFL_TESSELATED)
	{
		int patchWide = (m_nWide-1)/2;
		int patchTall = (m_nTall-1)/2;

		int vertexwide = patchWide * m_tesselation.nTesselation+1;
		int vertextall = patchTall * m_tesselation.nTesselation+1;

		int nControlPoints = m_nWide*m_nTall;

		eqlevelvertex_t *patch_vertices = new eqlevelvertex_t[vertexwide*vertextall];
		eqlevelvertex_t *biquadVertices = new eqlevelvertex_t[(m_tesselation.nTesselation+1)*(m_tesselation.nTesselation+1)];

		m_tesselation.nVerts = vertexwide*vertextall;
		m_tesselation.pVertexData = patch_vertices;

		//Loop through the biquadratic patches
		for(int j = 0; j < patchTall; j++)
		{
			for(int k = 0; k < patchWide; k++)
			{
				//Fill an array with the 9 control points for this biquadratic patch
				eqlevelvertex_t biquadControlPoints[9];

				biquadControlPoints[0] = m_pVertexData[j*m_nWide*2 + k*2];
				biquadControlPoints[1] = m_pVertexData[j*m_nWide*2 + k*2 +1];
				biquadControlPoints[2] = m_pVertexData[j*m_nWide*2 + k*2 +2];

				biquadControlPoints[3] = m_pVertexData[j*m_nWide*2 + m_nWide + k*2];
				biquadControlPoints[4] = m_pVertexData[j*m_nWide*2 + m_nWide + k*2 +1];
				biquadControlPoints[5] = m_pVertexData[j*m_nWide*2 + m_nWide + k*2 +2];

				biquadControlPoints[6] = m_pVertexData[j*m_nWide*2 + m_nWide*2 + k*2];
				biquadControlPoints[7] = m_pVertexData[j*m_nWide*2 + m_nWide*2 + k*2 +1];
				biquadControlPoints[8] = m_pVertexData[j*m_nWide*2 + m_nWide*2 + k*2 +2];

				//Tesselate this biquadratic patch into the biquadVertices
				if(!TesselatePatch(biquadControlPoints, biquadVertices, m_tesselation.nTesselation))
					return;

				//Add the calculated vertices to the array
				for(int l = 0; l < m_tesselation.nTesselation+1; l++)
				{
					for(int m = 0; m < m_tesselation.nTesselation+1; m++)
					{
						int index= (j*vertexwide+k)*m_tesselation.nTesselation + l*vertexwide + m;
						patch_vertices[index] = biquadVertices[l*(m_tesselation.nTesselation+1)+m];
					}
				}
			}
		}

		delete [] biquadVertices;

		int vWide = vertexwide;
		int vTall = vertextall;

		m_tesselation.nIndices = (vWide-1)*(vTall-1)*6;
		m_tesselation.pIndexData = new int[m_tesselation.nIndices];

		int nTriangle = 0;

		// compute indices
		for(int c = 0; c < (vTall-1); c++)
		{
			for(int r = 0; r < (vWide-1); r++)
			{
				int index1 = r + c*vWide;
				int index2 = r + (c+1)*vWide;
				int index3 = (r+1) + c*vWide;
				int index4 = (r+1) + (c+1)*vWide;

				m_tesselation.pIndexData[nTriangle*3] = index1;
				m_tesselation.pIndexData[nTriangle*3+1] = index2;
				m_tesselation.pIndexData[nTriangle*3+2] = index3;

				nTriangle++;

				m_tesselation.pIndexData[nTriangle*3] = index3;
				m_tesselation.pIndexData[nTriangle*3+1] = index2;
				m_tesselation.pIndexData[nTriangle*3+2] = index4;

				nTriangle++;
			}
		}

		for(int i = 0; i < m_tesselation.nIndices; i+=3)
		{
			int idx0 = m_tesselation.pIndexData[i];
			int idx1 = m_tesselation.pIndexData[i+1];
			int idx2 = m_tesselation.pIndexData[i+2];

			Vector3D t,b,n;
			ComputeTriangleTBN(	m_tesselation.pVertexData[idx0].position,m_tesselation.pVertexData[idx1].position,m_tesselation.pVertexData[idx2].position,
								m_tesselation.pVertexData[idx0].texcoord, m_tesselation.pVertexData[idx1].texcoord,m_tesselation.pVertexData[idx2].texcoord, 
								n,t,b);

			m_tesselation.pVertexData[idx0].tangent = t;
			m_tesselation.pVertexData[idx0].binormal = b;
			m_tesselation.pVertexData[idx0].normal = n;

			m_tesselation.pVertexData[idx1].tangent = t;
			m_tesselation.pVertexData[idx1].binormal = b;
			m_tesselation.pVertexData[idx1].normal = n;

			m_tesselation.pVertexData[idx2].tangent = t;
			m_tesselation.pVertexData[idx2].binormal = b;
			m_tesselation.pVertexData[idx2].normal = n;
		}

		for(int i = 0; i < numDrawVerts; i++)
		{
			m_tesselation.pVertexData[i].tangent = normalize(m_tesselation.pVertexData[i].tangent);
			m_tesselation.pVertexData[i].binormal = normalize(m_tesselation.pVertexData[i].binormal);
			m_tesselation.pVertexData[i].normal = normalize(m_tesselation.pVertexData[i].normal);
		}

		pDrawVerts = m_tesselation.pVertexData;
		pDrawIndices = m_tesselation.pIndexData;

		numDrawVerts	= m_tesselation.nVerts;
		numDrawIndices	= m_tesselation.nIndices;
	}

	// can lock VBO or not?
	bool update_only = true;

	// create buffers if not exist
	if(!m_pVB)
	{
		m_pVB = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, numDrawVerts, sizeof(eqlevelvertex_t), pDrawVerts);
		update_only = false;
	}

	if(!m_pIB)
	{
		m_pIB = g_pShaderAPI->CreateIndexBuffer(numDrawIndices, 4, BUFFER_DYNAMIC, pDrawIndices);
		update_only = false;
	}

	// only update
	if(update_only)
	{
		int* indexdata = NULL;
		if(m_pIB->Lock(0, numDrawIndices, (void**)&indexdata, false))
		{
			// copy to index buffer
			memcpy(indexdata, pDrawIndices, numDrawIndices*sizeof(int));

			m_pIB->Unlock();
		}

		eqlevelvertex_t* vbo_vertex_data = NULL;
		if(m_pVB->Lock(0,numDrawVerts,(void**)&vbo_vertex_data,false))
		{
			// copy to vertex buffer
			memcpy(vbo_vertex_data, pDrawVerts, numDrawVerts*sizeof(eqlevelvertex_t));

			m_pVB->Unlock();
		}
	}

	m_nModifyTimes--;

	RebuildBoundingBox();
}

#include "IDebugOverlay.h"

int	CEditableSurface::GetInstersectingTriangle(const Vector3D &start, const Vector3D &end)
{
	Vector3D best_point = end;
	float best_dist = MAX_COORD_UNITS;
	int best_triangle = -1;

	IMatVar* pVar = m_surftex.pMaterial->FindMaterialVar("nocull");

	bool bDoTwoSided = false;
	if(pVar)
		bDoTwoSided = (pVar->GetInt() > 0);

	float fraction = 1.0f;

	Vector3D ray_dir = end-start;

	eqlevelvertex_t* pDrawVerts		= m_pVertexData;
	int* pDrawIndices				= m_pIndexData;

	int	numDrawVerts				= m_numAllVerts;
	int	numDrawIndices				= m_numAllIndices;

	int tri_idx = 0;

	for(int i = 0; i < numDrawIndices; i+=3)
	{
		Vector3D v0,v1,v2;

		v0 = pDrawVerts[pDrawIndices[i]].position;
		v1 = pDrawVerts[pDrawIndices[i+1]].position;
		v2 = pDrawVerts[pDrawIndices[i+2]].position;

		float dist = MAX_COORD_UNITS+1;

		if(IsRayIntersectsTriangle(v0,v1,v2, start, ray_dir, dist, bDoTwoSided))
		{
			if(dist < best_dist && dist > 0)
			{
				best_dist = dist;
				fraction = dist;

				best_point = lerp(start, end, dist);
				best_triangle = tri_idx;
			}
		}

		tri_idx++;
	}

	return best_triangle;
}

// checks object for intersection. useful for Selection
float CEditableSurface::CheckLineIntersection(const Vector3D &start, const Vector3D &end, Vector3D &intersectionPos)
{
	Vector3D best_point = end;
	float best_dist = MAX_COORD_UNITS;

	IMatVar* pVar = m_surftex.pMaterial->FindMaterialVar("nocull");

	bool bDoTwoSided = false;
	if(pVar)
		bDoTwoSided = (pVar->GetInt() > 0);

	float fraction = 1.0f;

	Vector3D ray_dir = end-start;

	eqlevelvertex_t* pDrawVerts		= m_pVertexData;
	int* pDrawIndices				= m_pIndexData;

	int	numDrawVerts				= m_numAllVerts;
	int	numDrawIndices				= m_numAllIndices;

	if(m_surftex.nFlags & STFL_TESSELATED)
	{
		pDrawVerts = m_tesselation.pVertexData;
		pDrawIndices = m_tesselation.pIndexData;

		numDrawVerts	= m_tesselation.nVerts;
		numDrawIndices	= m_tesselation.nIndices;
	}

	for(int i = 0; i < numDrawIndices; i+=3)
	{
		if(	i+1 > numDrawIndices ||
			i+2 > numDrawIndices)
			break;

		Vector3D v0,v1,v2;

		if(pDrawIndices[i] >= numDrawVerts ||
			pDrawIndices[i+1] >= numDrawVerts || 
			pDrawIndices[i+2] >= numDrawVerts ||
			pDrawIndices[i] < 0 ||
			pDrawIndices[i+1] < 0 || 
			pDrawIndices[i+2] < 0)
			continue;

		v0 = pDrawVerts[pDrawIndices[i]].position;
		v1 = pDrawVerts[pDrawIndices[i+1]].position;
		v2 = pDrawVerts[pDrawIndices[i+2]].position;

		float dist = MAX_COORD_UNITS+1;

		if(IsRayIntersectsTriangle(v0,v1,v2, start, ray_dir, dist, bDoTwoSided))
		{
			if(dist < best_dist && dist > 0)
			{
				best_dist = dist;
				fraction = dist;

				best_point = lerp(start, end, dist);
			}
		}
	}

	intersectionPos = best_point;

	return fraction;
}

int	CEditableSurface::GetVertexCount()
{
	return m_numAllVerts;
}

int	CEditableSurface::GetIndexCount()
{
	return m_numAllIndices;
}

// returns vertices. NOTE: don't modify mesh without BeginModify\EndModify
eqlevelvertex_t* CEditableSurface::GetVertices()
{
	return m_pVertexData;
}

// returns indices. NOTE: don't modify mesh without BeginModify\EndModify
int* CEditableSurface::GetIndices()
{
	return m_pIndexData;
}

IVertexBuffer* CEditableSurface::GetVertexBuffer()
{
	return m_pVB;
}

IIndexBuffer* CEditableSurface::GetIndexBuffer()
{
	return m_pIB;
}

Vector3D CEditableSurface::GetBoundingBoxMins()
{
	return m_bbox[0];
}

Vector3D CEditableSurface::GetBoundingBoxMaxs()
{
	return m_bbox[1];
}

void CEditableSurface::RebuildBoundingBox()
{
	m_bbox[0] = MAX_COORD_UNITS;
	m_bbox[1] = -MAX_COORD_UNITS;

	for(int i = 0; i < m_numAllVerts; i++)
	{
		POINT_TO_BBOX(m_pVertexData[i].position, m_bbox[0], m_bbox[1]);
	}

	if(m_bbox[0].x >= m_bbox[1].x)
		m_bbox[1].x = m_bbox[0].x + 1;

	if(m_bbox[0].y >= m_bbox[1].y)
		m_bbox[1].y = m_bbox[0].y + 1;

	if(m_bbox[0].z >= m_bbox[1].z)
		m_bbox[1].z = m_bbox[0].z + 1;
}

void CEditableSurface::Render(int nViewRenderFlags)
{
	if(m_nModifyTimes || m_surftype == SURFACE_INVALID)
		return;

	// cancel if transparents disabled
	if((nViewRenderFlags & VR_FLAG_NO_TRANSLUCENT) && (m_surftex.pMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT))
		return;
		
	// cancel if opaques disabled
	if((nViewRenderFlags & VR_FLAG_NO_OPAQUE) && !(m_surftex.pMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT))
		return;

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	materials->SetCullMode(CULL_BACK);

	//if(!(nViewRenderFlags & VR_FLAG_NO_MATERIALS))
		materials->BindMaterial(m_surftex.pMaterial, false);
		/*
	else
	{
		//if(m_nRenderMode == VDM_SHADOW)
		{
			viewrenderer->UpdateDepthSetup(false,false);

#ifdef EQLC
			pMaterial->GetShader()->InitParams();
#endif

			float fAlphatestFac = 0.0f;

			if(m_surftex.pMaterial->GetFlags() & MATERIAL_FLAG_ALPHATESTED)
				fAlphatestFac = 50.0f;

			g_pShaderAPI->SetPixelShaderConstantFloat(0, fAlphatestFac);
			g_pShaderAPI->SetTexture(m_surftex.pMaterial->GetShader()->GetBaseTexture(0));
			g_pShaderAPI->ApplyTextures();
			
		}
	}
	*/

	g_pShaderAPI->SetVertexFormat(g_pLevel->GetLevelVertexFormat());
	g_pShaderAPI->SetVertexBuffer(m_pVB, 0);
	g_pShaderAPI->SetIndexBuffer(m_pIB);

	//if(viewrenderer->Get == VDM_SHADOW)
	//viewrenderer->UpdateDepthSetup();

	materials->Apply();

	int	numDrawVerts				= m_numAllVerts;
	int	numDrawIndices				= m_numAllIndices;

	if(m_surftex.nFlags & STFL_TESSELATED)
	{
		numDrawVerts	= m_tesselation.nVerts;
		numDrawIndices	= m_tesselation.nIndices;
	}

	g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, 0, numDrawIndices, 0, numDrawVerts);

	/*
	if((m_surftex->nFlags & STFL_SELECTED) && !g_editormainframe->GetSurfaceDialog()->IsSelectionMaskDisabled())
	{
		ColorRGBA sel_color(1.0f,0.0f,0.0f,0.35f);
		materials->SetAmbientColor(sel_color);

		materials->BindMaterial(g_pLevel->GetFlatMaterial(), false);
		materials->SetDepthStencilStates(true, false);

		materials->Apply();
		g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, 0, numDrawIndices, 0, numDrawVerts);
	}
	*/
}

void CEditableSurface::RenderGhost(Vector3D &addpos, Vector3D &addscale, Vector3D &addrot, bool use_center, Vector3D &center)
{
	if(m_nModifyTimes || m_surftype == SURFACE_INVALID)
		return;

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

	materials->SetMatrix(MATRIXMODE_WORLD,rendermatrix);

	materials->BindMaterial(g_pLevel->GetFlatMaterial(), false);

	g_pShaderAPI->SetVertexFormat(g_pLevel->GetLevelVertexFormat());
	g_pShaderAPI->SetVertexBuffer(m_pVB, 0);
	g_pShaderAPI->SetIndexBuffer(m_pIB);

	materials->Apply();

	int	numDrawVerts				= m_numAllVerts;
	int	numDrawIndices				= m_numAllIndices;

	if(m_surftex.nFlags & STFL_TESSELATED)
	{
		numDrawVerts	= m_tesselation.nVerts;
		numDrawIndices	= m_tesselation.nIndices;
	}

	g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, 0, numDrawIndices, 0, numDrawVerts);
}

EditableSurfaceType_e CEditableSurface::GetSurfaceType()
{
	return m_surftype;
}

void CEditableSurface::Translate(Vector3D &position)
{
	BeginModify();

	for(int i = 0; i < GetVertexCount(); i++)
	{
		m_pVertexData[i].position += position;
	}

	UpdateTextureCoords(position);

	EndModify();
}

void CEditableSurface::Scale(Vector3D &scale, bool use_center, Vector3D &scale_center)
{
	BeginModify();

	Vector3D bbox_center = (GetBBoxMins() + GetBBoxMaxs())*0.5f;

	if(use_center)
		bbox_center = scale_center;

	for(int i = 0; i < GetVertexCount(); i++)
	{
		m_pVertexData[i].position -= bbox_center;
		m_pVertexData[i].position *= scale;
		m_pVertexData[i].position += bbox_center;
	}

	bool invert = false;
	if(scale.x < 0)
		invert = !invert;
	if(scale.y < 0)
		invert = !invert;
	if(scale.z < 0)
		invert = !invert;

	if(invert)
	{
		for(int i = 0; i < GetIndexCount(); i+=3)
		{
			int temp = m_pIndexData[i];
			m_pIndexData[i] = m_pIndexData[i+2];
			m_pIndexData[i+2] = temp;
		}
	}

	UpdateTextureCoords();

	EndModify();
}

void CEditableSurface::Rotate(Vector3D &rotation_angles, bool use_center, Vector3D &rotation_center)
{
	BeginModify();

	Vector3D bbox_center = (GetBBoxMins() + GetBBoxMaxs())*0.5f;

	if(use_center)
		bbox_center = rotation_center;

	Matrix3x3 rotation = rotateXYZ3(DEG2RAD(rotation_angles.x),DEG2RAD(rotation_angles.y),DEG2RAD(rotation_angles.z));

	m_surftex.Plane.normal = normalize( rotation * m_surftex.Plane.normal );
	m_surftex.UAxis.normal = normalize( rotation * m_surftex.UAxis.normal );
	m_surftex.VAxis.normal = normalize( rotation * m_surftex.VAxis.normal );

	for(int i = 0; i < GetVertexCount(); i++)
	{
		m_pVertexData[i].position -= bbox_center;

		m_pVertexData[i].position = rotation * m_pVertexData[i].position;

		m_pVertexData[i].position += bbox_center;
	}

	UpdateTextureCoords();

	EndModify();
}

void CEditableSurface::FlipFaces()
{
	BeginModify();

	for(int i = 0; i < GetIndexCount(); i+=3)
	{
		int temp = m_pIndexData[i];
		m_pIndexData[i] = m_pIndexData[i+2];
		m_pIndexData[i+2] = temp;
	}

	EndModify();
}

// rendering bbox
Vector3D CEditableSurface::GetBBoxMins()
{
	return m_bbox[0];
}

Vector3D CEditableSurface::GetBBoxMaxs()
{
	return m_bbox[1];
}

void CEditableSurface::UpdateTextureCoords(Vector3D &vMovement)
{
	// record mesh
	// RecordObjectForUndo();

	BeginModify();

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

	for(int i = 0; i < GetVertexCount(); i++)
	{
		if(!(m_surftex.nFlags & STFL_CUSTOMTEXCOORD))
		{
			float U, V;
		
			U = dot(m_surftex.UAxis.normal, m_pVertexData[i].position);
			U /= ( ( float )texSizeW ) * m_surftex.vScale.x;
			U += ( m_surftex.UAxis.offset / ( float )texSizeW );

			V = dot(m_surftex.VAxis.normal, m_pVertexData[i].position);
			V /= ( ( float )texSizeH ) * m_surftex.vScale.y;
			V += ( m_surftex.VAxis.offset / ( float )texSizeH );

			m_pVertexData[i].texcoord.x = U;
			m_pVertexData[i].texcoord.y = V;
		}

		m_pVertexData[i].tangent = vec3_zero;
		m_pVertexData[i].binormal = vec3_zero;
		m_pVertexData[i].normal = vec3_zero;
	}

	for(int i = 0; i < GetIndexCount(); i+=3)
	{
		int idx0 = m_pIndexData[i];
		int idx1 = m_pIndexData[i+1];
		int idx2 = m_pIndexData[i+2];

		Vector3D t,b,n;
		ComputeTriangleTBN(	m_pVertexData[idx0].position,m_pVertexData[idx1].position,m_pVertexData[idx2].position,
							m_pVertexData[idx0].texcoord, m_pVertexData[idx1].texcoord,m_pVertexData[idx2].texcoord, 
							n,t,b);

		float fTriangleArea = TriangleArea(m_pVertexData[idx0].position,m_pVertexData[idx1].position,m_pVertexData[idx2].position);

		m_pVertexData[idx0].tangent += t*fTriangleArea;
		m_pVertexData[idx0].binormal += b*fTriangleArea;
		m_pVertexData[idx0].normal += n*fTriangleArea;

		m_pVertexData[idx1].tangent += t*fTriangleArea;
		m_pVertexData[idx1].binormal += b*fTriangleArea;
		m_pVertexData[idx1].normal += n*fTriangleArea;

		m_pVertexData[idx2].tangent += t*fTriangleArea;
		m_pVertexData[idx2].binormal += b*fTriangleArea;
		m_pVertexData[idx2].normal += n*fTriangleArea;
	}

	for(int i = 0; i < GetVertexCount(); i++)
	{
		m_pVertexData[i].tangent = normalize(m_pVertexData[i].tangent);
		m_pVertexData[i].binormal = normalize(m_pVertexData[i].binormal);
		m_pVertexData[i].normal = normalize(m_pVertexData[i].normal);

		//debugoverlay->Line3D(m_pVertexData[i].position, m_pVertexData[i].position+m_pVertexData[i].tangent*10,ColorRGBA(1,0,0,1),ColorRGBA(1,0,0,1),10);
		//debugoverlay->Line3D(m_pVertexData[i].position, m_pVertexData[i].position+m_pVertexData[i].binormal*10,ColorRGBA(0,1,0,1),ColorRGBA(0,1,0,1),10);
		//debugoverlay->Line3D(m_pVertexData[i].position, m_pVertexData[i].position+m_pVertexData[i].normal*10,ColorRGBA(0,0,1,1),ColorRGBA(0,0,1,1),10);
	}

	EndModify();
}

void CEditableSurface::OnRemove(bool bOnLevelCleanup)
{
	Destroy();
}

// saves this object
bool CEditableSurface::WriteObject(IVirtualStream* pStream)
{
	// externally write object type again =)
	int nType = m_objtype;
	pStream->Write(&nType, 1, sizeof(int));

	// write surface type
	int nSurfType = m_surftype;
	pStream->Write(&nSurfType, 1, sizeof(int));

	// write vertex count and index count
	pStream->Write(&m_numAllVerts, 1, sizeof(int));
	pStream->Write(&m_numAllIndices, 1, sizeof(int));

	// dump vertex and index data
	pStream->Write(m_pVertexData, 1, sizeof(eqlevelvertex_t) * m_numAllVerts);
	pStream->Write(m_pIndexData, 1, sizeof(int) * m_numAllIndices);

	// write surface shared normal
	pStream->Write(&m_surftex.Plane.normal, 1, sizeof(Vector3D));

	// prepare and write texture projection info
	ReadWriteSurfaceTexture_t surfData;
	surfData.Plane = m_surftex.Plane;
	surfData.UAxis = m_surftex.UAxis;
	surfData.VAxis = m_surftex.VAxis;
	surfData.scale = m_surftex.vScale;
	surfData.nFlags = m_surftex.nFlags;
	surfData.nTesseleation = m_tesselation.nTesselation;

	if(m_surftex.pMaterial)
		strcpy(surfData.material, m_surftex.pMaterial->GetName());
	else
		strcpy(surfData.material, "ERROR");

	pStream->Write(&surfData, 1, sizeof(ReadWriteSurfaceTexture_t));

	// write terrain patch sizes
	pStream->Write(&m_nWide, 1, sizeof(int));
	pStream->Write(&m_nTall, 1, sizeof(int));

	return true;
}

/*
struct ReadWriteSurfaceTextureOld_t
{
	char		material[256];	// material and texture
	Plane		UAxis;
	Plane		VAxis;

	float		rotate;
	Vector2D	scale;

	int			nFlags;
	int			nTesseleation;
};*/

// read this object
bool CEditableSurface::ReadObject(IVirtualStream* pStream)
{
	// read external object type again =)
	int nType;
	pStream->Read(&nType, 1, sizeof(int));

	BeginModify();

	m_objtype = (EditableType_e)nType;

	// read surface type
	int nSurfType;
	pStream->Read(&nSurfType, 1, sizeof(int));

	m_surftype = (EditableSurfaceType_e)nSurfType;

	// read vertex count and index count
	pStream->Read(&m_numAllVerts, 1, sizeof(int));
	pStream->Read(&m_numAllIndices, 1, sizeof(int));

	// allocate buffers
	m_pVertexData = new eqlevelvertex_t[m_numAllVerts];
	m_pIndexData = new int[m_numAllIndices];

	// read vertex and index data
	pStream->Read(m_pVertexData, 1, sizeof(eqlevelvertex_t) * m_numAllVerts);
	pStream->Read(m_pIndexData, 1, sizeof(int) * m_numAllIndices);

	// read surface shared normal
	pStream->Read(&m_surftex.Plane.normal, 1, sizeof(Vector3D));

	// read texture projection info
	ReadWriteSurfaceTexture_t surfData;

	pStream->Read(&surfData, 1, sizeof(ReadWriteSurfaceTexture_t));

	m_surftex.Plane = surfData.Plane;
	m_surftex.UAxis = surfData.UAxis;
	m_surftex.VAxis = surfData.VAxis;
	m_surftex.vScale = surfData.scale;
	m_surftex.nFlags = surfData.nFlags;
	m_tesselation.nTesselation = surfData.nTesseleation;

	m_surftex.pMaterial = materials->GetMaterial(surfData.material,true);

	// read terrain patch sizes
	pStream->Read(&m_nWide, 1, sizeof(int));
	pStream->Read(&m_nTall, 1, sizeof(int));


	EndModify();

	return true;
}

// stores object in keyvalues
void CEditableSurface::SaveToKeyValues(kvkeybase_t* pSection)
{
	EqString modelFileName(varargs("/surfdata/obj_%d.edm", m_id));
	EqString leveldir(EqString("worlds/")+g_pLevel->GetLevelName());
	EqString object_full_filename(leveldir + modelFileName);

	// remove it
	g_fileSystem->FileRemove(object_full_filename.GetData(), SP_MOD);

	IFile* pStream = g_fileSystem->Open(object_full_filename.GetData(), "wb");

	// save model
	WriteObject(pStream);

	g_fileSystem->Close(pStream);

	pSection->AddKeyBase("meshfile", (char*)modelFileName.GetData());
}

// stores object in keyvalues
bool CEditableSurface::LoadFromKeyValues(kvkeybase_t* pSection)
{
	EqString leveldir(EqString("worlds/")+g_pLevel->GetLevelName());
	kvkeybase_t* pPair = pSection->FindKeyBase("meshfile");

	if(pPair)
	{
		EqString object_full_filename(leveldir + pPair->values[0]);

		IFile* pStream = g_fileSystem->Open(object_full_filename.GetData(), "rb");
		if(!pStream)
		{
			MsgError("Failed to open '%s'\n", object_full_filename.GetData());
			return false;
		}

		bool state = ReadObject(pStream);

		g_fileSystem->Close(pStream);

		return state;
	}

	return true;
}



/*
// called when all level builds
void CEditableSurface::BuildObject(level_build_data_t* pLevelBuildData)
{
	IMatVar* pNodraw = m_surftex.pMaterial->FindMaterialVar("nodraw");
	IMatVar* pNoCollide = m_surftex.pMaterial->FindMaterialVar("nocollide");

	bool nodraw = false;
	bool nocollide = false;

	if(pNodraw && pNodraw->GetInt() > 0)
		nodraw = true;

	if(pNoCollide && pNoCollide->GetInt() > 0)
		nocollide = true;

	if(nocollide && nodraw)
		return;

	eqlevelvertex_t* pDrawVerts		= m_pVertexData;
	int* pDrawIndices				= m_pIndexData;

	int	numDrawVerts				= m_numAllVerts;
	int	numDrawIndices				= m_numAllIndices;

	if(m_surftex.nFlags & STFL_TESSELATED)
	{
		pDrawVerts = m_tesselation.pVertexData;
		pDrawIndices = m_tesselation.pIndexData;

		numDrawVerts	= m_tesselation.nVerts;
		numDrawIndices	= m_tesselation.nIndices;
	}

	if(!nodraw)
	{
		int start_vert = pLevelBuildData->vertices.numElem();
		int start_indx = pLevelBuildData->indices.numElem();

		int material_id = pLevelBuildData->used_materials.findIndex(m_surftex.pMaterial);

		if(material_id == -1)
			material_id = pLevelBuildData->used_materials.append(m_surftex.pMaterial);

		// build surface data
		eqlevelsurf_t surf;

		surf.material_id = material_id;
		surf.flags = 0;
		surf.firstindex = start_indx;
		surf.firstvertex = start_vert;

		surf.numindices = numDrawIndices;
		surf.numvertices = numDrawVerts;

		// for now occlusion wasn't built
//		surf.occdata_geom_id = -1;

		BoundingBox bbox;

		// add vertices
		for(int i = 0; i < numDrawVerts; i++)
		{
			pLevelBuildData->vertices.append(pDrawVerts[i]);
			bbox.AddVertex(pDrawVerts[i].position);
		}

		surf.mins = bbox.minPoint;
		surf.maxs = bbox.maxPoint;

		// add indices with offset
		for(int i = 0; i < numDrawIndices; i++)
		{
			int idx = start_vert + pDrawIndices[i];
			pLevelBuildData->indices.append(idx);
		}

		// add surface
		pLevelBuildData->surfaces.append(surf);
	}

	if(!nocollide)
	{
		int material_id = pLevelBuildData->used_materials.findIndex(m_surftex.pMaterial);

		if(material_id == -1)
			material_id = pLevelBuildData->used_materials.append(m_surftex.pMaterial);

		// build physics data
		int phys_start_vert = pLevelBuildData->phys_vertices.numElem();

		// add vertices
		for(int i = 0; i < numDrawVerts; i++)
			pLevelBuildData->phys_vertices.append(pDrawVerts[i]);

		// subdivide surface physics geometry on 4 surfaces
		for(int i = 0; i < 4; i++)
		{
			DkList<int> grouptriangleindices;

			// build triangle list that mostly painted
			for(int j = 0; j < numDrawIndices; j += 3)
			{
				int i0 = pDrawIndices[j];
				int i1 = pDrawIndices[j+1];
				int i2 = pDrawIndices[j+2];

				float factor = (pDrawVerts[i0].vertexPaint[i] + pDrawVerts[i1].vertexPaint[i] + pDrawVerts[i2].vertexPaint[i]) / 3.0f;
				if(factor > 0.2f)
				{
					grouptriangleindices.append(phys_start_vert+i0);
					grouptriangleindices.append(phys_start_vert+i1);
					grouptriangleindices.append(phys_start_vert+i2);
				}
			}

			if(grouptriangleindices.numElem() == 0)
				continue;

			//
			// Build physics surface data
			//

			eqlevelphyssurf_t phys_surface;

			int first_index = pLevelBuildData->phys_indices.numElem();

			phys_surface.material_id = material_id;
			phys_surface.firstindex = first_index;
			phys_surface.firstvertex = phys_start_vert;

			phys_surface.numindices = grouptriangleindices.numElem();
			phys_surface.numvertices = numDrawVerts;

			IMatVar* pSurfPropsVar = m_surftex.pMaterial->FindMaterialVar(varargs("surfaceprops%d", i+1));

			if(!pSurfPropsVar)
				pSurfPropsVar = m_surftex.pMaterial->FindMaterialVar("surfaceprops");

			if(pSurfPropsVar)
				strcpy(phys_surface.surfaceprops, pSurfPropsVar->GetString());
			else
				strcpy(phys_surface.surfaceprops, "default");

			// add surface
			pLevelBuildData->phys_surfaces.append(phys_surface);

			// add triangles
			pLevelBuildData->phys_indices.append(grouptriangleindices);
		}
	}
}
*/

void CEditableSurface::CutSurfaceByPlane(Plane &plane, CEditableSurface** ppSurface)
{
	int nNegative = 0;

	for(int i = 0; i < m_numAllVerts; i++)
	{
		if(plane.ClassifyPointEpsilon( m_pVertexData[i].position, 0.0f) == CP_BACK)
			nNegative++;
	}
	
	if(nNegative == m_numAllVerts)
	{
		*ppSurface = (CEditableSurface*)CloneObject();
		return;
	}

	if(nNegative == 0)
	{
		return;
	}

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

				/*
				if (front)
				{
					front->copyVertex(nFrontVertices++, *this, i + front0);
					front->copyVertex(nFrontVertices++, *this, i + front1);
					front->interpolateVertex(nFrontVertices++, *this, i + front1, i + front2, x0);

					front->copyVertex(nFrontVertices++, *this, i + front0);
					front->interpolateVertex(nFrontVertices++, *this, i + front1, i + front2, x0);
					front->interpolateVertex(nFrontVertices++, *this, i + front0, i + front2, x1);
				}

				if (back)
				{
					back->copyVertex(nBackVertices++, *this, i + front2);
					back->interpolateVertex(nBackVertices++, *this, i + front0, i + front2, x1);
					back->interpolateVertex(nBackVertices++, *this, i + front1, i + front2, x0);
				}
				*/
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

				/*
				if (front)
				{
					front->copyVertex(nFrontVertices++, *this, i + front0);
					front->interpolateVertex(nFrontVertices++, *this, i + front0, i + front1, x0);
					front->interpolateVertex(nFrontVertices++, *this, i + front0, i + front2, x1);
				}

				if (back)
				{
					back->copyVertex(nBackVertices++, *this, i + front1);
					back->copyVertex(nBackVertices++, *this, i + front2);
					back->interpolateVertex(nBackVertices++, *this, i + front0, i + front2, x1);

					back->copyVertex(nBackVertices++, *this, i + front1);
					back->interpolateVertex(nBackVertices++, *this, i + front0, i + front2, x1);
					back->interpolateVertex(nBackVertices++, *this, i + front0, i + front1, x0);
				}
				*/
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

		CEditableSurface* pSurf = new CEditableSurface;
		*ppSurface = pSurf;

		pSurf->m_objtype = m_objtype;
		pSurf->m_surftype = m_surftype;
		pSurf->m_surftex = m_surftex;
		pSurf->m_surftex.nFlags &= ~STFL_SELECTED | STFL_TESSELATED;

		pSurf->m_numAllVerts = vertices.numElem();
		pSurf->m_numAllIndices = indices.numElem();

		pSurf->m_pVertexData = new eqlevelvertex_t[pSurf->m_numAllVerts];
		pSurf->m_pIndexData = new int[pSurf->m_numAllIndices];

		memcpy(pSurf->m_pVertexData, vertices.ptr(), sizeof(eqlevelvertex_t) * vertices.numElem());
		memcpy(pSurf->m_pIndexData, indices.ptr(), sizeof(int) * indices.numElem());

		pSurf->BeginModify();
		pSurf->EndModify();
	}
}


// add surface geometry, and use this material
void CEditableSurface::AddSurfaceGeometry(CEditableSurface* pSurface)
{
	DkList<int>				indices;
	DkList<eqlevelvertex_t> vertices;

	indices.resize(m_numAllIndices + pSurface->m_numAllIndices);
	vertices.resize(m_numAllVerts + pSurface->m_numAllVerts);

	for(int i = 0; i < m_numAllVerts; i++)
		vertices.append(m_pVertexData[i]);

	for(int i = 0; i < m_numAllIndices; i++)
		indices.append(m_pIndexData[i]);

	for(int i = 0; i < pSurface->m_numAllVerts; i++)
		vertices.append(pSurface->m_pVertexData[i]);

	for(int i = 0; i < pSurface->m_numAllIndices; i++)
		indices.append(m_numAllVerts + pSurface->m_pIndexData[i]);

	BeginModify();

	Destroy();

	m_surftype = SURFACE_NORMAL;

	m_numAllVerts = vertices.numElem();
	m_numAllIndices = indices.numElem();

	m_pVertexData = new eqlevelvertex_t[m_numAllVerts];
	m_pIndexData = new int[m_numAllIndices];

	memcpy(m_pVertexData, vertices.ptr(), sizeof(eqlevelvertex_t) * vertices.numElem());
	memcpy(m_pIndexData, indices.ptr(), sizeof(int) * indices.numElem());

	EndModify();
}

// copies this object
CBaseEditableObject*  CEditableSurface::CloneObject()
{
	CEditableSurface* pClone = new CEditableSurface;
	pClone->m_objtype = m_objtype;
	pClone->m_surftype = m_surftype;
	pClone->m_surftex = m_surftex;
	pClone->m_surftex.nFlags &= ~STFL_SELECTED;

	pClone->m_pVertexData = new eqlevelvertex_t[m_numAllVerts];
	pClone->m_pIndexData = new int[m_numAllIndices];
	pClone->m_numAllVerts = m_numAllVerts;
	pClone->m_numAllIndices = m_numAllIndices;

	pClone->SetName(GetName());

	memcpy(pClone->m_pVertexData, m_pVertexData, sizeof(eqlevelvertex_t)*m_numAllVerts);
	memcpy(pClone->m_pIndexData, m_pIndexData, sizeof(int)*m_numAllIndices);

	pClone->m_tesselation = m_tesselation;

	pClone->m_tesselation.nVerts = 0;
	pClone->m_tesselation.nIndices = 0;
	pClone->m_tesselation.pIndexData = NULL;
	pClone->m_tesselation.pVertexData = NULL;

	pClone->BeginModify();
	pClone->EndModify();

	return pClone;
}