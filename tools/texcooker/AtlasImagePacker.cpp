//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas packer - main code
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "utils/KeyValues.h"
#include "utils/RectanglePacker.h"
#include "imaging/ImageLoader.h"
#include "texcooker_defs.h"

unsigned long UpperPowerOfTwo(unsigned long v)
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

constexpr EqStringRef s_blendModeStr[] = 
{
	// none is lerp
	"lerp",
	"add",
	"sub",
	"mul",
	"div",
	"lerp_alpha",
};

static Vector4D fnBlendLerp(const Vector4D& dest, const Vector4D& src, float transparency)
{
	return lerp(dest, src, transparency);
}

static Vector4D fnBlendAdd(const Vector4D& dest, const Vector4D& src, float transparency)
{
	return dest + src * transparency;
}

static Vector4D fnBlendSub(const Vector4D& dest, const Vector4D& src, float transparency)
{
	return dest - src * transparency;
}

static Vector4D fnBlendMul(const Vector4D& dest, const Vector4D& src, float transparency)
{
	// do it like photoshop does
	return fnBlendLerp(dest, dest * src, transparency);
}

static Vector4D fnBlendDiv(const Vector4D& dest, const Vector4D& src, float transparency)
{
	// do it like photoshop does
	return fnBlendLerp(dest, dest / src, transparency);
}

static Vector4D fnBlendLerpAlpha(const Vector4D& dest, const Vector4D& src, float transparency)
{
	return Vector4D(lerp(dest.xyz(), src.xyz(), transparency*src.w), dest.w);
}

using BlendPixFunc = Vector4D(*)(const Vector4D& dest, const Vector4D& src, float transparency);

static BlendPixFunc s_BlendFuncs[] = 
{
	&fnBlendLerp,
	&fnBlendAdd,
	&fnBlendSub,
	&fnBlendMul,
	&fnBlendDiv,
	&fnBlendLerpAlpha
};

static EBlendMode GetBlendmodeByStr(const char* mode)
{
	for(int i = 0; i < BLEND_MODES; i++)
	{
		if(!s_blendModeStr[i].CompareCaseIns(mode))
			return (EBlendMode)i;
	}

	return BLEND_NONE;
}

struct ImgLayer
{
	CRefPtr<CImage>	image;
	ColorRGB		color{ color_white };
	float			transparency{ 1.0f };
	EBlendMode		blendMode{ BLEND_ADD };
};

struct ImageDesc
{
	Array<ImgLayer> layers{ PP_SL };
	EqString name;
};

//
// Parses image description keybase
//
static bool ParseImageDesc(const char* atlasPath, ImageDesc& dest, const KVSection& kv)
{
	EqStringRef imageName;
	if (kv.GetValues(imageName) < 1)
	{
		MsgError("No valid image name in atlas '%s' line %d\n", atlasPath, kv.line);
		return false;
	}

	if(imageName.Length() == 0)
	{
		MsgError("No valid image name in atlas '%s' line %d\n", atlasPath, kv.line);
		return false;
	}

	// always strip extension
	dest.name = fnmPathStripExt(imageName);

	if(!kv.IsSection())
	{
		EqStringRef imageFileName = imageName;
		kv.GetValuesAt(1, imageFileName);

		const EqString imgName = fnmPathCombine(atlasPath, imageFileName);

		CRefPtr<CImage> pImg = CRefPtr_new(CImage);
		bool isOk = pImg->Load(imgName.ToCString());

		if(!isOk || pImg->Is1D() || pImg->IsCube())
		{
			Msg("Can't open image '%s'\n", imgName.ToCString());
		}
		else
		{
			ImgLayer& layer = dest.layers.append();
			layer.image = pImg;
		}
	}	
	else
	{
		// parse blend modes and colors

		// FORMAT IS:
		// EBlendMode [optional imageName] [optional transparency] [optional R G B]

		for(const KVSection& kb : kv.Keys())
		{
			ImgLayer& layer = dest.layers.append();
			layer.blendMode = GetBlendmodeByStr( kb.GetName() );
			layer.image = nullptr;

			EqStringRef imageFileName;
			kb.GetValues(imageFileName);

			const bool hasImagePath = imageFileName.Length() > 0 && (CType::IsAlphabetic(*imageFileName.ToCString()) || *imageFileName.ToCString() == '_');
			if(hasImagePath)
			{
				const EqString imgName = fnmPathCombine(atlasPath, imageFileName);

				CRefPtr<CImage> pImg = CRefPtr_new(CImage);
				bool isOk = pImg->Load(imgName.ToCString());

				if(!isOk || pImg->Is1D() || pImg->IsCube())
				{
					Msg("Can't open image '%s'\n", imgName.ToCString());
				}
				else
				{
					layer.image = pImg;
				}

				layer.transparency = KV_GetValueFloat(&kb, 1, 1.0f);
				layer.color = KV_GetVector3D(&kb, 2, Vector3D(1.0f));
			}
			else
			{
				// since imageName is optional, we're parsing transparency from 1 value
				layer.transparency = KV_GetValueFloat(&kb, 0, 1.0f);
				layer.color = KV_GetVector3D(&kb, 1, Vector3D(1.0f));
			}			
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
			dest.layers[i].image = nullptr;
		}

		if(i == 0 && !dest.layers[i].image)
		{
			MsgError("Atlas '%s' image '%s' first entry must have image!\n", atlasPath, dest.name.ToCString());
			return false;
		}
	}

	return dest.layers.numElem() > 0;
}

static void BlendPixel(ubyte* destPixels, int destStride, const ImgLayer& layer, int srcStride)
{
	const ETextureFormat format = layer.image->GetFormat();
	const int channelCnt = GetChannelCount(format);

	// initial source color is layer color
	Vector4D srcPixel(layer.color, 1.0f);

	if( layer.image ) // apply source pixel color if we have image
	{
		ubyte* srcPixels = layer.image->GetPixels(0,0);
		for (int i = 0; i < channelCnt; ++i)
			srcPixel[i] = (float)srcPixels[srcStride+i] / 255.0f;
	}

	ASSERT(layer.blendMode >= 0 && layer.blendMode < BLEND_MODES);

	Vector4D destSrcPixel;
	for (int i = 0; i < channelCnt; ++i)
		destSrcPixel[i] = (float)destPixels[destStride + i] / 255.0f;

	Vector4D result = s_BlendFuncs[layer.blendMode](destSrcPixel, srcPixel, layer.transparency);
	for (int i = 0; i < channelCnt; ++i)
		destPixels[destStride+i] = result[i] * 255.0f;
}

#define ROLLING_VALUE(x, limit)		((x + limit) % limit)

static void BlendAtlasTo(ubyte* pDst, const ImageDesc* srcImage, int dst_x, int dst_y, int dst_wide, int padding, EPaddingMode padMode)
{
	for(int i = 0; i < srcImage->layers.numElem(); i++)
	{
		const ImgLayer& layer = srcImage->layers[i];
		const ETextureFormat format = layer.image->GetFormat();

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
				int nDestStride = ((dst_x+x) + (dst_y+y)*dst_wide) * GetChannelCount(format);
				int nSrcStride = 0;

				if(x < 0 || y < 0 || x >= src_w || y >= src_h)
				{
					switch(padMode)
					{
						case PAD_CLAMP:
						{
							int nx = clamp(x, 0, src_w-1);
							int ny = clamp(y, 0, src_h-1);
							nSrcStride = (nx + ny*src_w) * GetChannelCount(format);
							break;
						}
						case PAD_REPEAT:
						{
							int nx = ROLLING_VALUE(x, src_w);
							int ny = ROLLING_VALUE(y, src_h);
							nSrcStride = (nx + ny*src_w) * GetChannelCount(format);
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

							nSrcStride = (nx + ny*src_w) * GetChannelCount(format);
							break;
						}
						default:
							continue;	// skip padding
					}
				}
				else
					nSrcStride = (x + y*src_w) * GetChannelCount(format);

				ASSERT(nDestStride >= 0);
				ASSERT(nSrcStride >= 0);

				// Blend pixel
				BlendPixel(pDst, nDestStride, layer, nSrcStride);
			}
		}
	}
}

inline static int AtlasPackComparison(PackerRectangle *const &elem0, PackerRectangle *const &elem1)
{
	return (elem1->width + elem1->height) - (elem0->width + elem0->height);
}

static bool CreateAtlasImage(const Array<ImageDesc>& images_list, 
						const char* materialsPath, const char* outputMaterialName, 
						const KVSection& pParams)
{
	int padding = 0;
	EqStringRef padModeStr;
	pParams.Get("padding").GetValues(padding, padModeStr);

	EPaddingMode padMode = PAD_NONE;
	if(!padModeStr.CompareCaseIns("clamp"))
		padMode = PAD_CLAMP;
	else if(!padModeStr.CompareCaseIns("repeat"))
		padMode = PAD_REPEAT;
	else if(!padModeStr.CompareCaseIns("mirror"))
		padMode = PAD_MIRROR;

	CRectanglePacker packer;
	packer.SetPackPadding( padding );	// we set double padding here

	// add
	for(ImageDesc& imgDesc : images_list)
	{
		CRefPtr<CImage> pImg = imgDesc.layers[0].image;

		Msg("Adding image set '%s' (%d %d)\n", imgDesc.name.ToCString(), pImg->GetWidth(), pImg->GetHeight());
		packer.AddRectangle( pImg->GetWidth(), pImg->GetHeight(), (void*)&imgDesc);
	}

	float wide = 512;
	float tall = 512;
	pParams.Get("size").GetValues(wide, tall);
	if(!packer.AssignCoords(wide, tall))
	{
		MsgError("Couldn't assign coordinates, too small primary size!!!\n");
		return false;
	}

	const KVSection& shaderBase = pParams.Get("shader");

	EqStringRef shaderName = "Base";
	shaderBase.GetValues(shaderName);

	Msg("Trying to generate atlas for '%s' (%g %g)...\n", outputMaterialName, wide, tall);

	wide = UpperPowerOfTwo(wide);
	tall = UpperPowerOfTwo(tall);

	// create new image
	CImage destImage;
	ubyte* destData = destImage.Create(FORMAT_RGBA8, wide, tall, 1, 1);
	ASSERT(destData);

	memset(destData, 0, destImage.GetMipMappedSize(0, destImage.GetMipMapCount()));

	const EqString fullMaterialPath = fnmPathCombine(materialsPath, outputMaterialName);
	const EqString imageFileName = fnmPathApplyExt(fullMaterialPath, s_sourceTextureFileExt);
	const EqString matFileName = fnmPathApplyExt(fullMaterialPath, s_materialFileExt);
	const EqString atlasFileName = fnmPathApplyExt(fullMaterialPath, s_materialAtlasFileExt);

	g_fileSystem->MakeDir(fnmPathStripName(matFileName), SP_ROOT);

	// save atlas info
	KVSection kvs;
	KVSection& atlasGroupKey = kvs.CreateSection("atlasgroup", outputMaterialName);

	KVSection materialKvs;
	KVSection& pShaderEntry = materialKvs.CreateSection(shaderName);
	pShaderEntry.MergeFrom(shaderBase, true);

	// process setting up
	for (KVSection& key : pShaderEntry.Keys())
	{
		EqString value;
		key.GetValues(value);
		if (!value.Length())
			continue;

		if (value.ReplaceSubstr(s_outputTag, outputMaterialName) != -1)
			key.SetValue(value, 0);
	}

	Vector2D sizeTexels(1.0f / wide, 1.0f / tall);

	// copy pixels
	for(int i = 0; i < packer.GetRectangleCount(); i++)
	{
		void* userData;
		AARectangle rect;

		packer.GetRectangle(rect, &userData, i);
		const ImageDesc* imgDesc = (ImageDesc*)userData;

		// rgba8 is pretty simple
		BlendAtlasTo(destData, imgDesc, (int)rect.leftTop.x, (int)rect.leftTop.y, (int)wide, padding, padMode);

		rect.leftTop *= sizeTexels;
		rect.rightBottom *= sizeTexels;

		// add info to keyvalues
		KVSection& rect_kv = atlasGroupKey.CreateSection(imgDesc->name);
		rect_kv.AddValue(rect.leftTop.x);
		rect_kv.AddValue(rect.leftTop.y);
		rect_kv.AddValue(rect.rightBottom.x);
		rect_kv.AddValue(rect.rightBottom.y);
	}

	// save image as DDS, or TGA ???
	if(destImage.SaveImage(imageFileName, SP_ROOT))
	{
		KV_WriteText(g_fileSystem->Open(atlasFileName, FS_OPEN_WRITE, SP_ROOT), kvs);
		KV_WriteText(g_fileSystem->Open(matFileName, FS_OPEN_WRITE, SP_ROOT), materialKvs);
	}
	else
	{
		MsgError("Error while saving '%s' atlas texture...\n", outputMaterialName);
		return false;
	}

	return true;
}

void ProcessAtlasFile(const char* atlasSrcFileName, const char* materialsPath)
{
	KVSection kvs;
	if( !KV_LoadFromFile(atlasSrcFileName, -1, kvs) )
	{
		MsgError("Can't open '%s'\n", atlasSrcFileName);
		return;
	}

	const char* materialFileName = KV_GetValueString(kvs.FindSection("material"), 0, nullptr);
	if(!materialFileName)
	{
		MsgError("Atlas file %s missing 'material'\n", atlasSrcFileName);
		return;
	}

	const EqString atlasDir = fnmPathStripName(atlasSrcFileName);

	Array<ImageDesc> imageList(PP_SL);

	// try loading images
	for(const KVSection& imgDescSec : kvs.Keys("image"))
	{
		const int idx = imageList.numElem();
		if (!ParseImageDesc(atlasDir, imageList.append(), imgDescSec))
			imageList.fastRemoveIndex(idx);
	}

	// pack atlas
	CreateAtlasImage(imageList, materialsPath, materialFileName, kvs);
}