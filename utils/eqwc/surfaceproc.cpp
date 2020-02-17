//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Surface processing and tesselation
//////////////////////////////////////////////////////////////////////////////////

#include "threadedprocess.h"
#include "eqwc.h"
#include "Utils/GeomTools.h"


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

#define MIN_EDGE_LENGTH		0.0001f
#define MIN_EDGE_COLLINEAR	(1.0f-0.000001f)

bool TriangleIsValid(eqlevelvertex_t &v0, eqlevelvertex_t &v1, eqlevelvertex_t &v2)
{
	// also check the edge directions (identifical angle, etc)
	Vector3D edges[3] = {	v0.position-v1.position,
							v1.position-v2.position,
							v2.position-v0.position};

	for(int i = 0; i < 3; i++)
	{
		int vi = (i < 2) ? (i+1) : 0;

		float edgelen = length(edges[i]);

		// check edge lengths
		if(edgelen < MIN_EDGE_LENGTH)
			return false;

		// check edges for collinearness
		if(dot(normalize(edges[i]),normalize(edges[vi])) > MIN_EDGE_COLLINEAR)
			return false;
	}

	return true;
}

bool TriangleIsDuplicate(int tri1[3], int tri2[3])
{
	int t1Variant[3][3] = {	{tri1[0], tri1[1], tri1[2]},
							{tri1[2], tri1[1], tri1[0]},
							{tri1[2], tri1[0], tri1[1]} };

	int t2Variant[3][3] = {	{tri2[0], tri2[1], tri2[2]},
							{tri2[2], tri2[1], tri2[0]},
							{tri2[2], tri2[0], tri2[1]} };

	for(int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			if(	t1Variant[i][0] == t2Variant[j][0] && 
				t1Variant[i][1] == t2Variant[j][1] &&
				t1Variant[i][2] == t2Variant[j][2])
				return true;
		}
	}

	return false;
}

// fixes surface from invalid triangles
void FixSurface(cwsurface_t* pSurf)
{
	int* vert_remap = new int[pSurf->numVertices];

	eqlevelvertex_t* pVerts = pSurf->pVerts;
	int* pIndices			= pSurf->pIndices;

	int numVerts			= pSurf->numVertices;
	int numIndices			= pSurf->numIndices;

	DkList<eqlevelvertex_t> new_verts;
	DkList<int>				new_indices;

	new_verts.resize(numVerts);
	new_indices.resize(numIndices);

	int nRemovedDegenerates = 0;
	int nRemovedDuplicates = 0;

#define ADD_REMAPPED_VERTEX(idx, vertex)				\
{														\
	if(vert_remap[ idx ] != -1)							\
	{													\
		new_indices.append( vert_remap[ idx ] );		\
	}													\
	else												\
	{													\
		int currVerts = new_verts.numElem();			\
		vert_remap[idx] = currVerts;					\
		new_indices.append( currVerts );				\
		new_verts.append( vertex );						\
	}													\
}

	for(int i = 0; i < pSurf->numVertices; i++)
		vert_remap[i] = -1;

	for(int i = 0; i < numIndices; i+=3)
	{
		int triIdx[3] = {pIndices[i], pIndices[i+1], pIndices[i+2]};

		if( TriangleIsValid( pVerts[triIdx[0]],pVerts[triIdx[1]],pVerts[triIdx[2]]) )
		{
			ADD_REMAPPED_VERTEX(triIdx[0], pVerts[triIdx[0]]);
			ADD_REMAPPED_VERTEX(triIdx[1], pVerts[triIdx[1]]);
			ADD_REMAPPED_VERTEX(triIdx[2], pVerts[triIdx[2]]);
		}
		else
			nRemovedDegenerates++;
	}

	if(nRemovedDegenerates)
		MsgWarning("%d degenerates and %d duplicates removed\n", nRemovedDegenerates, nRemovedDuplicates);

	delete [] pVerts;
	delete [] pIndices;
	delete [] vert_remap;

	pSurf->numVertices = new_verts.numElem();
	pSurf->numIndices = new_indices.numElem();

	pSurf->pVerts = new eqlevelvertex_t[new_verts.numElem()];
	pSurf->pIndices = new int[new_indices.numElem()];

	memcpy(pSurf->pVerts, new_verts.ptr(),new_verts.numElem()*sizeof(eqlevelvertex_t));
	memcpy(pSurf->pIndices, new_indices.ptr(),new_indices.numElem()*sizeof(int));
}

cwsurface_t* CloneSurface(cwsurface_t* pSurf)
{
	// just copy surface or tesselate
	cwsurface_t* pProcSurf = new cwsurface_t;

	// copy surface contents
	pProcSurf->flags = pSurf->flags;
	pProcSurf->nSmoothingGroup = pSurf->nSmoothingGroup;

	pProcSurf->surfflags = pSurf->surfflags;

	pProcSurf->pMaterial = pSurf->pMaterial;

	pProcSurf->numIndices = pSurf->numIndices;
	pProcSurf->pIndices = new int[pProcSurf->numIndices];
	memcpy(pProcSurf->pIndices, pSurf->pIndices, sizeof(int)*pProcSurf->numIndices);

	pProcSurf->numVertices = pSurf->numVertices;
	pProcSurf->pVerts = new eqlevelvertex_t[pProcSurf->numVertices];
	memcpy(pProcSurf->pVerts, pSurf->pVerts, sizeof(eqlevelvertex_t)*pProcSurf->numVertices);

	pProcSurf->nTesselation = 0;
	pProcSurf->nWide = 0;
	pProcSurf->nTall = 0;
	pProcSurf->surfflags = 0;

	return pProcSurf;
}

void SurfaceToGlobalList(cwsurface_t* pSurf)
{
	if(pSurf->flags & CS_FLAG_OCCLUDER)
	{
		ThreadLock();
		g_occlusionsurfaces.append(CloneSurface(pSurf));
		ThreadUnlock();
		return;
	}

	//if(g_bOnlyEnts)
	//	return;

	// just copy surface or tesselate
	cwsurface_t* pProcSurf = new cwsurface_t;

	// copy surface contents
	pProcSurf->flags = pSurf->flags;
	pProcSurf->nSmoothingGroup = pSurf->nSmoothingGroup;

	pProcSurf->surfflags = pSurf->surfflags;

	pProcSurf->pMaterial = pSurf->pMaterial;

	pProcSurf->nTesselation = pSurf->nTesselation;
	pProcSurf->nWide = pSurf->nWide;
	pProcSurf->nTall = pSurf->nTall;

	/*
	if(pProcSurf->surfflags & STFL_NOCOLLIDE)
		pProcSurf->flags |= CS_FLAG_NOCOLLIDE;

	if(pProcSurf->surfflags & STFL_NOSUBDIVISION)
		pProcSurf->flags |= CS_FLAG_DONTSUBDIVIDE;
		*/
	
	// tesselate or just copy the buffers
	if(pProcSurf->surfflags & STFL_TESSELATED)
	{
		int nW = pProcSurf->nWide;
		int nH = pProcSurf->nTall;

		int patchWide = (nW-1)/2;
		int patchTall = (nH-1)/2;

		int vertexwide = patchWide * pProcSurf->nTesselation+1;
		int vertextall = patchTall * pProcSurf->nTesselation+1;

		int nControlPoints = nW*nH;

		eqlevelvertex_t *patch_vertices = new eqlevelvertex_t[vertexwide*vertextall];
		eqlevelvertex_t *biquadVertices = new eqlevelvertex_t[(pProcSurf->nTesselation+1)*(pProcSurf->nTesselation+1)];

		pProcSurf->numVertices = vertexwide*vertextall;
		pProcSurf->pVerts = patch_vertices;

		//Loop through the biquadratic patches
		for(int j = 0; j < patchTall; j++)
		{
			for(int k = 0; k < patchWide; k++)
			{
				//Fill an array with the 9 control points for this biquadratic patch
				eqlevelvertex_t biquadControlPoints[9];

				biquadControlPoints[0] = pSurf->pVerts[j*nW*2 + k*2];
				biquadControlPoints[1] = pSurf->pVerts[j*nW*2 + k*2 +1];
				biquadControlPoints[2] = pSurf->pVerts[j*nW*2 + k*2 +2];

				biquadControlPoints[3] = pSurf->pVerts[j*nW*2 + nW + k*2];
				biquadControlPoints[4] = pSurf->pVerts[j*nW*2 + nW + k*2 +1];
				biquadControlPoints[5] = pSurf->pVerts[j*nW*2 + nW + k*2 +2];

				biquadControlPoints[6] = pSurf->pVerts[j*nW*2 + nW*2 + k*2];
				biquadControlPoints[7] = pSurf->pVerts[j*nW*2 + nW*2 + k*2 +1];
				biquadControlPoints[8] = pSurf->pVerts[j*nW*2 + nW*2 + k*2 +2];

				//Tesselate this biquadratic patch into the biquadVertices
				if(!TesselatePatch(biquadControlPoints, biquadVertices, pProcSurf->nTesselation))
					return;

				//Add the calculated vertices to the array
				for(int l = 0; l < pProcSurf->nTesselation+1; l++)
				{
					for(int m = 0; m < pProcSurf->nTesselation+1; m++)
					{
						int index= (j*vertexwide+k)*pProcSurf->nTesselation + l*vertexwide + m;
						patch_vertices[index] = biquadVertices[l*(pProcSurf->nTesselation+1)+m];
					}
				}
			}
		}

		delete [] biquadVertices;

		int vWide = vertexwide;
		int vTall = vertextall;

		pProcSurf->numIndices = (vWide-1)*(vTall-1)*6;
		pProcSurf->pIndices = new int[pProcSurf->numIndices];

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

				pProcSurf->pIndices[nTriangle*3] = index1;
				pProcSurf->pIndices[nTriangle*3+1] = index2;
				pProcSurf->pIndices[nTriangle*3+2] = index3;

				nTriangle++;

				pProcSurf->pIndices[nTriangle*3] = index3;
				pProcSurf->pIndices[nTriangle*3+1] = index2;
				pProcSurf->pIndices[nTriangle*3+2] = index4;

				nTriangle++;
			}
		}

		for(int i = 0; i < pProcSurf->numIndices; i+=3)
		{
			int idx0 = pProcSurf->pIndices[i];
			int idx1 = pProcSurf->pIndices[i+1];
			int idx2 = pProcSurf->pIndices[i+2];

			Vector3D t,b,n;
			ComputeTriangleTBN(	pProcSurf->pVerts[idx0].position, pProcSurf->pVerts[idx1].position, pProcSurf->pVerts[idx2].position,
								pProcSurf->pVerts[idx0].texcoord, pProcSurf->pVerts[idx1].texcoord, pProcSurf->pVerts[idx2].texcoord, 
								n,t,b);

			pProcSurf->pVerts[idx0].tangent = t;
			pProcSurf->pVerts[idx0].binormal = b;
			pProcSurf->pVerts[idx0].normal = n;

			pProcSurf->pVerts[idx1].tangent = t;
			pProcSurf->pVerts[idx1].binormal = b;
			pProcSurf->pVerts[idx1].normal = n;

			pProcSurf->pVerts[idx2].tangent = t;
			pProcSurf->pVerts[idx2].binormal = b;
			pProcSurf->pVerts[idx2].normal = n;
		}

		for(int i = 0; i < pProcSurf->numVertices; i++)
		{
			pProcSurf->pVerts[i].tangent = normalize(pProcSurf->pVerts[i].tangent);
			pProcSurf->pVerts[i].binormal = normalize(pProcSurf->pVerts[i].binormal);
			pProcSurf->pVerts[i].normal = normalize(pProcSurf->pVerts[i].normal);
		}
	}
	else
	{
		pProcSurf->numIndices = pSurf->numIndices;
		pProcSurf->pIndices = new int[pProcSurf->numIndices];
		memcpy(pProcSurf->pIndices, pSurf->pIndices, sizeof(int)*pProcSurf->numIndices);

		pProcSurf->numVertices = pSurf->numVertices;
		pProcSurf->pVerts = new eqlevelvertex_t[pProcSurf->numVertices];
		memcpy(pProcSurf->pVerts, pSurf->pVerts, sizeof(eqlevelvertex_t)*pProcSurf->numVertices);
	}

	// Fix surface from some errors
	FixSurface( pProcSurf );

	pProcSurf->nTesselation = 0;
	pProcSurf->nWide = 0;
	pProcSurf->nTall = 0;
	pProcSurf->surfflags &= ~STFL_TESSELATED;

	ThreadLock();
	g_surfaces.append(pProcSurf);
	ThreadUnlock();
}

void ComputeSurfaceBBOX(cwsurface_t* pSurf)
{
	pSurf->bbox.Reset();

	for(int i = 0; i < pSurf->numVertices; i++)
	{
		pSurf->bbox.AddVertex(pSurf->pVerts[i].position);
	}
}

void ComputeSurfaceBBOX(cwlitsurface_t* pSurf)
{
	pSurf->bbox.Reset();

	for(int i = 0; i < pSurf->numVertices; i++)
		pSurf->bbox.AddVertex(pSurf->pVerts[i].position);
}

void MergeSurfaceIntoList(cwsurface_t* pSurface, DkList<cwsurface_t*> &surfaces, bool bCheckTranslucents)
{
	int nSurfaceIndex = -1;
	for(int i = 0; i < surfaces.numElem(); i++)
	{
		int surfFlags = pSurface->pMaterial->GetFlags();
		bool isTranslucent = false;

		if(bCheckTranslucents)
		{
			if(	(surfFlags & MATERIAL_FLAG_TRANSPARENT) ||
				(surfFlags & MATERIAL_FLAG_ADDITIVE) || 
				(surfFlags & MATERIAL_FLAG_MODULATE))
			{
				isTranslucent = true;
			}
		}

		if((surfaces[i]->pMaterial == pSurface->pMaterial) && !isTranslucent)
		{
			nSurfaceIndex = i;
			break;
		}
	}

	if(nSurfaceIndex == -1)
	{
		cwsurface_t* pNewSurface = new cwsurface_t;
		surfaces.append(pNewSurface);

		*pNewSurface = *pSurface;

		pNewSurface->pVerts = new eqlevelvertex_t[pSurface->numVertices];
		pNewSurface->pIndices = new int[pSurface->numIndices];

		// copy data
		memcpy(pNewSurface->pVerts, pSurface->pVerts, sizeof(eqlevelvertex_t)*pSurface->numVertices);
		memcpy(pNewSurface->pIndices, pSurface->pIndices, sizeof(int)*pSurface->numIndices);
	}
	else
	{
		// reallocate index and vertex buffers
		cwsurface_t* pNewSurface = surfaces[nSurfaceIndex];

		eqlevelvertex_t*	pVerts		= new eqlevelvertex_t[pNewSurface->numVertices + pSurface->numVertices];
		int*				pIndices	= new int[pNewSurface->numIndices + pSurface->numIndices];

		// copy old contents to new buffers
		memcpy(pVerts, pNewSurface->pVerts, sizeof(eqlevelvertex_t)*pNewSurface->numVertices);
		memcpy(pIndices, pNewSurface->pIndices, sizeof(int)*pNewSurface->numIndices);

		// copy new contents
		memcpy(pVerts+pNewSurface->numVertices, pSurface->pVerts, sizeof(eqlevelvertex_t)*pSurface->numVertices);

		// indices is always remapped
		for(int i = 0; i < pSurface->numIndices; i++)
			pIndices[pNewSurface->numIndices+i] = (pSurface->pIndices[i] + pNewSurface->numVertices);

		pNewSurface->numVertices += pSurface->numVertices;
		pNewSurface->numIndices += pSurface->numIndices;

		delete [] pNewSurface->pVerts;
		delete [] pNewSurface->pIndices;

		pNewSurface->pVerts = pVerts;
		pNewSurface->pIndices = pIndices;
	}
}

void TheadedSurfToGlobalList(int i)
{
	SurfaceToGlobalList(g_surfacemodels[i]);

	FreeSurface(g_surfacemodels[i]);
}

void ProcessSurfaces()
{
	MsgWarning("\nProcessing %d surfaces...\n", g_surfacemodels.numElem());
	
	StartPacifier("SurfacesToGlobalList ");

	RunThreadsOnIndividual(g_surfacemodels.numElem(), true, TheadedSurfToGlobalList);

	/*
	for(int i = 0; i < g_surfacemodels.numElem(); i++)
	{
		UpdatePacifier((float)i / (float)g_surfacemodels.numElem());
		SurfaceToGlobalList(g_surfacemodels[i]);

		FreeSurface(g_surfacemodels[i]);
	}
	*/
	EndPacifier();

	g_surfacemodels.clear();

	StartPacifier("LevelSurfacesBBox ");

	worldGlobals.worldBox.Reset();

	for(int i = 0; i < g_surfaces.numElem(); i++)
	{
		UpdatePacifier((float)i / (float)g_surfaces.numElem());
		ComputeSurfaceBBOX(g_surfaces[i]);

		worldGlobals.worldBox.AddVertex(g_surfaces[i]->bbox.minPoint);
		worldGlobals.worldBox.AddVertex(g_surfaces[i]->bbox.maxPoint);
	}

	EndPacifier();

	Msg("\n");
}

struct litsurf_save_data_t
{
	// surface group material
	char		material[256];

	// basic surface data
	int			numIndices;
	int			numVertices;

	int			flags;

	int			lightmap_id;

	Vector3D	mins;
	Vector3D	maxs;

	int			layer_index;

	// this surface size including header
	int			size;
};

cwlitsurface_t* LoadLitSurface(IVirtualStream* pStream)
{
	litsurf_save_data_t rdata;

	pStream->Read(&rdata, 1, sizeof(litsurf_save_data_t));

	// check size
	int cSize = sizeof(litsurf_save_data_t) + sizeof(int)*rdata.numIndices + sizeof(eqlevelvertexlm_t)*rdata.numVertices;

	if(cSize != rdata.size)
	{
		MsgError("Backup data stream error!\n");
		return NULL;
	}

	cwlitsurface_t* pSurf = new cwlitsurface_t;

	pSurf->bbox.minPoint = rdata.mins;
	pSurf->bbox.maxPoint = rdata.maxs;

	pSurf->numIndices = rdata.numIndices;
	pSurf->numVertices = rdata.numVertices;

	pSurf->flags = rdata.flags;
	pSurf->layer_index = rdata.layer_index;
	pSurf->lightmap_id = rdata.lightmap_id;

	pSurf->pMaterial = materials->GetMaterial( rdata.material, true );

	pSurf->pVerts = new eqlevelvertexlm_t[pSurf->numVertices];
	pSurf->pIndices = new int[pSurf->numIndices];

	// read data
	pStream->Read(pSurf->pVerts, 1, sizeof(eqlevelvertexlm_t)*pSurf->numVertices);
	pStream->Read(pSurf->pIndices, 1, sizeof(int)*pSurf->numIndices);

	return pSurf;
}

bool SaveLitSurface(cwlitsurface_t* pSurf, IVirtualStream* pStream)
{
	litsurf_save_data_t wdata;

	wdata.flags = pSurf->flags;
	wdata.layer_index = pSurf->layer_index;
	wdata.lightmap_id = pSurf->lightmap_id;

	wdata.mins = pSurf->bbox.minPoint;
	wdata.maxs = pSurf->bbox.maxPoint;

	wdata.numIndices = pSurf->numIndices;
	wdata.numVertices = pSurf->numVertices;

	strcpy(wdata.material, pSurf->pMaterial->GetName());

	wdata.size = sizeof(litsurf_save_data_t) + sizeof(int)*pSurf->numIndices + sizeof(eqlevelvertexlm_t)*pSurf->numVertices;

	// write header
	pStream->Write(&wdata, 1, sizeof(litsurf_save_data_t));

	// write data
	pStream->Write(pSurf->pVerts, 1, sizeof(eqlevelvertexlm_t)*pSurf->numVertices);
	pStream->Write(pSurf->pIndices, 1, sizeof(int)*pSurf->numIndices);

	return true;
}