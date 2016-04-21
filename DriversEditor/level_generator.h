//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate level generator
//////////////////////////////////////////////////////////////////////////////////

#ifndef LEVEL_GENERATOR
#define LEVEL_GENERATOR

#include "utils/eqstring.h"
#include "heightfield.h"

struct tileGenTexture_t
{
	IMaterial*			material;
	int					atlasIdx;
};

struct LevelGenParams_t
{
	LevelGenParams_t()
	{
		onlyRoadTextures = false;
		keepOldLevel = false;
		cellsPerRegion = 0;
	}

	~LevelGenParams_t()
	{
		for(int i = 0; i < atlases.numElem(); i++)
			delete atlases[i];
	}

	struct tileTextures_t
	{
		tileGenTexture_t normal;
		tileGenTexture_t normal_darken;

		tileGenTexture_t normal_faded;
		tileGenTexture_t normal_faded_inv;

		tileGenTexture_t normal_innercorner;
		tileGenTexture_t normal_outercorner;
		
		tileGenTexture_t line_solid;
		tileGenTexture_t line_shortdots;
		tileGenTexture_t line_longdots;
		tileGenTexture_t zebra;

		tileGenTexture_t line_solid_inv;
		tileGenTexture_t line_shortdots_inv;
		tileGenTexture_t line_longdots_inv;
		tileGenTexture_t zebra_inv;

		tileGenTexture_t sidewalk;
		tileGenTexture_t grass;
	} tiles;

	bool onlyRoadTextures;
	bool keepOldLevel;

	int cellsPerRegion;

	DkList<CTextureAtlas*> atlases;
};

void LoadTileTextureFile(const char* filename, LevelGenParams_t& params);

#endif // LEVEL_GENERATOR