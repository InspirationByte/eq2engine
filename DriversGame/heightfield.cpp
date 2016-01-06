//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Heightfield
//////////////////////////////////////////////////////////////////////////////////

#include "heightfield.h"
#include "world.h"
#include "VirtualStream.h"
#include "IDebugOverlay.h"
#include "math/math_util.h"

CHeightTileField::CHeightTileField()
{
	m_position = vec3_zero;
	m_points = NULL;

	m_sizew = 0;
	m_sizeh = 0;

	m_regionPos = 0;

	m_fieldIdx = 0;

	m_userData = NULL;
}

CHeightTileField::~CHeightTileField()
{
	Destroy();
}

void CHeightTileField::Init(int size, IVector2D& regionPos)
{
	m_sizew = size;
	m_sizeh = size;

	m_regionPos = regionPos;

	m_points = new hfieldtile_t[m_sizew*m_sizeh];
}

void CHeightTileField::Destroy()
{
	delete [] m_points;
	m_points = NULL;

	for(int i = 0; i < m_materials.numElem(); i++)
	{
		materials->FreeMaterial( m_materials[i]->material );
		delete m_materials[i];
	}
	m_materials.clear();
}

struct material_s
{
	char materialname[260];
};

ALIGNED_TYPE(material_s,4) material_t;

void CHeightTileField::ReadFromStream( IVirtualStream* stream )
{
	int numMaterials = 0;
	int matNamesSize = 0;

	if(!m_points)
		m_points = new hfieldtile_t[m_sizew*m_sizeh];

	stream->Read(m_points, m_sizew*m_sizeh, sizeof(hfieldtile_t));

	stream->Read(&numMaterials, sizeof(int), 1);
	stream->Read(&matNamesSize, 1, sizeof(int));

	char* matNamesData = new char[matNamesSize];

	stream->Read(matNamesData, 1, matNamesSize);

	char* matNamePtr = matNamesData;

	for(int i = 0; i < numMaterials; i++)
	{
		hfieldmaterial_t* matBundle = new hfieldmaterial_t();
		matBundle->material = materials->FindMaterial(matNamePtr, true);
		matBundle->atlas = TexAtlas_LoadAtlas(("materials/"+_Es(matNamePtr)+".atlas").c_str(), matNamePtr, true);

		if(matBundle->material)
			matBundle->material->Ref_Grab();

		m_materials.append( matBundle );

		// valid?
		matNamePtr += strlen(matNamePtr)+1;
	}

	delete [] matNamesData;
}

bool CHeightTileField::IsEmpty()
{
	for(int x = 0; x < m_sizew; x++)
	{
		for(int y = 0; y < m_sizeh; y++)
		{
			if(m_points[y*m_sizew+x].texture != -1)
				return false;
		}
	}

	return true;
}

int CHeightTileField::WriteToStream( IVirtualStream* stream )
{
	long fpos = stream->Tell();

	// write heightfield data
	stream->Write(m_points, m_sizew*m_sizeh, sizeof(hfieldtile_t));

	// write material names
	{
		int numMaterials = m_materials.numElem();

		char nullSymbol = '\0';

		// write model names
		CMemoryStream matNamesData;
		matNamesData.Open(NULL, VS_OPEN_WRITE, 2048);

		for(int i = 0; i < numMaterials; i++)
		{
			matNamesData.Print( m_materials[i]->material->GetName() );
			matNamesData.Write( &nullSymbol, 1, 1 );
		}

		matNamesData.Write(&nullSymbol, 1, 1);

		int matNamesSize = matNamesData.Tell();

		stream->Write(&numMaterials, 1, sizeof(int));
		stream->Write(&matNamesSize, 1, sizeof(int));

		stream->Write(matNamesData.GetBasePointer(), 1, matNamesSize);
	}

	// return written byte count
	return stream->Tell() - fpos;
}

// optimizes heightfield, removing unused cells
void CHeightTileField::Optimize()
{
	// refresh size, position

	// GenerateRenderData();
}

// get/set
bool CHeightTileField::SetPointMaterial(int x, int y, IMaterial* pMaterial, int atlIdx)
{
	if(	(x >= m_sizew || y >= m_sizeh) ||
		(x < 0 || y < 0))
		return false;

	int tileIdx = y*m_sizew + x;
	hfieldtile_t& tile = m_points[tileIdx];

	if(pMaterial == NULL)
	{
		int matIdx = tile.texture;

		// decrement
		//if(matIdx >= 0)
		//	m_materials[matIdx]->material->Ref_Drop();

		tile.texture = -1;
	}
	else
	{
		int matIdx = -1;
		for(int i = 0; i < m_materials.numElem(); i++)
		{
			if(m_materials[i]->material == pMaterial)
			{
				matIdx = i;
				break;
			}
		}

		if(matIdx == -1)
		{
			// try to load atlas
			hfieldmaterial_t* matBundle = new hfieldmaterial_t;
			matBundle->material = pMaterial;
			matBundle->atlas = TexAtlas_LoadAtlas(("materials/"+_Es(pMaterial->GetName())+".atlas").c_str(), pMaterial->GetName(), true);

			matIdx = m_materials.append(matBundle);
		}

		if(tile.texture == matIdx && tile.atlasIdx == atlIdx)
			return false;

		// decrement
		//if(tile.texture >= 0)
		//	m_materials[tile.texture]->material->Ref_Drop();

		// increment
		tile.texture = matIdx;
		tile.atlasIdx = atlIdx;

		pMaterial->Ref_Grab();
	}

	return true;
}

// returns tile for modifying
hfieldtile_t* CHeightTileField::GetTile(int x, int y) const
{
	// достать соседа
	CHeightTileField* neighbour = NULL;

	return GetTileAndNeighbourField(x, y, &neighbour);
}

void UTIL_GetTileIndexes(const IVector2D& tileXY, const IVector2D& fieldWideTall, IVector2D& outTileXY, IVector2D& outFieldOffset)
{
	// if we're out of bounds - try to find neightbour tile
	if(	(tileXY.x >= fieldWideTall.x || tileXY.y >= fieldWideTall.y) ||
		(tileXY.x < 0 || tileXY.y < 0))
	{
		// only -1/+1, no more
		int ofs_x = (tileXY.x < 0) ? -1 : ((tileXY.x >= fieldWideTall.x) ? 1 : 0 );
		int ofs_y = (tileXY.y < 0) ? -1 : ((tileXY.y >= fieldWideTall.y) ? 1 : 0 );

		outFieldOffset.x = ofs_x;
		outFieldOffset.y = ofs_y;

		// достать соседа
		// rolling
		outTileXY.x = ROLLING_VALUE(tileXY.x, fieldWideTall.x);
		outTileXY.y = ROLLING_VALUE(tileXY.y, fieldWideTall.y);

		return;
	}

	outFieldOffset.x = 0;
	outFieldOffset.y = 0;

	outTileXY = tileXY;
}

hfieldtile_t* CHeightTileField::GetTileAndNeighbourField(int x, int y, CHeightTileField** field) const
{
	// if we're out of bounds - try to find neightbour tile
	if(	(x >= m_sizew || y >= m_sizeh) ||
		(x < 0 || y < 0))
	{
		if(m_regionPos.x < 0)
			return NULL;

		// only -1/+1, no more
		int ofs_x = (x < 0) ? -1 : ((x >= m_sizew) ? 1 : 0 );
		int ofs_y = (y < 0) ? -1 : ((y >= m_sizeh) ? 1 : 0 );

		// достать соседа
		*field = g_pGameWorld->m_level.GetHeightFieldAt( IVector2D(m_regionPos.x + ofs_x, m_regionPos.y + ofs_y), m_fieldIdx );

		if(*field)
		{
			// rolling
			int tofs_x = ROLLING_VALUE(x, (*field)->m_sizew);
			int tofs_y = ROLLING_VALUE(y, (*field)->m_sizeh);

			return (*field)->GetTile(tofs_x, tofs_y);
		}
		else
			return NULL;
	}

	return &m_points[y*m_sizew + x];
}

void CHeightTileField::GetTileTBN(int x, int y, Vector3D& tang, Vector3D& binorm, Vector3D& norm) const
{
	int dx[] = NEIGHBOR_OFFS_XDX(x, 1);
	int dy[] = NEIGHBOR_OFFS_YDY(y, 1);

	hfieldtile_t* tile = GetTile(x,y);
	Vector3D tilePosition(x*HFIELD_POINT_SIZE, (float)tile->height*HFIELD_HEIGHT_STEP, y*HFIELD_POINT_SIZE);

	Vector3D t(0,1,1);
	Vector3D b(1,1,0);

	// tangent and binormal, positive and negative
	Vector3D tp(0),tn(0);
	Vector3D bp(0),bn(0);

	int nIter = 0;

	// get neighbour tiles
	for(int i = 0; i < 8; i++)
	{
		//bool isDetached = (tile->flags & EHTILE_DETACHED) ? EHTILE_DETACHED : 0;

		// get the tiles only with corresponding detaching
		hfieldtile_t* ntile = GetTile(dx[i], dy[i]);

		if(!ntile)
			continue;

		if(((ntile->flags & EHTILE_DETACHED) > 0) != ((tile->flags & EHTILE_DETACHED) > 0) &&
			ntile->height < tile->height)
			continue;

		if(ntile->texture == -1)
			continue;

		Vector3D ntilePosition(dx[i]*HFIELD_POINT_SIZE, (float)ntile->height*HFIELD_HEIGHT_STEP, dy[i]*HFIELD_POINT_SIZE);

		// make only y has sign
		Vector3D tt = (ntilePosition-tilePosition)*t;
		Vector3D bb = (ntilePosition-tilePosition)*b;
		
		float ttd = dot(vec3_forward, tt);
		float bbd = dot(vec3_right, bb);

		if(ttd > 0)
			tp += Vector3D(0.0f, tt.y, ttd);
		else
			tn += Vector3D(0.0f, tt.y, ttd);

		if(bbd > 0)
			bp += Vector3D(bbd, bb.y, 0.0f);
		else
			bn += Vector3D(bbd, bb.y, 0.0f);

		//tp.y += tt.y;
		//bp.y += bb.y;

		nIter++;
	}

	// single tile island?
	if(nIter <= 2)
	{
		tang = Vector3D(0.0f, 0.0f, 1.0f);
		binorm = Vector3D(1.0f, 0.0f, 0.0f);
		norm = Vector3D(0.0f, 1.0f, 0.0f);

		return;
	}

	tang = tp-tn;
	binorm = bp-bn;

	if(lengthSqr(tang) <= 0.01f)
		tang = Vector3D(0.0f, 0.0f, 1.0f);

	if(lengthSqr(binorm) <= 0.01f)
		binorm = Vector3D(1.0f, 0.0f, 0.0f);

	tang = fastNormalize(tang);
	binorm = fastNormalize(binorm);
	norm = cross(tang, binorm);

	/*
	{
		debugoverlay->Line3D(m_position+tilePosition, m_position+tilePosition+tang, ColorRGBA(1,0,0,1), ColorRGBA(1,0,0,1), 0.0f );
		debugoverlay->Line3D(m_position+tilePosition, m_position+tilePosition+binorm, ColorRGBA(0,1,0,1), ColorRGBA(0,1,0,1), 0.0f );
		debugoverlay->Line3D(m_position+tilePosition, m_position+tilePosition+norm, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1), 0.0f );
	}*/

	return;
}

hfieldtile_t* CHeightTileField::GetTile_CheckFlag(int x, int y, int flag, bool enabled) const
{
	hfieldtile_t* tile = GetTile(x,y);

	if(!tile)
		return NULL;

	if((tile->flags & flag) > 0 != enabled)
		return NULL;

	return tile;
}

// returns face at position
bool CHeightTileField::PointAtPos(const Vector3D& pos, int& x, int& y) const
{
	Vector3D zeroedPos = pos - m_position;

	float p_size = (1.0f / HFIELD_POINT_SIZE);

	Vector2D xz_pos = zeroedPos.xz() * p_size;

	x = xz_pos.x+0.5f;
	y = xz_pos.y+0.5f;

	if(x < 0 || x >= m_sizew)
		return false;

	if(y < 0 || y >= m_sizeh)
		return false;

	return true;
}

void EdgeIndexToVertex(int edge, int& vi1, int& vi2)
{
	int i1, i2;

	i1 = edge;
	i2 = edge-1;

	if(i1 < 0)
		i1 = 3;

	if(i2 < 0)
		i2 = 3;

	vi1 = i1;
	vi2 = i2;
}

hfieldbatch_t* FindBatchInList(IMaterial* pMaterial, DkList<hfieldbatch_t*>& batches, bool useSplitCoords, int sx, int sy)
{
	for(int i = 0; i < batches.numElem(); i++)
	{
		IMaterial* mat = batches[i]->materialBundle->material;

		if(useSplitCoords)
		{
			if(mat == pMaterial && batches[i]->sx == sx && batches[i]->sy == sy)
				return batches[i];
		}
		else if(mat == pMaterial)
			return batches[i];
	}

	return NULL;
}

int valid_edge_index(int idx)
{
	int eidx = idx;

	if(eidx > 2)
		eidx -= 3;

	if(eidx < 0)
		eidx += 3;

	return eidx;
}

#define HFIELD_SUBDIVIDE (16.0f)

bool hfieldVertexComparator(const hfielddrawvertex_t& a, const hfielddrawvertex_t& b)
{
	if(a.position == b.position && a.texcoord == b.texcoord)
		return true;

	return false;
}

void CHeightTileField::Generate(bool generate_render, DkList<hfieldbatch_t*>& batches )
{
	Vector3D hfield_offset = generate_render ? m_position : vec3_zero;//Vector3D(HFIELD_POINT_SIZE, 0, HFIELD_POINT_SIZE)*0.5f;

	// generate polys
	for(int x = 0; x < m_sizew; x++)
	{
		for(int y = 0; y < m_sizeh; y++)
		{
			int pt_idx = y*m_sizew + x;
			hfieldtile_t& point = m_points[pt_idx];

			if( point.texture == -1 )
				continue;

			int sx = floor((float)x / HFIELD_SUBDIVIDE);
			int sy = floor((float)y / HFIELD_SUBDIVIDE);

			//Msg("tile=%d %d sxsy=%d %d\n", x,y, sx, sy);

			hfieldbatch_t* batch = FindBatchInList( m_materials[point.texture]->material, batches, generate_render, sx, sy);

			float fTexelX = 0.0f;
			float fTexelY = 0.0f;

			if(!batch)
			{
				int nBatchFlags = 0;

				IMaterial* material = m_materials[point.texture]->material;

				IMatVar* nocollide = material->FindMaterialVar("nocollide");
				IMatVar* detach = material->FindMaterialVar("detached");
				IMatVar* addwall = material->FindMaterialVar("addwall");
				IMatVar* rotatable = material->FindMaterialVar("rotatable");

				if(nocollide)
					nBatchFlags |= nocollide->GetInt() ? (EHTILE_NOCOLLIDE) : 0;

				if(detach)
					nBatchFlags |= detach->GetInt() ? EHTILE_DETACHED : 0;

				if(addwall)
					nBatchFlags |= addwall->GetInt() ? EHTILE_ADDWALL : 0;

				if(rotatable)
					nBatchFlags |= rotatable->GetInt() ? EHTILE_ROTATABLE : 0;

				if(!generate_render && (nBatchFlags & EHTILE_NOCOLLIDE))
					continue;

				batch = new hfieldbatch_t;
				batch->materialBundle = m_materials[point.texture];
				batch->verts.resize(m_sizew*m_sizeh*6);
				batch->indices.resize(m_sizew*m_sizeh*6);
				batch->flags = nBatchFlags;

				batch->sx = sx;
				batch->sy = sy;

				batches.append(batch);
			}

			IMaterial* batchMaterial = batch->materialBundle->material;
			CTextureAtlas* batchAtlas = batch->materialBundle->atlas;

			/*
			if(batchMaterial->GetBaseTexture())
			{
				fTexelX = 1.0f / batchMaterial->GetBaseTexture()->GetWidth();
				fTexelY = 1.0f / batchMaterial->GetBaseTexture()->GetHeight();
			}*/

			int vertex_heights[4] = {point.height, point.height, point.height, point.height};

			int xv[4] = NEIGHBOR_OFFS_X(x);
			int yv[4] = NEIGHBOR_OFFS_Y(y);

			int xvd[4] = NEIGHBOR_OFFS_DX(x, 1);
			int yvd[4] = NEIGHBOR_OFFS_DY(y, 1);

			int pointFlags = (point.flags | batch->flags);

			bool isDetached = (pointFlags & EHTILE_DETACHED) > 0;
			bool addWallOnEdges = ((pointFlags & EHTILE_ADDWALL) && generate_render) || ((pointFlags & EHTILE_ADDWALL) && (pointFlags & EHTILE_COLLIDE_WALL));
			bool isEmpty = (pointFlags & EHTILE_EMPTY) > 0;
			bool rotatable = (pointFlags & EHTILE_ROTATABLE) > 0;

			if(!generate_render && (pointFlags & EHTILE_NOCOLLIDE))
				continue;

			bool verts_stripped[4] = { false, false, false, false };
			bool edges_stripped[4] = {false, false, false, false};
			bool edges_wall[4] = { false, false, false, false };
			int  edge_stripped_height[4] = {0,0,0,0};

			// настраиваем каждую вершину тайла по высоте
			for(int i = 0; i < 4; i++)
			{
				//GetTile(xv[i], yv[i]);
				hfieldtile_t* ntile = GetTile(xv[i], yv[i]);//GetTile_CheckFlag(xv[i], yv[i], EHTILE_DETACHED, isDetached);

				int v1, v2;
				EdgeIndexToVertex(i, v1, v2);

				// сначала с соседей по граням
				if (ntile && ntile->texture != -1 && ((ntile->flags & EHTILE_DETACHED) > 0) == isDetached)
				{
					if( ntile->height > vertex_heights[v1] )
						vertex_heights[v1] = ntile->height;

					if( ntile->height > vertex_heights[v2] )
						vertex_heights[v2] = ntile->height;
				}
				else if(ntile)
				{
					verts_stripped[v1] = (ntile->height > vertex_heights[v1]);
					verts_stripped[v2] = (ntile->height > vertex_heights[v2]);
				}
				else
				{
					verts_stripped[v1] = true;
					verts_stripped[v2] = true;
				}

				ntile = GetTile(xvd[i], yvd[i]);//GetTile_CheckFlag(xvd[i], yvd[i], EHTILE_DETACHED, isDetached);

				// затем по угловым соседям
				if (ntile && ntile->texture != -1 && ((ntile->flags & EHTILE_DETACHED) > 0) == isDetached)
				{
					if( ntile->height > vertex_heights[i] )
						vertex_heights[i] = ntile->height;
				}
				else if(ntile)
					verts_stripped[i] = (ntile->height > vertex_heights[i]);
				else
					verts_stripped[i] = true;
			}

			// определяем, нужна ли стена
			for(int i = 0; i < 4; i++)
			{
				int i1, i2;

				i1 = valid_edge_index(i-1);
				i2 = valid_edge_index(i+1);

				int edge_ngb[] = {i1, i2};

				hfieldtile_t* ntile = GetTile_CheckFlag(xv[i], yv[i], EHTILE_DETACHED, !isDetached);

				if(ntile && isDetached != ((ntile->flags & EHTILE_DETACHED) > 0) /*&& ntile->height < vertex_heights[i]*/)
				{
					edges_stripped[i] = true;
					edges_wall[i] = addWallOnEdges;
					edge_stripped_height[i] = ntile->height;

					for(int j = 0; j < 2; j++)
					{
						hfieldtile_t* ntile2 = GetTile_CheckFlag(xv[edge_ngb[j]], yv[edge_ngb[j]], EHTILE_DETACHED, !isDetached);

						// тут можно не спрашивать о разнице в высоте (хотя по-сути как-то нужно)
						if(ntile2 && isDetached != ((ntile2->flags & EHTILE_DETACHED) > 0))
						{
							edges_stripped[edge_ngb[j]] = true;
							edges_wall[i] = addWallOnEdges;
						}
					}
				}
			}

			// генерируем уже вершины

			float dxv[4] = NEIGHBOR_OFFS_DX(float(x), 0.5f);
			float dyv[4] = NEIGHBOR_OFFS_DY(float(y), 0.5f);
			float drxv[4] = NEIGHBOR_OFFS_DX(0.0f, 0.5f);
			float dryv[4] = NEIGHBOR_OFFS_DY(0.0f, 0.5f);

			int vindxs[4];

			if(!isEmpty || addWallOnEdges)
			{
				for(int i = 0; i < 4; i++)
				{
					Vector3D point_position(dxv[i] * HFIELD_POINT_SIZE, float(vertex_heights[i])*HFIELD_HEIGHT_STEP, dyv[i] * HFIELD_POINT_SIZE);

					int rIndex = rotatable ? (i + point.rotatetex) : i;

					if(rIndex > 3)
						rIndex -= 4;

					float tc_x = 0;
					float tc_y = 0;

					if( batchAtlas )
					{
						TexAtlasEntry_t* atlEntry = batchAtlas->GetEntry(point.atlasIdx);

						Vector2D size = atlEntry->rect.GetSize();
						Vector2D center = atlEntry->rect.GetCenter();

						Vector2D tcd(drxv[rIndex],dryv[rIndex]);

						tcd = center + tcd*size;

						tc_x = tcd.x + fTexelX*0.5f;
						tc_y = tcd.y + fTexelY*0.5f;
					}
					else
					{
						if(rotatable)
						{
							tc_x = (drxv[rIndex] + 0.5f) + fTexelX*0.5f;
							tc_y = (dryv[rIndex] + 0.5f) + fTexelY*0.5f;
						}
						else
						{
							tc_x = dxv[rIndex] + 0.5f;
							tc_y = dyv[rIndex] + 0.5f;
						}
					}

					Vector2D texCoord = Vector2D(tc_x,tc_y);

					hfielddrawvertex_t vert(point_position + hfield_offset, Vector3D(0.0f, 1.0f, 0.0f), texCoord);
					
					if (verts_stripped[i])
						vert.border = 0.0f;

					vindxs[i] = batch->verts.addUnique(vert,hfieldVertexComparator);
				}

				if(!isEmpty)
				{

					//Vector3D norm1 = NormalOfTriangle(	batch->verts[vindxs[2]].position, 
					//									batch->verts[vindxs[1]].position,
					//									batch->verts[vindxs[0]].position);

					if(generate_render)
					{
						Vector3D t,b,n;
						GetTileTBN( x, y, t,b,n );

						batch->verts[vindxs[0]].normal = n;
						batch->verts[vindxs[1]].normal = n;
						batch->verts[vindxs[2]].normal = n;
						batch->verts[vindxs[3]].normal = n;
					}

					// add quad
					batch->indices.append(vindxs[2]);
					batch->indices.append(vindxs[1]);
					batch->indices.append(vindxs[0]);

					batch->indices.append(vindxs[3]);
					batch->indices.append(vindxs[2]);
					batch->indices.append(vindxs[0]);
				}

				for(int i = 0; i < 4; i++)
				{
					int txv[4] = NEIGHBOR_OFFS_X(0);
					int tyv[4] = NEIGHBOR_OFFS_Y(0);

					int eindxs[4] = {-1,-1,-1,-1};

					if( edges_stripped[i] && edges_wall[i] )
					{
						int v1, v2;
						EdgeIndexToVertex(i, v1, v2);

						if(generate_render)
						{
							eindxs[0] = batch->verts.append( batch->verts[vindxs[v1]] );
							eindxs[1] = batch->verts.append( batch->verts[vindxs[v2]] );
						}
						else
						{
							eindxs[0] = vindxs[v1];
							eindxs[1] = vindxs[v2];
						}

						Vector3D point_position1(dxv[v1] * HFIELD_POINT_SIZE, float(edge_stripped_height[i])*HFIELD_HEIGHT_STEP, dyv[v1] * HFIELD_POINT_SIZE);
						Vector3D point_position2(dxv[v2] * HFIELD_POINT_SIZE, float(edge_stripped_height[i])*HFIELD_HEIGHT_STEP, dyv[v2] * HFIELD_POINT_SIZE);

						float fTexY1 = (batch->verts[vindxs[v1]].position.y-point_position1.y) / HFIELD_POINT_SIZE;//point_position1.y / HFIELD_POINT_SIZE;
						float fTexY2 = (batch->verts[vindxs[v2]].position.y-point_position2.y) / HFIELD_POINT_SIZE;//point_position2.y / HFIELD_POINT_SIZE;

						int rIndex = rotatable ? (i + point.rotatetex) : i;

						if(rIndex > 3)
							rIndex -= 4;

						int tv1, tv2;
						EdgeIndexToVertex(rIndex, tv1, tv2);

						// edge direction by texcoord
						Vector2D edgeTexDir(txv[rIndex], tyv[rIndex]);

						Vector2D texCoord1, texCoord2;

						if(rotatable)
						{
							texCoord1 = Vector2D(drxv[tv1]+0.5f, dryv[tv1]+0.5f) + edgeTexDir*fTexY1 + fTexelX*0.5f;
							texCoord2 = Vector2D(drxv[tv2]+0.5f, dryv[tv2]+0.5f) + edgeTexDir*fTexY2 + fTexelY*0.5f;
						}
						else
						{
							texCoord1 = Vector2D(dxv[tv1]+0.5f, dyv[tv1]+0.5f) + edgeTexDir*fTexY1 + fTexelX*0.5f;
							texCoord2 = Vector2D(dxv[tv2]+0.5f, dyv[tv2]+0.5f) + edgeTexDir*fTexY2 + fTexelY*0.5f;
						}



						hfielddrawvertex_t vert1 = hfielddrawvertex_t(point_position2 + hfield_offset, Vector3D(0, 1, 0), texCoord2);
						hfielddrawvertex_t vert2 =	hfielddrawvertex_t(point_position1 + hfield_offset, Vector3D(0, 1, 0), texCoord1);

						vert1.border = 0.0f;
						vert2.border = 0.0f;

						eindxs[2] = batch->verts.append(vert1);
						eindxs[3] = batch->verts.append(vert2);

						Vector3D norm1;

						float fCheckDegenerareArea1 = TriangleArea(	batch->verts[eindxs[2]].position,
																	batch->verts[eindxs[1]].position,
																	batch->verts[eindxs[0]].position);
						// invert normal to make it good
						if( fCheckDegenerareArea1 > 0.001f )
							norm1 = NormalOfTriangle(	batch->verts[eindxs[2]].position, 
														batch->verts[eindxs[1]].position, 
														batch->verts[eindxs[0]].position);
						else
							norm1 = NormalOfTriangle(	batch->verts[eindxs[3]].position,
														batch->verts[eindxs[2]].position,
														batch->verts[eindxs[0]].position);

						batch->verts[eindxs[0]].normal = norm1;
						batch->verts[eindxs[1]].normal = norm1;
						batch->verts[eindxs[2]].normal = norm1;
						batch->verts[eindxs[3]].normal = norm1;

						// add quad
						batch->indices.append(eindxs[2]);
						batch->indices.append(eindxs[1]);
						batch->indices.append(eindxs[0]);

						batch->indices.append(eindxs[3]);
						batch->indices.append(eindxs[2]);
						batch->indices.append(eindxs[0]);
					}
				}
			}
		}
	}
}

inline void ListLine(const Vector3D &from, const Vector3D &to, DkList<Vertex3D_t> &verts)
{
	verts.append(Vertex3D_t(from, vec2_zero));
	verts.append(Vertex3D_t(to, vec2_zero));
}

void ListQuad(const Vector3D &v1, const Vector3D &v2, const Vector3D& v3, const Vector3D& v4, const ColorRGBA &color, DkList<Vertex3D_t> &verts)
{
	verts.append(Vertex3D_t(v3, vec2_zero, color));
	verts.append(Vertex3D_t(v2, vec2_zero, color));
	verts.append(Vertex3D_t(v1, vec2_zero, color));

	verts.append(Vertex3D_t(v4, vec2_zero, color));
	verts.append(Vertex3D_t(v3, vec2_zero, color));
	verts.append(Vertex3D_t(v1, vec2_zero, color));
}

void DrawGridH(int size, int count, const Vector3D& pos, const ColorRGBA &color, bool for2D)
{
	int grid_lines = count;

	DkList<Vertex3D_t> grid_vertices(grid_lines / size);

	for(int i = 0; i <= grid_lines / size;i++)
	{
		int max_grid_size = grid_lines;
		int grid_step = size*i;

		ListLine(pos + Vector3D(0,0,grid_step),pos + Vector3D(max_grid_size,0,grid_step), grid_vertices);
		ListLine(pos + Vector3D(grid_step,0,0),pos + Vector3D(grid_step,0,max_grid_size), grid_vertices);

		ListLine(pos + Vector3D(0,0,-grid_step),pos + Vector3D(-max_grid_size,0,-grid_step), grid_vertices);
		ListLine(pos + Vector3D(-grid_step,0,0),pos + Vector3D(-grid_step,0,-max_grid_size), grid_vertices);

		// draw another part

		ListLine(pos + Vector3D(0,0,-grid_step),pos + Vector3D(max_grid_size,0,-grid_step), grid_vertices);
		ListLine(pos + Vector3D(-grid_step,0,0),pos + Vector3D(-grid_step,0,max_grid_size), grid_vertices);

		ListLine(pos + Vector3D(0,0,grid_step),pos + Vector3D(-max_grid_size,0,grid_step), grid_vertices);
		ListLine(pos + Vector3D(grid_step,0,0),pos + Vector3D(grid_step,0,-max_grid_size), grid_vertices);
	}

	DepthStencilStateParams_t depth;

	depth.depthTest = !for2D;
	depth.depthWrite = !for2D;
	depth.depthFunc = COMP_LEQUAL;

	RasterizerStateParams_t raster;

	raster.cullMode = CULL_BACK;
	raster.fillMode = FILL_SOLID;
	raster.multiSample = true;
	raster.scissor = false;

	materials->DrawPrimitivesFFP(PRIM_LINES, grid_vertices.ptr(), grid_vertices.numElem(), NULL, color, NULL, &depth, &raster);
}

void CHeightTileField::DebugRender(bool bDrawTiles, float gridHeight)
{
	if(!m_sizew || !m_sizeh)
		return;

	Vector3D halfsize = Vector3D(HFIELD_POINT_SIZE, 0, HFIELD_POINT_SIZE)*0.5f;
	DrawGridH(HFIELD_POINT_SIZE, m_sizew*2, m_position + Vector3D(m_sizew*HFIELD_POINT_SIZE*0.5f, gridHeight, m_sizew*HFIELD_POINT_SIZE*0.5f) - halfsize, ColorRGBA(1,1,1,0.1), false);

	if(!bDrawTiles)
		return;

	DkList<Vertex3D_t> tile_verts(64);
	tile_verts.resize(m_sizew*m_sizeh*6);

	for(int x = 0; x < m_sizew; x++)
	{
		for(int y = 0; y < m_sizeh; y++)
		{
			float dxv[4] = NEIGHBOR_OFFS_DX(x, 0.5);
			float dyv[4] = NEIGHBOR_OFFS_DY(y, 0.5);

			int pt_idx = y*m_sizew + x;
			hfieldtile_t& tile = m_points[pt_idx];

			//int vindxs[4];

			Vector3D p1(dxv[0] * HFIELD_POINT_SIZE, float(tile.height)*HFIELD_HEIGHT_STEP+0.1f, dyv[0] * HFIELD_POINT_SIZE);
			Vector3D p2(dxv[1] * HFIELD_POINT_SIZE, float(tile.height)*HFIELD_HEIGHT_STEP+0.1f, dyv[1] * HFIELD_POINT_SIZE);
			Vector3D p3(dxv[2] * HFIELD_POINT_SIZE, float(tile.height)*HFIELD_HEIGHT_STEP+0.1f, dyv[2] * HFIELD_POINT_SIZE);
			Vector3D p4(dxv[3] * HFIELD_POINT_SIZE, float(tile.height)*HFIELD_HEIGHT_STEP+0.1f, dyv[3] * HFIELD_POINT_SIZE);

			p1 += m_position;
			p2 += m_position;
			p3 += m_position;
			p4 += m_position;

			ColorRGBA tileColor(0,0,0,0.1);

			if(tile.texture != -1)
			{
				tileColor.x = (tile.flags & EHTILE_DETACHED) > 0 ? 0.0f : 1.0f;
				tileColor.y = (tile.flags & EHTILE_ADDWALL) > 0 ? ((tile.flags & EHTILE_COLLIDE_WALL) ? 0.25 : 0.0f) : 1.0f;
				tileColor.z = (tile.flags & EHTILE_NOCOLLIDE) > 0 ? 0.0f : 1.0f;
				tileColor.w = 0.5f;

				tileColor = color4_white-tileColor;
			}

			ListQuad(p1, p2, p3, p4, tileColor, tile_verts);
		}
	}

	DepthStencilStateParams_t depth;

	depth.depthTest = true;
	depth.depthWrite = false;
	depth.depthFunc = COMP_LEQUAL;

	BlendStateParam_t blend;

	blend.blendEnable = true;
	blend.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blend.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	RasterizerStateParams_t raster;

	raster.cullMode = CULL_BACK;
	raster.fillMode = FILL_SOLID;
	raster.multiSample = true;
	raster.scissor = false;

	materials->DrawPrimitivesFFP(PRIM_TRIANGLES, tile_verts.ptr(), tile_verts.numElem(), NULL, color4_white, &blend, &depth, &raster);
}

//-----------------------------------------------------------------------------

#ifdef EDITOR
CHeightTileFieldRenderable::CHeightTileFieldRenderable() : CHeightTileField(), CUndoableObject()
#else
CHeightTileFieldRenderable::CHeightTileFieldRenderable() : CHeightTileField()
#endif // EDITOR
{
	m_batches = NULL;
	m_numBatches = 0;

	m_format = NULL;
	m_vertexbuffer = NULL;
	m_indexbuffer = NULL;

	m_isChanged = true;

}

CHeightTileFieldRenderable::~CHeightTileFieldRenderable()
{
	CleanRenderData();
}

#ifdef EDITOR
bool CHeightTileFieldRenderable::Undoable_WriteObjectData( IVirtualStream* stream )
{
	WriteToStream(stream);

	return true;
}

void CHeightTileFieldRenderable::Undoable_ReadObjectData( IVirtualStream* stream )
{
	g_pShaderAPI->Reset(STATE_RESET_VBO);
	g_pShaderAPI->ApplyBuffers();

	Destroy();
	ReadFromStream( stream );
	m_isChanged = true;
	//GenereateRenderData();
}
#endif // EDITOR

void CHeightTileFieldRenderable::CleanRenderData(bool deleteVBO)
{
	delete [] m_batches;

	m_batches = NULL;

	m_numBatches = 0;

	if(deleteVBO)
	{
		if(m_format)
			g_pShaderAPI->DestroyVertexFormat(m_format);

		m_format = NULL;

		if(m_vertexbuffer)
			g_pShaderAPI->DestroyVertexBuffer(m_vertexbuffer);

		m_vertexbuffer = NULL;

		if(m_indexbuffer)
			g_pShaderAPI->DestroyIndexBuffer(m_indexbuffer);

		m_indexbuffer = NULL;
	}

	m_isChanged = true;
}

void CHeightTileFieldRenderable::GenereateRenderData()
{
	if(!m_isChanged)
		return;

	m_isChanged = false;

	// delete batches only
	CleanRenderData(false);

	DkList<hfieldbatch_t*> batches;

	// precache it first
	for(int i = 0; i < m_materials.numElem(); i++)
		materials->PutMaterialToLoadingQueue( m_materials[i]->material );

	// сгенерить, полученные батчи соединить и распределить по материалам
	Generate(true, batches);

	if(batches.numElem() == 0)
	{
		m_numBatches = 0;
		return;
	}

	m_batches = new hfielddrawbatch_t[batches.numElem()];
	DkList<hfielddrawvertex_t>	verts;
	DkList<int>					indices;

	for(int i = 0; i < batches.numElem(); i++)
	{
		m_batches[i].startVertex = verts.numElem();
		m_batches[i].numVerts = batches[i]->verts.numElem();

		m_batches[i].startIndex = indices.numElem();
		m_batches[i].numIndices = batches[i]->indices.numElem();
		m_batches[i].pMaterial = batches[i]->materialBundle->material;
		//m_batches[i].pMaterial->Ref_Grab();

		for(int j = 0; j < batches[i]->verts.numElem(); j++)
		{
			m_batches[i].bbox.AddVertex( batches[i]->verts[j].position );
			//verts.append(batches[i]->verts[j]);
		}

		verts.append(batches[i]->verts);

		indices.resize(indices.numElem() + batches[i]->indices.numElem());

		for(int j = 0; j < batches[i]->indices.numElem(); j++)
			indices.append(batches[i]->indices[j] + m_batches[i].startVertex);

		// that' all, folks
		delete batches[i];
	}

	m_numVerts = verts.numElem();

	m_numBatches = batches.numElem();

	if(!m_vertexbuffer || !m_indexbuffer || !m_format)
	{
		VertexFormatDesc_t pFormat[] = {
			{ 0, 3, VERTEXTYPE_VERTEX, ATTRIBUTEFORMAT_FLOAT },	  // position
			{ 0, 2, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // texcoord 0
			{ 0, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // Normal (TC1) + border
		};

		DevMsg(2,"Creating hfield buffers, %d verts %d indices in %d batches\n", verts.numElem(), indices.numElem(), m_numBatches);

		if(!m_format)
			m_format = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));

#ifdef EDITOR
		BufferAccessType_e bufferType = BUFFER_STATIC;

		int vb_lock_size = m_sizew*m_sizeh*12;
		int ib_lock_size = m_sizew*m_sizeh*16;

		m_vertexbuffer = g_pShaderAPI->CreateVertexBuffer(bufferType, vb_lock_size, sizeof(hfielddrawvertex_t), NULL);
		m_indexbuffer = g_pShaderAPI->CreateIndexBuffer(ib_lock_size, sizeof(int), bufferType, NULL);
#else
		BufferAccessType_e bufferType = BUFFER_STATIC;
		int vb_lock_size = verts.numElem();
		int ib_lock_size = indices.numElem();

		m_vertexbuffer = g_pShaderAPI->CreateVertexBuffer(bufferType, vb_lock_size, sizeof(hfielddrawvertex_t), verts.ptr());
		m_indexbuffer = g_pShaderAPI->CreateIndexBuffer(ib_lock_size, sizeof(int), bufferType, indices.ptr());

		return;
#endif
	}

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
	}
}

void CHeightTileFieldRenderable::Render(int nDrawFlags, const occludingFrustum_t& occlSet)
{
	//g_pShaderAPI->Reset();

	if(m_isChanged)
	{
#ifdef EDITOR
		g_pShaderAPI->Reset();
		g_pShaderAPI->ApplyBuffers();

		// regenerate again
		GenereateRenderData();
#endif // EDITOR

		m_isChanged = false;
	}

	if(m_batches == NULL)
		return;

	for(int i = 0; i < m_numBatches; i++)
	{
		if(!occlSet.IsBoxVisible(m_batches[i].bbox))
			continue;

		g_pGameWorld->ApplyLighting( m_batches[i].bbox );

		materials->SetMatrix(MATRIXMODE_WORLD, identity4());
		materials->SetCullMode((nDrawFlags & RFLAG_FLIP_VIEWPORT_X) ? CULL_FRONT : CULL_BACK);

		g_pShaderAPI->SetVertexFormat(m_format);
		g_pShaderAPI->SetVertexBuffer(m_vertexbuffer, 0);
		g_pShaderAPI->SetIndexBuffer(m_indexbuffer);

		materials->BindMaterial(m_batches[i].pMaterial, false);
		//debugoverlay->Box3D(m_batches[i].bbox.minPoint, m_batches[i].bbox.maxPoint, ColorRGBA(1,1,0,0.1));

		materials->Apply();

		g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, m_batches[i].startIndex, m_batches[i].numIndices, 0, m_numVerts);
	}
}
