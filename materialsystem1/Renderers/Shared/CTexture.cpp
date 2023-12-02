//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base texture class
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "renderers/ShaderAPI_defs.h"
#include "renderers/IShaderAPI.h"
#include "imaging/ImageLoader.h"
#include "imaging/PixWriter.h"

#include "CTexture.h"

void CTexture::SetName(const char* pszNewName)
{
	ASSERT_MSG(*pszNewName != 0, "Empty texture names are not allowed");
	m_name = pszNewName;
	m_name.Path_FixSlashes();
	m_nameHash = StringToHash(m_name.ToCString(), true);
}

// sets current animated texture frames
void CTexture::SetAnimationFrame(int frame)
{
	m_animFrame = clamp(frame, 0, m_animFrameCount-1);
}

// initializes procedural (lockable) texture
bool CTexture::InitProcedural(const SamplerStateParams& sampler, ETextureFormat format, int width, int height, int depth, int arraySize, int flags)
{
	if (flags & TEXFLAG_CUBEMAP)
	{
		ASSERT_MSG(depth > 1, "depth for TEXFLAG_CUBEMAP should be not greater than 1");
		depth = 0;
	}

	// make texture
	CImagePtr genTex = CRefPtr_new(CImage);
	
	ubyte* newData = genTex->Create(format, width, height, depth, 1, arraySize);

	if(newData)
	{
		const int texDataSize = width * height * depth * arraySize * GetBytesPerPixel(format);
		memset(newData, 0, texDataSize);
	}
	else
	{
		MsgError("ERROR -  Cannot initialize texture '%s', bad format\n", GetName());
		return false;	// don't generate error
	}

	FixedArray<CImagePtr, 1> imgs;
	imgs.append(genTex);

	return Init(sampler, imgs, flags);
}

static void MakeCheckerBoxImage(ubyte* dest, int width, int height, int checkerSize, const MColor& color1, const MColor& color2)
{
	PixelWriter pixelWriter(FORMAT_RGBA8, dest, 0);
	for (int y = 0; y < height; ++y)
	{
		pixelWriter.Seek(0, y);
		for (int x = 0; x < width; ++x)
		{
			if ((x & checkerSize) ^ (y & checkerSize))
				pixelWriter.WritePixel(color1);
			else
				pixelWriter.WritePixel(color2);
		}
	}
}

// generates a new error texture
bool CTexture::GenerateErrorTexture(int flags)
{
	SamplerStateParams texSamplerParams(TEXFILTER_LINEAR, TEXADDRESS_WRAP);

	MColor color1(0, 0, 0, 128);
	MColor color2(255, 64, 255, 255);

	const int depth = (flags & TEXFLAG_CUBEMAP) ? 0 : 1;

	const int CHECKER_SIZE = 4;

	CImagePtr image = CRefPtr_new(CImage);
	image->SetName("Error");
	ubyte* destPixels = image->Create(FORMAT_RGBA8, 64, 64, depth, 1);

	const int size = image->GetMipMappedSize(0, 1);
	if (flags & TEXFLAG_CUBEMAP)
	{
		const int cubeFaceSize = size / 6;
		for (int i = 0; i < 6; ++i)
		{
			MakeCheckerBoxImage(destPixels, image->GetWidth(), image->GetHeight(), CHECKER_SIZE, color1, color2);
			destPixels += cubeFaceSize;
		}
	}
	else
	{
		MakeCheckerBoxImage(destPixels, image->GetWidth(), image->GetHeight(), CHECKER_SIZE, color1, color2);
	}

	image->CreateMipMaps();

	FixedArray<CImagePtr, 1> images;
	images.append(image);

	return Init(texSamplerParams, images, flags);
}