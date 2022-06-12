//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas packer - main code
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "utils/KeyValues.h"
#include "utils/RectanglePacker.h"
#include "imaging/ImageLoader.h"
#include "imaging/PixWriter.h"

static const EqString s_outputTag("%OUTPUT%");

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

enum EBlendMode
{
	BLEND_LERP = 0, // OR BLEND_NONE
	BLEND_NONE = BLEND_LERP,
	BLEND_ADD,
	BLEND_SUB,
	BLEND_MUL,
	BLEND_DIV,
	BLEND_LERP_ALPHA,

	// TODO: negate
	BLEND_MODES,
};

static const char* s_blendModeStr[] = 
{
	"lerp",
	"add",
	"sub",
	"mul",
	"div",
	"lerp_alpha",
};

Vector4D fnBlendLerp(const Vector4D& dest, const Vector4D& src, float transparency)
{
	return lerp(dest, src, transparency);
}

Vector4D fnBlendAdd(const Vector4D& dest, const Vector4D& src, float transparency)
{
	return dest + src * transparency;
}

Vector4D fnBlendSub(const Vector4D& dest, const Vector4D& src, float transparency)
{
	return dest - src * transparency;
}

Vector4D fnBlendMul(const Vector4D& dest, const Vector4D& src, float transparency)
{
	// do it like photoshop does
	return fnBlendLerp(dest, dest * src, transparency);
}

Vector4D fnBlendDiv(const Vector4D& dest, const Vector4D& src, float transparency)
{
	// do it like photoshop does
	return fnBlendLerp(dest, dest / src, transparency);
}

Vector4D fnBlendLerpAlpha(const Vector4D& dest, const Vector4D& src, float transparency)
{
	return Vector4D(lerp(dest.xyz(), src.xyz(), transparency*src.w), dest.w);
}

typedef Vector4D (*BLENDPIXFUNC)(const Vector4D& dest, const Vector4D& src, float transparency);

static BLENDPIXFUNC s_BlendFuncs[] = 
{
	fnBlendLerp,
	fnBlendAdd,
	fnBlendSub,
	fnBlendMul,
	fnBlendDiv,
	fnBlendLerpAlpha
};

EBlendMode GetBlendmodeByStr(const char* mode)
{
	for(int i = 0; i < BLEND_MODES; i++)
	{
		if(!stricmp(s_blendModeStr[i], mode))
			return (EBlendMode)i;
	}

	return BLEND_NONE;
}

struct imgLayer_t
{
	imgLayer_t()
	{
		image = NULL;
		transparency = 1.0f;
		color = ColorRGB(1.0f);
		blendMode = BLEND_ADD;
	}

	CImage*		image;
	ColorRGB	color;
	float		transparency;
	EBlendMode	blendMode; 
};

struct imageDesc_t
{
	~imageDesc_t()
	{
		for(int i = 0; i < layers.numElem(); i++)
			delete layers[i].image;
	}

	Array<imgLayer_t> layers{ PP_SL };
	EqString name;
};

//
// Parses image description keybase
//
bool ParseImageDesc(const char* atlasPath, imageDesc_t& dest, KVSection* kv)
{
	EqString atlas_dir = _Es(atlasPath).Path_Strip_Name();
	EqString image_name = KV_GetValueString(kv, 0, NULL);

	if(image_name.Length() == 0)
	{
		MsgError("No valid image name in atlas '%s' line %d\n", atlasPath, kv->line);
		return false;
	}

	// always strip extension
	dest.name = image_name.Path_Strip_Ext();

	if(!kv->IsSection())
	{
		EqString imgName(atlas_dir + image_name);

		CImage* pImg = PPNew CImage();
		bool isOk = pImg->LoadImage(imgName.ToCString());

		if(!isOk || pImg->Is1D() || pImg->IsCube())
		{
			Msg("Can't open image '%s'\n", imgName.ToCString());

			delete pImg;
		}
		else
		{
			imgLayer_t layer;
			layer.image = pImg;

			dest.layers.append(layer);
		}
	}	
	else
	{
		// parse blend modes and colors

		// FORMAT IS:
		// EBlendMode [optional imageName] [optional transparency] [optional R G B]

		for(int i = 0; i < kv->keys.numElem(); i++)
		{
			KVSection* kb = kv->keys[i];

			imgLayer_t layer;
			layer.blendMode = GetBlendmodeByStr( kb->name );
			layer.image = NULL;

			EqString image_path = KV_GetValueString(kb,0,NULL);

			bool hasImagePath = image_path.Length() > 0 && (isalpha(*image_path.ToCString()) || *image_path.ToCString() == '_');

			if(hasImagePath)
			{
				EqString imgName(atlas_dir + image_path);
				CImage* pImg = PPNew CImage();
				bool isOk = pImg->LoadImage(imgName.ToCString());

				if(!isOk || pImg->Is1D() || pImg->IsCube())
				{
					Msg("Can't open image '%s'\n", imgName.ToCString());
					delete pImg;
				}
				else
				{
					layer.image = pImg;
				}

				layer.transparency = KV_GetValueFloat(kb, 1, 1.0f);
				layer.color = KV_GetVector3D(kb, 2, Vector3D(1.0f));
			}
			else
			{
				// since imageName is optional, we're parsing transparency from 1 value
				layer.transparency = KV_GetValueFloat(kb, 0, 1.0f);
				layer.color = KV_GetVector3D(kb, 1, Vector3D(1.0f));
			}

			dest.layers.append(layer);
		}
	}

	//
	// Convert all images to RGBA
	//
	for(int i = 0; i < dest.layers.numElem(); i++)
	{
		CImage* img = dest.layers[i].image;

		if(!img)
			continue;

		bool imageIsOk = true;

		if(img->GetFormat() != FORMAT_RGBA8)
			imageIsOk = img->Convert(FORMAT_RGBA8);

		if(!imageIsOk)
		{
			MsgError("Failed to convert image '%s'\n", dest.name.ToCString() );
			delete dest.layers[i].image;
			dest.layers[i].image = NULL;
		}

		if(i == 0 && !dest.layers[i].image)
		{
			MsgError("Atlas '%s' image '%s' first entry must have image!\n", atlasPath, dest.name.ToCString());
			return false;
		}
	}

	return dest.layers.numElem() > 0;
}

void BlendPixel(ubyte* destPixels, int destStride, const imgLayer_t& layer, int srcStride)
{
	// initial source color is layer color
	Vector4D srcPixel(layer.color, 1.0f);

	if( layer.image ) // apply source pixel color if we have image
	{
		ubyte* srcPixels = layer.image->GetPixels(0,0);
		srcPixel *= Vector4D((float)srcPixels[srcStride] / 255.0f, 
							(float)srcPixels[srcStride+1] / 255.0f,
							(float)srcPixels[srcStride+2] / 255.0f, 
							(float)srcPixels[srcStride+3] / 255.0f);
	}

	ASSERT(layer.blendMode >= 0 && layer.blendMode < BLEND_MODES);

	Vector4D destSrcPixel(	(float)destPixels[destStride] / 255.0f, 
							(float)destPixels[destStride+1] / 255.0f,
							(float)destPixels[destStride+2] / 255.0f, 
							(float)destPixels[destStride+3] / 255.0f);

	Vector4D result = s_BlendFuncs[layer.blendMode](destSrcPixel, srcPixel, layer.transparency);

	destPixels[destStride] = result[0] * 255.0f;
	destPixels[destStride+1] = result[1] * 255.0f;
	destPixels[destStride+2] = result[2] * 255.0f;
	destPixels[destStride+3] = result[3] * 255.0f;
}

#define ROLLING_VALUE(x, max) ((x < 0) ? max+x : ((x >= max) ? x-max : x ))

void BlendAtlasTo(ubyte* pDst, imageDesc_t* srcImage, int dst_x, int dst_y, int dst_wide, int padding, EPaddingMode padMode)
{
	for(int i = 0; i < srcImage->layers.numElem(); i++)
	{
		const imgLayer_t& layer = srcImage->layers[i];

		int src_w, src_h;

		if(layer.image)
		{
			src_w = layer.image->GetWidth(0);
			src_h = layer.image->GetHeight(0);
		}
		else
		{
			src_w = srcImage->layers[0].image->GetWidth(0);
			src_h = srcImage->layers[0].image->GetHeight(0);
		}

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

				// Blend pixel
				BlendPixel(pDst, nDestStride, layer, nSrcStride);
			}
		}
	}
}

inline int AtlasPackComparison(PackerRectangle *const &elem0, PackerRectangle *const &elem1)
{
	return (elem1->width + elem1->height) - (elem0->width + elem0->height);
}

bool CreateAtlasImage(const Array<imageDesc_t*>& images_list, 
						const char* pszOutputImageName, 
						KVSection* pParams)
{
	int padding = KV_GetValueInt(pParams->FindSection("padding"), 0, 0);
	EPaddingMode padMode = PAD_NONE;

	const char* padModeStr = KV_GetValueString(pParams->FindSection("padding"), 1, "none");
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
		CImage* pImg = images_list[i]->layers[0].image;

		Msg("Adding image set '%s' (%d %d)\n", images_list[i]->name.ToCString(), pImg->GetWidth(), pImg->GetHeight());
		packer.AddRectangle( pImg->GetWidth(), pImg->GetHeight(), images_list[i]);
	}

	float	wide = 512,
			tall = 512;

	EqString shaderName = "Base";

	KVSection* pSizeKey = pParams->FindSection("size");

	if(pSizeKey)
	{
		wide = KV_GetValueInt(pSizeKey, 0, 512);
		tall = KV_GetValueInt(pSizeKey, 1, 512);
	}

	KVSection* shaderBase = pParams->FindSection("shader");
	shaderName = KV_GetValueString(shaderBase, 0, "Base");

	// pack
	if(!packer.AssignCoords(wide, tall))//, AtlasPackComparison))
	{
		MsgError("Couldn't assign coordinates, too small primary size!!!\n");
		return false;
	}

	Msg("Trying to generate atlas for '%s' (%g %g)...\n", pszOutputImageName, wide, tall);

	wide = upper_power_of_two(wide);
	tall = upper_power_of_two(tall);

	// create new image
	CImage destImage;
	ubyte* destData = destImage.Create(FORMAT_RGBA8, wide, tall, 1, 1);

	ASSERT(destData);

	EqString file_name = _Es(pszOutputImageName).Path_Strip_Ext();
	EqString mat_file_name = file_name + ".mat";

	// save atlas info
	KeyValues kvs;
	KVSection* pAtlasGroup = kvs.GetRootSection()->CreateSection("atlasgroup", file_name.GetData());

	KeyValues kv_material;
	KVSection* pShaderEntry = kv_material.GetRootSection()->CreateSection(shaderName.GetData());
	pShaderEntry->MergeFrom(shaderBase, true);

	// process setting up
	for (int i = 0; i < pShaderEntry->keys.numElem(); i++)
	{
		KVSection* key = pShaderEntry->keys[i];

		EqString value = KV_GetValueString(key, 0, "");
		if (!value.Length())
			continue;

		if (value.ReplaceSubstr(s_outputTag.ToCString(), file_name.ToCString()) != -1)
			key->SetValue(value.ToCString(), 0);
	}

	Vector2D sizeTexels(1.0f / wide, 1.0f / tall);

	// copy pixels
	for(int i = 0; i < packer.GetRectangleCount(); i++)
	{
		void* userData;
		Rectangle_t rect;

		packer.GetRectangle(rect, &userData, i);
		imageDesc_t* imgDesc = (imageDesc_t*)userData;

		// rgba8 is pretty simple
		BlendAtlasTo(destData, imgDesc, (int)rect.vleftTop.x, (int)rect.vleftTop.y, (int)wide, padding, padMode);

		rect.vleftTop *= sizeTexels;
		rect.vrightBottom *= sizeTexels;

		// add info to keyvalues
		KVSection* rect_kv = pAtlasGroup->CreateSection(imgDesc->name.ToCString());
		rect_kv->AddValue(rect.vleftTop.x);
		rect_kv->AddValue(rect.vleftTop.y);
		rect_kv->AddValue(rect.vrightBottom.x);
		rect_kv->AddValue(rect.vrightBottom.y);
	}

	// done with it
	for(int i = 0; i < images_list.numElem(); i++)
		delete images_list[i];

	// save image as DDS, or TGA ???
	if(destImage.SaveImage(pszOutputImageName))
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

void ProcessNewAtlas(const char* atlasPath, const char* pszOutputName)
{
	Array<imageDesc_t*> imageList{ PP_SL };

	KeyValues kvs;
	if( kvs.LoadFromFile(atlasPath) )
	{
		// try loading images
		for(int i = 0; i < kvs.GetRootSection()->keys.numElem(); i++)
		{
			if(!stricmp(kvs.GetRootSection()->keys[i]->name, "image"))
			{
				KVSection* kb = kvs.GetRootSection()->keys[i];

				imageDesc_t* imgDesc = PPNew imageDesc_t();

				if(ParseImageDesc(atlasPath, *imgDesc, kb))
					imageList.append(imgDesc);
			}
		}

		// pack atlas
		CreateAtlasImage(imageList, pszOutputName, kvs.GetRootSection());
	}
	else
	{
		MsgError("Can't open '%s'\n", atlasPath);
	}
}