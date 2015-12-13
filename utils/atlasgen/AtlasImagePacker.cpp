//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas packer - main code
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"

#include "imaging/ImageLoader.h"
#include "imaging/PixWriter.h"

#include "utils/RectanglePacker.h"

unsigned long upper_power_of_two(unsigned long v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

enum EPaddingMode
{
	PAD_NONE = 0,
	PAD_CLAMP,
	PAD_REPEAT,
	PAD_MIRROR,
};

#define ROLLING_VALUE(x, max) ((x < 0) ? max+x : ((x >= max) ? x-max : x ))

void CopyPixels(CImage* pSrc, ubyte* pDst, int dst_x, int dst_y, int dst_wide, int padding, EPaddingMode padMode)
{
	ubyte* pSrcData = pSrc->GetPixels(0,0);

	int src_w = pSrc->GetWidth(0),
		src_h = pSrc->GetHeight(0);

	for(int x = -padding; x < src_w + padding; x++)
	{
		for(int y = -padding; y < src_h + padding; y++)
		{
			int nDestStride = ((dst_x+x) + (dst_y+y)*dst_wide) * GetChannelCount(FORMAT_RGBA8);
			int nSrcStride = 0;

			if(x < 0 || y < 0 || x >= src_w || y >= src_h)
			{
				switch(padMode)
				{
					case PAD_CLAMP:
					{
						int nx = clamp(x, 0, src_w-1);
						int ny = clamp(y, 0, src_h-1);
						nSrcStride = (nx + ny*src_w) * GetChannelCount(FORMAT_RGBA8);
						break;
					}
					case PAD_REPEAT:
					{
						int nx = ROLLING_VALUE(x, src_w);
						int ny = ROLLING_VALUE(y, src_h);
						nSrcStride = (nx + ny*src_w) * GetChannelCount(FORMAT_RGBA8);
						break;
					}
					case PAD_MIRROR:
					{
						int nx = abs(x);
						int ny = abs(y);

						if(nx >= src_w)
							nx = nx-src_w;

						if(ny >= src_h)
							ny = ny-src_h;

						nSrcStride = (nx + ny*src_w) * GetChannelCount(FORMAT_RGBA8);
						break;
					}
					default:
						continue;	// skip padding
				}
			}
			else
				nSrcStride = (x + y*src_w) * GetChannelCount(FORMAT_RGBA8);

			ASSERT(nDestStride >= 0);
			ASSERT(nSrcStride >= 0);

			// copy channels individually
			pDst[nDestStride]	= pSrcData[nSrcStride];
			pDst[nDestStride+1] = pSrcData[nSrcStride+1];
			pDst[nDestStride+2] = pSrcData[nSrcStride+2];
			pDst[nDestStride+3] = pSrcData[nSrcStride+3];
		}
	}
}

inline int AtlasPackComparison(PackerRectangle *const &elem0, PackerRectangle *const &elem1)
{
	return (elem1->width + elem1->height) - (elem0->width + elem0->height);
}

bool CreateAtlasImage(const DkList<CImage*>& images_list, 
						const char* pszOutputImageName, 
						kvkeybase_t* pParams)
{
	int padding = KV_GetValueInt(pParams->FindKeyBase("padding"), 0, 2);
	EPaddingMode padMode = PAD_NONE;

	const char* padModeStr = KV_GetValueString(pParams->FindKeyBase("padding"), 1, "none");
	if(!stricmp(padModeStr, "clamp"))
	{
		padMode = PAD_CLAMP;
	}
	else if(!stricmp(padModeStr, "repeat"))
	{
		padMode = PAD_REPEAT;
	}
	else if(!stricmp(padModeStr, "mirror"))
	{
		padMode = PAD_MIRROR;
	}

	CRectanglePacker packer;
	packer.SetPackPadding( padding );	// we set double padding here

	// add
	for(int i = 0; i < images_list.numElem(); i++)
	{
		CImage* pImg = images_list[i];

		bool bAccept = true;

		if(pImg->GetFormat() != FORMAT_RGBA8)
			bAccept = pImg->Convert(FORMAT_RGBA8);

		// add rectangle if it has same format
		if(bAccept)
		{
			Msg("Adding image '%s' (%d %d)\n", pImg->GetName(), pImg->GetWidth(), pImg->GetHeight());
			packer.AddRectangle( pImg->GetWidth(), pImg->GetHeight(), pImg);
		}
	}

	float	wide = 512,
			tall = 512;

	EqString shader = "BaseParticle";
	EqString shader_mode = "translucent";

	kvkeybase_t* pSizeKey = pParams->FindKeyBase("size");

	if(pSizeKey)
	{
		wide = KV_GetValueInt(pSizeKey, 0, 512);
		tall = KV_GetValueInt(pSizeKey, 1, 512);
	}

	kvkeybase_t* shaderBase = pParams->FindKeyBase("shader");
	shader = KV_GetValueString(shaderBase, 0, "BaseParticle");

	// pack
	if(!packer.AssignCoords(wide, tall, AtlasPackComparison))
	{
		MsgError("Couldn't assign coordinates, too small primary size!!!\n");
		return false;
	}

	Msg("Trying to generate atlas for '%s' (%g %g)...\n", pszOutputImageName, wide, tall);

	wide = upper_power_of_two(wide);
	tall = upper_power_of_two(tall);

	// create new image
	CImage img;
	ubyte* pData = img.Create(FORMAT_RGBA8, wide, tall, 1, 1);

	ASSERT(pData);

	EqString file_name = _Es(pszOutputImageName).Path_Strip_Ext();
	EqString mat_file_name = file_name + ".mat";

	// save atlas info
	KeyValues kvs;
	kvkeybase_t* pAtlasGroup = kvs.GetRootSection()->AddKeyBase("atlasgroup", file_name.GetData());

	KeyValues kv_material;
	kvkeybase_t* pShaderEntry = kv_material.GetRootSection()->AddKeyBase(shader.GetData());

	pShaderEntry->AddKeyBase("BaseTexture", file_name.GetData());
	pShaderEntry->MergeFrom(shaderBase, true);


	// copy pixels
	for(int i = 0; i < packer.GetRectangleCount(); i++)
	{
		PackerRectangle* pRect = packer.GetRectangle(i);
		CImage* pImage = (CImage*)pRect->userdata;

		// rgba8 is pretty simple
		CopyPixels(pImage, pData, pRect->x, pRect->y, wide, padding, padMode);

		// get file name only
		char img_file_name_only[MAX_PATH];
		ExtractFileBase(pImage->GetName(), img_file_name_only);

		// add info to keyvalues
		kvkeybase_t* rect_kv = pAtlasGroup->AddKeyBase(img_file_name_only);
		rect_kv->SetValueByIndex(varargs("%g", pRect->x / wide), 0);
		rect_kv->SetValueByIndex(varargs("%g", pRect->y / tall), 1);
		rect_kv->SetValueByIndex(varargs("%g", (pRect->x+pRect->width) / wide), 2);
		rect_kv->SetValueByIndex(varargs("%g", (pRect->y+pRect->height) / tall), 3);

		// done with it
		delete pImage;
	}

	// save image as DDS, or TGA ???
	if(img.SaveImage(pszOutputImageName))
	{
		kvs.SaveToFile((file_name + ".atlas").GetData());
		kv_material.SaveToFile((file_name + ".mat").GetData());
	}
	else
	{
		MsgError("Error while saving '%s' atlas texture...\n",pszOutputImageName);
		return false;
	}

	return true;
}

void ProcessNewAtlas(const char* pszAtlasSrcName, const char* pszOutputName)
{
	DkList<CImage*> pImages;

	KeyValues kvs;
	if( kvs.LoadFromFile(pszAtlasSrcName) )
	{
		// try loading images
		for(int i = 0; i < kvs.GetRootSection()->keys.numElem(); i++)
		{
			if(!stricmp(kvs.GetRootSection()->keys[i]->name, "image"))
			{
				CImage* pImg = new CImage();

				EqString image_name = KV_GetValueString(kvs.GetRootSection()->keys[i], 0, "no_image");
				EqString atlas_dir = _Es(pszAtlasSrcName).Path_Strip_Name();

				bool isOk = pImg->LoadTGA((atlas_dir + "/" + image_name).GetData());

				if(!isOk || pImg->Is1D() || pImg->IsCube())
				{
					Msg("Can't open image '%s'\n", pImg->GetName());

					delete pImg;
				}
				else
					pImages.append(pImg);
			}
		}

		// pack atlas
		CreateAtlasImage(pImages, pszOutputName, kvs.GetRootSection());
	}
	else
	{
		MsgError("Can't open '%s'\n", pszAtlasSrcName);
	}
}