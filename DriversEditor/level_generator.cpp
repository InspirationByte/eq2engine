//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate level generator
//////////////////////////////////////////////////////////////////////////////////

#include "materialsystem/IMaterialSystem.h"

#include "level.h"
#include "region.h"
#include "utils/strtools.h"

#define DEFAULT_ASPHALT_MATERIAL	"tools/default_road"
#define DEFAULT_SIDEWALK_MATERIAL	"tools/default_side"
#define DEFAULT_GRASS_MATERIAL		"tools/default_grass"

void LookupMaterial(const LevelGenParams_t& params, tileGenTexture_t& tex, kvkeybase_t* base)
{
	const char* matName = KV_GetValueString(base,0,"");
	const char* texName = base->name;

	if(!strchr(matName, CORRECT_PATH_SEPARATOR) && !strchr(matName, INCORRECT_PATH_SEPARATOR))
	{
		CTextureAtlas* atl = NULL;
		
		for(int i = 0; i < params.atlases.numElem(); i++)
		{
			if(!stricmp(params.atlases[i]->GetName(), matName))
			{
				atl = params.atlases[i];
				break;
			}
		}

		if(!atl)
		{
			MsgError("Can't find atlas named '%s'\n", matName);
			tex.material = NULL;
			tex.atlasIdx = 0;
			return;
		}

		tex.material = materials->FindMaterial( atl->GetMaterialName(), true);
		tex.atlasIdx = atl->FindEntryIndex(texName);

		// fail-safe
		if(tex.atlasIdx == -1)
		{	
			tex.atlasIdx = 0;
			MsgError("Can't find atlas enrty '%s'\n", texName);
		}
	}
	else
	{
		tex.material = materials->FindMaterial( matName, true );
		tex.atlasIdx = 0;
	}
}

void LoadTileTextureFile(const char* filename, LevelGenParams_t& params)
{
	KeyValues kvs;
	if(!kvs.LoadFromFile(filename, SP_ROOT))
	{
		params.tiles.grass.material = materials->FindMaterial(DEFAULT_GRASS_MATERIAL, true);
		params.tiles.normal.material = materials->FindMaterial(DEFAULT_ASPHALT_MATERIAL, true);
		params.tiles.sidewalk.material = materials->FindMaterial(DEFAULT_SIDEWALK_MATERIAL, true);
		params.tiles.normal_innercorner.material = materials->FindMaterial(DEFAULT_ASPHALT_MATERIAL, true);
		params.tiles.normal_faded.material = materials->FindMaterial(DEFAULT_ASPHALT_MATERIAL, true);
		params.tiles.normal_outercorner.material = materials->FindMaterial(DEFAULT_ASPHALT_MATERIAL, true);
		params.tiles.line_longdots.material = materials->FindMaterial(DEFAULT_ASPHALT_MATERIAL, true);
		params.tiles.zebra.material = materials->FindMaterial(DEFAULT_ASPHALT_MATERIAL, true);
		return;
	}

	for(int i = 0; i < kvs.GetRootSection()->keys.numElem(); i++)
	{
		kvkeybase_t* kvb = kvs.GetRootSection()->keys[i];

		if(!stricmp(kvb->name, "atlas"))
		{
			const char* name = KV_GetValueString(kvb,0, "");
			const char* path = KV_GetValueString(kvb,1, "");
			CTextureAtlas* atl = TexAtlas_LoadAtlas(("materials/"+_Es(path)+".atlas").c_str(), name, true);

			if(atl)
				params.atlases.append(atl);
		}
		else if(!stricmp(kvb->name, "textures"))
		{
			LookupMaterial(params, params.tiles.normal,kvb->FindKeyBase("normal"));

			LookupMaterial(params, params.tiles.normal_darken,kvb->FindKeyBase("normal_darken"));
			LookupMaterial(params, params.tiles.normal_faded,kvb->FindKeyBase("normal_faded"));
			LookupMaterial(params, params.tiles.normal_innercorner,kvb->FindKeyBase("normal_innercorner"));
			LookupMaterial(params, params.tiles.normal_outercorner,kvb->FindKeyBase("normal_outercorner"));

			LookupMaterial(params, params.tiles.line_solid,kvb->FindKeyBase("line_solid"));
			LookupMaterial(params, params.tiles.line_shortdots,kvb->FindKeyBase("line_shortdots"));
			LookupMaterial(params, params.tiles.line_longdots,kvb->FindKeyBase("line_longdots"));

			LookupMaterial(params, params.tiles.zebra,kvb->FindKeyBase("zebra"));

			LookupMaterial(params, params.tiles.line_solid_inv,kvb->FindKeyBase("line_solid_inv"));
			LookupMaterial(params, params.tiles.line_shortdots_inv,kvb->FindKeyBase("line_shortdots_inv"));
			LookupMaterial(params, params.tiles.line_longdots_inv,kvb->FindKeyBase("line_longdots_inv"));

			LookupMaterial(params, params.tiles.zebra_inv,kvb->FindKeyBase("zebra_inv"));

			LookupMaterial(params, params.tiles.sidewalk, kvb->FindKeyBase("sidewalk"));
			LookupMaterial(params, params.tiles.grass, kvb->FindKeyBase("grass"));
		}
	}
}

enum EMapImageFlags
{
	MAP_FLAG_JUNCTION			= (1 << 0),
	MAP_FLAG_STRAIGHT			= (1 << 1),

	MAP_FLAG_JUNCTION_MANUAL	= (1 << 2),	// manually textured junction
};

#define GTEX( name ) genParams.tiles.##name.material, genParams.tiles.##name.atlasIdx
#define GATL( name ) genParams.tiles.##name.atlasIdx

// channel indexes
#define GEN_TILE_GRASS		1
#define GEN_TILE_ROAD		0
#define GEN_TILE_JUNCTION	2

inline int PixelToArrayIndex(int x,int y,int w,int h)
{
	if(x < 0 || y < 0 || x >= w || y >= h)
		return -1;

	return (h-1-y)*w+x;
}

bool CGameLevel::Ed_GenerateMap( LevelGenParams_t& genParams, const CImage* img )
{
	int iw = 0;
	int ih = 0;

	TVec3D<ubyte>* pixels = NULL;

	if(!genParams.keepOldLevel)
	{
		ASSERTMSG(genParams.cellsPerRegion >= 32, "Programmer error! cellsPerRegion must be greater than or equal 32");
		ASSERTMSG(img, "Programmer error! NULL image input for Ed_GenerateMap");

		iw = img->GetWidth();
		ih = img->GetHeight();

		int regW = iw / genParams.cellsPerRegion;
		int regH = ih / genParams.cellsPerRegion;

		regW += 2;
		regH += 2;

		if(regW & (0x1))
			regW++;

		if(regH & (0x1))
			regH++;

		pixels = (TVec3D<ubyte>*)img->GetPixels();

		Init(regW, regH, genParams.cellsPerRegion, true);

		Msg("Generating full map from '%s'...\n", img->GetName());

		// this check is not needed anymore
		//if(iw > m_wide*m_cellsSize || ih > m_tall*m_cellsSize)

	}
	else
	{
		// use existing map just to regenerate old one
		iw = m_wide * m_cellsSize;
		ih = m_tall * m_cellsSize;

		pixels = new TVec3D<ubyte>[iw*ih];
	}

	ubyte* pixelFlagMap = new ubyte[iw*ih];
	memset(pixelFlagMap, 0, iw*ih);

	// world center
	IVector2D worldCenter( m_wide*m_cellsSize/2, m_tall*m_cellsSize/2);
	IVector2D imgCenter( iw/2, ih/2);

	IVector2D imgOffsetOnWorld = worldCenter-imgCenter;

	int rdir_x[] = ROADNEIGHBOUR_OFFS_X(0);
	int rdir_y[] = ROADNEIGHBOUR_OFFS_Y(0);

	// copy road map to image
	if(genParams.keepOldLevel)
	{
		for(int x = 0; x < iw; x++)
		{
			for(int y = 0; y < ih; y++)
			{
				int pixIdx = PixelToArrayIndex(x,y,iw,ih);

				// convert to region-position
				CLevelRegion* reg = NULL;
				IVector2D localTilePos;
				GlobalToLocalPoint( imgOffsetOnWorld+IVector2D(x,y), localTilePos, &reg);

				if(!reg)
					continue;

				int tileIdx = localTilePos.y*m_cellsSize+localTilePos.x;
				if(reg->m_roads[tileIdx].type != ROADTYPE_NOROAD)
				{
					pixels[pixIdx][GEN_TILE_ROAD] = 255;
					reg->GetHField()->SetChanged();
				}
				else
					pixels[pixIdx][GEN_TILE_GRASS] = 255;
			}
		}
	}

	// first pass: make everything
	for(int x = 0; x < iw; x++)
	{
		for(int y = 0; y < ih; y++)
		{
			int pixIdx = PixelToArrayIndex(x,y,iw,ih);

			// empty? Don't put tiles
			if(pixels[pixIdx][GEN_TILE_GRASS] == 0 && pixels[pixIdx][GEN_TILE_ROAD] == 0 && pixels[pixIdx][GEN_TILE_JUNCTION] == 0)
				continue;

			// convert to region-position
			CLevelRegion* reg = NULL;
			IVector2D localTilePos;
			GlobalToLocalPoint( imgOffsetOnWorld+IVector2D(x,y), localTilePos, &reg);

			int tileIdx = localTilePos.y*m_cellsSize+localTilePos.x;

			bool putSidewalk = false;
			
			int off_dx[] = NEIGHBOR_OFFS_XDX(x,1);
			int off_dy[] = NEIGHBOR_OFFS_YDY(y,1);

			int roff_x[] = ROADNEIGHBOUR_OFFS_X(x);
			int roff_y[] = ROADNEIGHBOUR_OFFS_Y(y);
			
			// check if this tile is best to put sidewalk
			if(pixels[pixIdx][GEN_TILE_GRASS] > 0)
			{
				for(int i = 0; i < 8; i++)
				{
					int npixIdx = PixelToArrayIndex(off_dx[i],off_dy[i],iw,ih);

					if(npixIdx == -1)
						continue;

					if( pixels[npixIdx][GEN_TILE_ROAD] > 0 || pixels[npixIdx][GEN_TILE_JUNCTION] > 0 )
					{
						putSidewalk = true;
						break;
					}
				}
			}
			else if(pixels[pixIdx][GEN_TILE_ROAD] > 0) // check if this tile wants to be straight
			{
				int tileDirection = -1;

				for(int i = 0; i < 4; i++)
				{
					int npixIdx = PixelToArrayIndex(roff_x[i],roff_y[i],iw,ih);

					if(npixIdx == -1)
						continue;

					if( pixels[npixIdx][GEN_TILE_GRASS] > 0 )
					{
						tileDirection = i;
						break;
					}
				}

				// if road tile detected
				if(tileDirection != -1)
				{
					int widthCheckDir = tileDirection;

					bool gotEndSide = false;

					// trace to opposite side to get width
					int roadWidth = 0;
					for(; roadWidth < 16; roadWidth++)
					{
						int trdx = x - rdir_x[widthCheckDir]*roadWidth;
						int trdy = y - rdir_y[widthCheckDir]*roadWidth;

						int npixIdx = PixelToArrayIndex(trdx,trdy,iw,ih);

						// empty? Don't put tiles
						if(pixels[npixIdx][GEN_TILE_ROAD] == 0)
						{
							gotEndSide = true;
							break;
						}
					}

					// not goes to infinity :D
					if(roadWidth < 16 && gotEndSide)
					{
						int halfWidth = roadWidth / 2;
					
						// make road
						tileDirection -= 1;

						if(tileDirection < 0)
							tileDirection += 4;

						reg->m_roads[tileIdx].direction = tileDirection;
						reg->m_roads[tileIdx].type = ROADTYPE_STRAIGHT;

						// mark as straight
						pixelFlagMap[pixIdx] = MAP_FLAG_STRAIGHT;

						bool isOddCount = halfWidth & 0x1;

						// mark other road tiles
						for(int i = 0; i < halfWidth; i++)
						{
							int trdx = x-rdir_x[widthCheckDir]*i;
							int trdy = y-rdir_y[widthCheckDir]*i;

							int wpixIdx = PixelToArrayIndex(trdx,trdy,iw,ih);

							// convert to region-position
							CLevelRegion* roadReg = NULL;
							IVector2D roadWTilePos;
							GlobalToLocalPoint( imgOffsetOnWorld+IVector2D(trdx,trdy), roadWTilePos, &roadReg);

							int rTileIdx = roadWTilePos.y*m_cellsSize+roadWTilePos.x;

							roadReg->m_roads[rTileIdx].direction = tileDirection;
							roadReg->m_roads[rTileIdx].type = ROADTYPE_STRAIGHT;

							pixelFlagMap[wpixIdx] = MAP_FLAG_STRAIGHT;

							int rotation = tileDirection + 1;

							while(rotation > 4)
								rotation -= 4;
	
							if(i == halfWidth-1)
							{
								roadReg->GetHField()->SetPointMaterial( roadWTilePos.x,roadWTilePos.y, GTEX(line_solid) );
							}
							else
							{
								rotation +=2;

								while(rotation > 4)
									rotation -= 4;

								if((i%2) == isOddCount)
									roadReg->GetHField()->SetPointMaterial( roadWTilePos.x,roadWTilePos.y, GTEX(line_longdots) );
								else
									roadReg->GetHField()->SetPointMaterial( roadWTilePos.x,roadWTilePos.y, GTEX(line_longdots_inv) );
							}

							roadReg->GetHField()->GetTile(roadWTilePos.x,roadWTilePos.y)->rotatetex = rotation;
						}
					}
					else
					{
						pixelFlagMap[pixIdx] = MAP_FLAG_JUNCTION;
					}
				}
				else if(pixelFlagMap[pixIdx] == 0)
				{
					pixelFlagMap[pixIdx] = MAP_FLAG_JUNCTION;
				}
			}

			// road tiles
			if( !genParams.onlyRoadTextures && pixels[pixIdx][GEN_TILE_GRASS] > 0)
			{
				// grass or sidewalk tiles
				if( putSidewalk )
				{
					reg->GetHField()->SetPointMaterial(localTilePos.x,localTilePos.y, GTEX(sidewalk));
					hfieldtile_t* tile = reg->GetHField()->GetTile(localTilePos.x,localTilePos.y);
					tile->flags |= EHTILE_DETACHED | EHTILE_ADDWALL;
					tile->height += 1;
				}
				else
					reg->GetHField()->SetPointMaterial(localTilePos.x,localTilePos.y, GTEX(grass));
			}
		}
	}
	
	// remove damn short roads
	for(int x = 0; x < iw; x++)
	{
		for(int y = 0; y < ih; y++)
		{
			int pixIdx = PixelToArrayIndex(x,y,iw,ih);

			// convert to region-position
			CLevelRegion* reg = NULL;
			IVector2D localTilePos;
			GlobalToLocalPoint( imgOffsetOnWorld+IVector2D(x,y), localTilePos, &reg);

			int tileIdx = localTilePos.y*m_cellsSize+localTilePos.x;

			if(pixelFlagMap[pixIdx] == MAP_FLAG_STRAIGHT)
			{
				int tileDir = reg->m_roads[tileIdx].direction;

				// check road length for 1 cell
				int fdx = x + rdir_x[tileDir];
				int fdy = y + rdir_y[tileDir];

				int bdx = x - rdir_x[tileDir];
				int bdy = y - rdir_y[tileDir];

				int fwdPixIdx = PixelToArrayIndex(fdx,fdy,iw,ih);
				int bckPixIdx = PixelToArrayIndex(bdx,bdy,iw,ih);

				if( !(pixelFlagMap[fwdPixIdx] == MAP_FLAG_STRAIGHT) && !(pixelFlagMap[bckPixIdx] == MAP_FLAG_STRAIGHT) )
				{
					pixelFlagMap[pixIdx] = MAP_FLAG_JUNCTION;
				}
			}
		}
	}

	// put zebra on the straights and grow the junction
	for(int x = 0; x < iw; x++)
	{
		for(int y = 0; y < ih; y++)
		{
			int pixIdx = PixelToArrayIndex(x,y,iw,ih);

			// empty? Don't put tiles
			if(pixels[pixIdx][GEN_TILE_GRASS] == 0 && pixels[pixIdx][GEN_TILE_ROAD] == 0 && pixels[pixIdx][GEN_TILE_JUNCTION] == 0)
				continue;

			// now processing junctions
			if(pixelFlagMap[pixIdx] == MAP_FLAG_JUNCTION)
			{
				// look for neighbours
				// find a straight
				for(int i = 0; i < 4; i++)
				{
					int cx = x+rdir_x[i];
					int cy = y+rdir_y[i];

					int cpixIdx = PixelToArrayIndex(cx,cy,iw,ih);

					// we have straight, start from it
					if( pixelFlagMap[cpixIdx] != MAP_FLAG_STRAIGHT )
						continue;

					for(int s = 1; s < 4; s++)
					{
						int nx = x+rdir_x[i]*s;
						int ny = y+rdir_y[i]*s;

						int npixIdx = PixelToArrayIndex(nx,ny,iw,ih);

						// convert to region-position
						CLevelRegion* nreg = NULL;
						IVector2D nTilePos;
						GlobalToLocalPoint( imgOffsetOnWorld+IVector2D(nx,ny), nTilePos, &nreg);

						int nTileIdx = nTilePos.y*m_cellsSize+nTilePos.x;
						if(!nreg->m_roads)
							break;

						// outgoing or incoming are accepted directions
						if((i%2) != (nreg->m_roads[nTileIdx].direction%2))
							break;


						hfieldtile_t* ntile = nreg->GetHField()->GetTile(nTilePos.x,nTilePos.y);

						bool isInverted =	(ntile->atlasIdx == GATL(line_longdots_inv)) ||
											(ntile->atlasIdx == GATL(line_shortdots_inv));

						if(s <= 2)
							pixelFlagMap[npixIdx] |= MAP_FLAG_JUNCTION;

						if(s == 2)
						{
							if(isInverted)
								nreg->GetHField()->SetPointMaterial(nTilePos.x,nTilePos.y, GTEX(zebra_inv));
							else
								nreg->GetHField()->SetPointMaterial(nTilePos.x,nTilePos.y, GTEX(zebra));

							pixelFlagMap[npixIdx] |= MAP_FLAG_JUNCTION_MANUAL;
						}
						else
						{
							nreg->GetHField()->SetPointMaterial(nTilePos.x,nTilePos.y, GTEX(normal_faded));

							if(isInverted)
								ntile->rotatetex += 2;

							while(ntile->rotatetex > 4)
								ntile->rotatetex -= 4;

							pixelFlagMap[npixIdx] |= MAP_FLAG_JUNCTION_MANUAL;
						}
					}
				}
			}
		}
	}

	// make junction painted correct
	for(int x = 0; x < iw; x++)
	{
		for(int y = 0; y < ih; y++)
		{
			int pixIdx = PixelToArrayIndex(x,y,iw,ih);

			// empty? Don't put tiles
			if(pixels[pixIdx][GEN_TILE_GRASS] == 0 && pixels[pixIdx][GEN_TILE_ROAD] == 0 && pixels[pixIdx][GEN_TILE_JUNCTION] == 0)
				continue;

			// convert to region-position
			CLevelRegion* reg = NULL;
			IVector2D localTilePos;
			GlobalToLocalPoint( imgOffsetOnWorld+IVector2D(x,y), localTilePos, &reg);

			int tileIdx = localTilePos.y*m_cellsSize+localTilePos.x;

			int roff_x[] = ROADNEIGHBOUR_OFFS_X(0);
			int roff_y[] = ROADNEIGHBOUR_OFFS_Y(0);

			if((pixelFlagMap[pixIdx] & MAP_FLAG_JUNCTION))
			{
				reg->m_roads[tileIdx].type = ROADTYPE_JUNCTION;

				if(!(pixelFlagMap[pixIdx] & MAP_FLAG_JUNCTION_MANUAL))
					reg->GetHField()->SetPointMaterial( localTilePos.x,localTilePos.y, GTEX(normal_darken) );
			}
		}
	}

	if(genParams.keepOldLevel)
		delete [] pixels;

	delete [] pixelFlagMap;

	return true;
}
